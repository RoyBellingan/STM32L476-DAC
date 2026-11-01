#include "cli.hpp"

#include "adc_in.hpp"
#include "analysis.hpp"
#include "loop_cfg.hpp"
#include "main.h"
#include "pga.hpp"
#include "sine_out.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <numbers>

#ifndef LOOP_GIT_REV
#define LOOP_GIT_REV "unknown"
#endif

namespace cli {
namespace {

/* A `read 500` holds new lines for 10 s, and each reply is ~10x its command:
   the ring must hold what a host sends meanwhile. 1 KB of 96 KB. */
constexpr size_t kLineMax = 128;
constexpr size_t kRxMax   = 1024;

uint8_t g_rx_byte;
uint8_t g_rx_ring[kRxMax];
volatile uint16_t g_rx_head;
volatile uint16_t g_rx_tail;
volatile uint32_t g_rx_drop;   /* bytes lost: ring full */
char g_line[kLineMax];
unsigned g_line_len;
bool g_discarding;

/* `read` runs in steps: wait for the DAC to settle, capture, report. New
   lines wait in the RX ring until it ends. */
enum class Read { Idle, WaitDac, Capture };
Read g_read = Read::Idle;
uint32_t g_read_periods;
uint32_t g_read_deadline;
float g_read_amp;      /* DAC peak codes during the capture */
float g_read_mid;      /* DAC centre codes during the capture */
uint32_t g_read_pga;
float g_read_nom;      /* nominal PGA gain */
bool g_have_capture;

/* All real-time work runs in the DMA interrupts, so blocking here is safe. */
void write(const char *s)
{
	(void)HAL_UART_Transmit(&huart2, reinterpret_cast<uint8_t *>(const_cast<char *>(s)),
	                        static_cast<uint16_t>(std::strlen(s)), 200);
}

void line(const char *s)
{
	write(s);
	write("\r\n");
}

void err(const char *s)
{
	write("@? ");
	line(s);
}

bool ring_push(uint8_t b)
{
	const uint16_t next = static_cast<uint16_t>((g_rx_head + 1u) % kRxMax);
	if (next == g_rx_tail) {
		return false;
	}
	g_rx_ring[g_rx_head] = b;
	g_rx_head = next;
	return true;
}

bool ring_pop(uint8_t &out)
{
	if (g_rx_tail == g_rx_head) {
		return false;
	}
	out = g_rx_ring[g_rx_tail];
	g_rx_tail = static_cast<uint16_t>((g_rx_tail + 1u) % kRxMax);
	return true;
}

void clear_rx_errors()
{
	if (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_ORE) != RESET) {
		__HAL_UART_CLEAR_FLAG(&huart2, UART_CLEAR_OREF);
	}
	if (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_FE) != RESET) {
		__HAL_UART_CLEAR_FLAG(&huart2, UART_CLEAR_FEF);
	}
	if (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_NE) != RESET) {
		__HAL_UART_CLEAR_FLAG(&huart2, UART_CLEAR_NEF);
	}
}

void arm_rx()
{
	(void)HAL_UART_Receive_IT(&huart2, &g_rx_byte, 1);
}

float vdda_now()
{
	const float v = adc_in::vdda();
	return (v > 2.0f && v < 3.7f) ? v : 3.3f;
}

/* ---- commands ---------------------------------------------------------- */

void cmd_help()
{
	line("@H help             this list");
	line("@H id               identity");
	line("@H clk              clock lock, sample rate, f0");
	line("@H amp [V]          set the sine peak in volts (0 .. 1.44), or read it");
	line("@H off              amp 0");
	line("@H freq [Hz]        50 .. 250 Hz; n = even samples per period (62.5 Hz: n=800)");
	line("@H harm [k r [deg]|off]  add harmonic k at r x the fundamental (3 0.3 180: peaky)");
	line("@H mid [V]          centre of the sine (default VDDA/2), or read it");
	line("@H pga [off|1|2|4|8|16] [ext]  OPAMP2 gain, PA6 (D12) in, PB0 (A3) out;");
	line("@H                  ext: VINM on PA7 (D11), read as channel m");
	line("@H pga std [G]      standalone op-amp, external R2 A3-D11, R1 D11-C-GND; G nominal");
	line("@H read [n]         measure n periods (default 10, max 500)");
	line("@H dump [avg|raw]   one period of the last read: averaged, or the raw first one");
	line("@H                  PGA centred output: input at VDDA/(2G) = 0.83/0.41/0.21/0.10 V");
	line("@H stat             counters");
}

void cmd_id()
{
	const uint32_t *uid = reinterpret_cast<const uint32_t *>(UID_BASE);
	char b[128];
	(void)std::snprintf(b, sizeof b, "@I loopsrc rev=%s uid=%08lX%08lX%08lX f0=%.5f",
	                    LOOP_GIT_REV, static_cast<unsigned long>(uid[2]),
	                    static_cast<unsigned long>(uid[1]), static_cast<unsigned long>(uid[0]),
	                    loopcfg::f0_hz(sine_out::period()));
	line(b);
}

void cmd_clk()
{
	char b[128];
	(void)std::snprintf(b, sizeof b, "@K lse=%d sysclk=%lu fs=%.3f f0=%.5f n=%lu",
	                    clock_locked_to_lse() ? 1 : 0,
	                    static_cast<unsigned long>(loopcfg::kSysclkHz), loopcfg::kFsHz,
	                    loopcfg::f0_hz(sine_out::period()),
	                    static_cast<unsigned long>(sine_out::period()));
	line(b);
}

void report_amp()
{
	const float vdda = vdda_now();
	const float code = sine_out::amp_codes();
	const float mid = sine_out::mid_codes();
	char b[128];
	(void)std::snprintf(b, sizeof b, "@S amp=%.5f mid=%.5f code=%.2f midcode=%.2f vdda=%.4f pga=%lu%s",
	                    static_cast<double>(code * vdda / 4096.0f),
	                    static_cast<double>(mid * vdda / 4096.0f),
	                    static_cast<double>(code), static_cast<double>(mid),
	                    static_cast<double>(vdda), static_cast<unsigned long>(pga::gain()),
	                    pga::ext_vinm() ? " ext" : "");
	line(b);
}

/* DAC buffer on: 0.2 V .. VDDA - 0.2 V (DS10198), 248 .. 3848 codes. amp is
   the fundamental's peak; the shape (`harm`) can reach peak x amp. */
bool dac_range_ok(float mid, float amp, float peak)
{
	const float a = amp * peak;
	return (mid - a) >= 248.0f && (mid + a) <= 3848.0f && a <= loopcfg::kDacMaxPeak;
}

bool dac_range_ok(float mid, float amp)
{
	return dac_range_ok(mid, amp, sine_out::wave_peak());
}

void cmd_amp(const char *arg)
{
	if (arg == nullptr || arg[0] == '\0') {
		report_amp();
		return;
	}
	char *end = nullptr;
	const float v = std::strtof(arg, &end);
	if (end == arg || v < 0.0f) {
		err("amp: volts peak, 0 .. 1.44");
		return;
	}
	const float code = v * 4096.0f / vdda_now();
	if (!dac_range_ok(sine_out::mid_codes(), code)) {
		err("amp: mid +- amp must stay in 0.2 .. VDDA-0.2 V (DAC buffer)");
		return;
	}
	sine_out::set_amp_codes(code);
	report_amp();
}

void cmd_mid(const char *arg)
{
	if (arg == nullptr || arg[0] == '\0') {
		report_amp();
		return;
	}
	char *end = nullptr;
	const float v = std::strtof(arg, &end);
	if (end == arg || v < 0.0f) {
		err("mid: volts");
		return;
	}
	const float code = v * 4096.0f / vdda_now();
	if (!dac_range_ok(code, sine_out::amp_codes())) {
		err("mid: mid +- amp must stay in 0.2 .. VDDA-0.2 V (DAC buffer)");
		return;
	}
	sine_out::set_mid_codes(code);
	report_amp();
}

void cmd_pga(const char *arg, const char *arg2)
{
	if (arg != nullptr && std::strcmp(arg, "std") == 0) {
		float nom = 1.0f;
		if (arg2 != nullptr) {
			nom = std::strtof(arg2, nullptr);
		}
		pga::set_standalone(nom);
		report_amp();
		return;
	}
	if (arg != nullptr && arg[0] != '\0') {
		const uint32_t g = (std::strcmp(arg, "off") == 0)
		                 ? 0u : static_cast<uint32_t>(std::strtoul(arg, nullptr, 10));
		const bool ext = (arg2 != nullptr && std::strcmp(arg2, "ext") == 0);
		if (!pga::set_gain(g, ext)) {
			err("pga: off, 1, 2, 4, 8 or 16");
			return;
		}
	}
	report_amp();
}

void report_harm()
{
	char b[160];
	int n = std::snprintf(b, sizeof b, "@P harm=%lu peak=%.4f",
	                      static_cast<unsigned long>(sine_out::harm_count()),
	                      static_cast<double>(sine_out::wave_peak()));
	for (uint32_t j = 0; j < sine_out::harm_count(); j++) {
		const sine_out::Harm &h = sine_out::harm(j);
		n += std::snprintf(b + n, sizeof b - n, " k%lu=%.4f@%.0f", static_cast<unsigned long>(h.k),
		                   static_cast<double>(h.ratio), static_cast<double>(h.deg));
	}
	line(b);
}

/* harm               report
   harm off           pure sine
   harm <k> <r> [deg] set harmonic k to r x the fundamental (r = 0 removes it) */
void cmd_harm(const char *arg, const char *arg2, const char *arg3)
{
	if (arg == nullptr || arg[0] == '\0') {
		report_harm();
		return;
	}
	sine_out::Harm list[sine_out::kMaxHarm];
	uint32_t count = 0;
	if (std::strcmp(arg, "off") != 0) {
		const long k = std::strtol(arg, nullptr, 10);
		const float r = (arg2 != nullptr) ? std::strtof(arg2, nullptr) : -1.0f;
		const float deg = (arg3 != nullptr) ? std::strtof(arg3, nullptr) : 0.0f;
		if (k < 2 || k > static_cast<long>(ana::kHarm) || r < 0.0f || r > 1.0f) {
			err("harm: <k 2..20> <ratio 0..1> [deg], or off");
			return;
		}
		for (uint32_t j = 0; j < sine_out::harm_count(); j++) {
			if (sine_out::harm(j).k != static_cast<uint32_t>(k)) {
				list[count++] = sine_out::harm(j);
			}
		}
		if (r > 0.0f) {
			if (count >= sine_out::kMaxHarm) {
				err("harm: 4 harmonics at most");
				return;
			}
			list[count++] = sine_out::Harm{static_cast<uint32_t>(k), r, deg};
		}
	}
	const float peak = sine_out::preview_peak(list, count);
	if (!dac_range_ok(sine_out::mid_codes(), sine_out::amp_codes(), peak)) {
		err("harm: the shape's peak leaves the DAC range at this amp; lower amp first");
		return;
	}
	if (!sine_out::set_shape(list, count)) {
		err("harm: busy");
		return;
	}
	report_harm();
}

void report_freq()
{
	char b[64];
	(void)std::snprintf(b, sizeof b, "@F f0=%.5f n=%lu",
	                    loopcfg::f0_hz(sine_out::period()),
	                    static_cast<unsigned long>(sine_out::period()));
	line(b);
}

/* Change the period length. The amplitude ramps to 0 first and back after,
   so the loop never sees a step. TIM6 stops for the change, so the DAC and
   the ADC restart together at trigger 0. */
void cmd_freq(const char *arg)
{
	if (arg == nullptr || arg[0] == '\0') {
		report_freq();
		return;
	}
	char *end = nullptr;
	const double hz = std::strtod(arg, &end);
	if (end == arg || hz <= 0.0) {
		err("freq: Hz, 50 .. 250");
		return;
	}
	/* Nearest even n: each ring half must be a whole number of samples. */
	const uint32_t n = 2u * static_cast<uint32_t>(loopcfg::kFsHz / (2.0 * hz) + 0.5);
	if (n < loopcfg::kSamplesMin || n > ana::kNMax) {
		err("freq: Hz, 50 .. 250");
		return;
	}
	if (n == sine_out::period()) {
		report_freq();
		return;
	}

	const float amp = sine_out::amp_codes();
	sine_out::set_amp_codes(0.0f);
	const uint32_t t0 = HAL_GetTick();
	for (;;) {
		uint32_t from = 0;
		if (sine_out::settled(from) && adc_in::current_period() >= from + 1u) {
			break;                      /* one whole flat period has played */
		}
		if (HAL_GetTick() - t0 > 1000u) {
			sine_out::set_amp_codes(amp);
			err("freq: ramp down timeout");
			return;
		}
	}

	CLEAR_BIT(TIM6->CR1, TIM_CR1_CEN);
	TIM6->CNT = 0;
	const bool ok = sine_out::reconfigure(n);
	adc_in::reconfigure(n);
	SET_BIT(TIM6->CR1, TIM_CR1_CEN);
	g_have_capture = false;
	sine_out::set_amp_codes(amp);
	if (!ok) {
		err("freq: DAC DMA restart failed");
		return;
	}
	report_freq();
}

void cmd_stat()
{
	char b[160];
	(void)std::snprintf(b, sizeof b,
	                    "@T up=%lu period=%lu und=%lu lost=%lu ovr=%lu dmaerr=%lu rxdrop=%lu vdda=%.4f vrefcal=%u",
	                    static_cast<unsigned long>(HAL_GetTick()),
	                    static_cast<unsigned long>(adc_in::current_period()),
	                    static_cast<unsigned long>(sine_out::underruns()),
	                    static_cast<unsigned long>(adc_in::lost_halves()),
	                    static_cast<unsigned long>(adc_in::overruns()),
	                    static_cast<unsigned long>(adc_in::dma_errors()),
	                    static_cast<unsigned long>(g_rx_drop),
	                    static_cast<double>(adc_in::vdda()),
	                    static_cast<unsigned>(adc_in::vrefint_cal()));
	line(b);
}

void cmd_read(const char *arg)
{
	uint32_t n = loopcfg::kReadDefaultPeriods;
	if (arg != nullptr && arg[0] != '\0') {
		const long v = std::strtol(arg, nullptr, 10);
		if (v < 1 || v > static_cast<long>(loopcfg::kReadMaxPeriods)) {
			err("read: 1 .. 500 periods");
			return;
		}
		n = static_cast<uint32_t>(v);
	}
	g_read_periods = n;
	g_read = Read::WaitDac;
	/* Settle (<= 3 periods) + capture + margin, at 20 ms a period. */
	/* Settle (<= 3 periods) + capture + margin; one period is n / 50 ms. */
	g_read_deadline = HAL_GetTick() + (n + 10u) * (sine_out::period() / 50u + 1u) + 200u;
}

void cmd_dump(const char *arg)
{
	if (!g_have_capture) {
		err("dump: no read yet");
		return;
	}
	const bool raw = (arg != nullptr && std::strcmp(arg, "raw") == 0);
	char b[96];
	int n = std::snprintf(b, sizeof b, "@W begin %s n=%lu cols=i", raw ? "raw" : "avg",
	                      static_cast<unsigned long>(raw ? 1u : adc_in::capture_periods()));
	for (uint32_t c = 0; c < adc_in::kNch; c++) {
		n += std::snprintf(b + n, sizeof b - n, ",%s", adc_in::chan(c).name);
	}
	line(b);
	const uint16_t *r = adc_in::raw_first_period();
	const double inv = 1.0 / static_cast<double>(adc_in::capture_periods());
	for (uint32_t i = 0; i < adc_in::period_scans(); i++) {
		n = std::snprintf(b, sizeof b, "@W %lu", static_cast<unsigned long>(i));
		for (uint32_t c = 0; c < adc_in::kNch; c++) {
			if (raw) {
				n += std::snprintf(b + n, sizeof b - n, " %u", r[i * adc_in::kNch + c]);
			} else {
				n += std::snprintf(b + n, sizeof b - n, " %.3f", adc_in::accum(c).acc[i] * inv);
			}
		}
		line(b);
	}
	line("@W end");
}

double to_db(double ratio)
{
	return (ratio > 1e-12) ? 20.0 * std::log10(ratio) : -999.0;
}

double wrap_deg(double d)
{
	while (d > 180.0) {
		d -= 360.0;
	}
	while (d <= -180.0) {
		d += 360.0;
	}
	return d;
}

void report_read()
{
	const ana::Basis &tab = sine_out::basis();
	const double f0 = loopcfg::f0_hz(sine_out::period());
	const ana::Result rv = ana::analyze(adc_in::accum(adc_in::kVrefIdx), tab);
	const double vdda = adc_in::vdda_from_mean(rv.mean);
	const bool bad = adc_in::capture_bad();

	char b[200];
	(void)std::snprintf(b, sizeof b,
	                    "@R n=%lu p0=%lu f0=%.5f vdda=%.4f amp=%.5f mid=%.5f pga=%lu lse=%d und=%lu lost=%lu ovr=%lu st=%s",
	                    static_cast<unsigned long>(adc_in::capture_periods()),
	                    static_cast<unsigned long>(adc_in::capture_start()), f0, vdda,
	                    static_cast<double>(g_read_amp) * vdda / 4096.0,
	                    static_cast<double>(g_read_mid) * vdda / 4096.0,
	                    static_cast<unsigned long>(g_read_pga),
	                    clock_locked_to_lse() ? 1 : 0,
	                    static_cast<unsigned long>(sine_out::underruns()),
	                    static_cast<unsigned long>(adc_in::lost_halves()),
	                    static_cast<unsigned long>(adc_in::overruns()), bad ? "bad" : "ok");
	line(b);

	ana::Result rp{};
	ana::Result ri{};
	bool rp_rail = false;
	for (uint32_t c = 0; c < adc_in::kNch; c++) {
		if (c == adc_in::kVrefIdx) {
			continue;
		}
		const adc_in::ChanInfo &ci = adc_in::chan(c);
		const ana::Result r = ana::analyze(adc_in::accum(c), tab);
		/* Volts per code: single-ended 0..VDDA over 4096 codes; differential
		   -VDDA..+VDDA over 4096 codes, 2048 = 0 V. */
		const double vpc = ci.diff ? vdda / 2048.0 : vdda / 4096.0;
		const double dc = ci.diff ? (r.mean - 2048.0) * vpc : r.mean * vpc;
		const double a1_rms = r.h_pk[1] * vpc / std::numbers::sqrt2;
		/* Gain against the DAC command: both are fractions of VDDA, so VDDA
		   cancels. */
		const double dac_pk_codes = static_cast<double>(g_read_amp);
		const double g = (dac_pk_codes > 0.0) ? (r.h_pk[1] * (ci.diff ? 2.0 : 1.0)) / dac_pk_codes
		                                      : 0.0;
		/* Phase at the trigger instant: remove the time from the trigger to
		   the end of this rank's sampling window. The DAC table starts at
		   sin(0) on sample 0 of each period, so dly is the delay from the
		   table to the pin, in us (one DAC sample is 20.0 us). Valid for a
		   continuous signal. On the bare DAC staircase every rank samples the
		   same step, so dly reads 20 us + this rank's hold time. */
		const double ph_deg = r.ph1 * 180.0 / std::numbers::pi;
		const double hold_deg = static_cast<double>(ci.hold_us) * 1e-6 * f0 * 360.0;
		const double ph_trig = wrap_deg(ph_deg - hold_deg);
		const double dly_us = -ph_trig / 360.0 / f0 * 1e6;

		(void)std::snprintf(b, sizeof b,
		                    "@C %s %s dc=%.6f rms=%.6f rmsh=%.6f a1=%.6f g=%.5f ph=%.4f dly=%.2f",
		                    ci.name, ci.pins, dc, r.ac_rms * vpc, r.h_rms * vpc, a1_rms, g,
		                    ph_trig, dly_us);
		write(b);
		(void)std::snprintf(b, sizeof b,
		                    " thd=%.4f dpf=%.5f thdn=%.4f sinad=%.2f enob=%.2f h2=%.1f h3=%.1f h4=%.1f h5=%.1f",
		                    r.thd * 100.0,
		                    (r.h_rms > 0.0) ? r.h_pk[1] / std::numbers::sqrt2 / r.h_rms : 0.0,
		                    r.thdn * 100.0, r.sinad_db, r.enob,
		                    to_db(r.h_pk[2] / r.h_pk[1]), to_db(r.h_pk[3] / r.h_pk[1]),
		                    to_db(r.h_pk[4] / r.h_pk[1]), to_db(r.h_pk[5] / r.h_pk[1]));
		write(b);
		/* Within 0.12 V of a rail: the PGA saturates at 0.1 V (DS10198). */
		const bool rail = !ci.diff && r.periods > 0u
		               && (r.vmin * vpc < 0.12 || r.vmax * vpc > vdda - 0.12);
		(void)std::snprintf(b, sizeof b, " nse=%.1f res=%.2f cf=%.4f min=%u max=%u clip=%lu rail=%d",
		                    r.noise_rms * vpc * 1e6, r.res_max, r.crest,
		                    static_cast<unsigned>(r.vmin), static_cast<unsigned>(r.vmax),
		                    static_cast<unsigned long>(r.clip), rail ? 1 : 0);
		line(b);
		if (c == adc_in::kPgaOutIdx) {
			rp = r;
			rp_rail = rail;
		}
		if (c == adc_in::kPgaInIdx) {
			ri = r;
		}
	}

	/* The PGA measured: output over input, both read in the same scan. dph is
	   the raw phase difference, with no hold-time correction: the test drives
	   the PGA from the DAC staircase, and every rank of a scan samples the same
	   DAC step (measured 2026-11-01: a, d, i all at -0.361 deg raw). DC: the
	   output minus G times the input, i.e. the offset at the output. */
	if (g_read_pga != 0u && ri.h_pk[1] > 0.0) {
		const double vpc = vdda / 4096.0;
		const double dph = wrap_deg((rp.ph1 - ri.ph1) * 180.0 / std::numbers::pi);
		const double gm = rp.h_pk[1] / ri.h_pk[1];
		const bool std_mode = (g_read_pga == pga::kStandalone);
		const double nom = (g_read_nom > 0.0f) ? static_cast<double>(g_read_nom) : 1.0;
		/* DC gain: g in PGA mode; 1 in standalone mode (R1 through a cap). */
		const double dc_gain = std_mode ? 1.0 : nom;
		(void)std::snprintf(b, sizeof b,
		                    "@G pga=%s%.2f gain=%.5f err=%.3f dph=%.4f off=%.2f thd_p=%.4f thd_i=%.4f nse_p_in=%.1f rail=%d",
		                    std_mode ? "std" : "", nom, gm, (gm / nom - 1.0) * 100.0, dph,
		                    (rp.mean - dc_gain * ri.mean) * vpc * 1e3,
		                    rp.thd * 100.0, ri.thd * 100.0,
		                    rp.noise_rms * vpc * 1e6 / gm, rp_rail ? 1 : 0);
		line(b);
	}
}

void read_poll()
{
	if (g_read == Read::Idle) {
		return;
	}
	if (static_cast<int32_t>(HAL_GetTick() - g_read_deadline) >= 0) {
		g_read = Read::Idle;
		err("read: timeout");
		return;
	}
	if (g_read == Read::WaitDac) {
		uint32_t from = 0;
		if (!sine_out::settled(from) || adc_in::busy()) {
			return;
		}
		uint32_t start = adc_in::current_period() + 1u;
		if (from > start) {
			start = from;
		}
		g_read_amp = sine_out::amp_codes();
		g_read_mid = sine_out::mid_codes();
		g_read_pga = pga::gain();
		g_read_nom = pga::nominal();
		(void)adc_in::begin(start, g_read_periods);
		g_read = Read::Capture;
		return;
	}
	if (adc_in::done()) {
		g_have_capture = true;
		report_read();
		adc_in::release();
		g_read = Read::Idle;
	}
}

void dispatch(char *s)
{
	char *cmd = std::strtok(s, " \t");
	if (cmd == nullptr) {
		return;
	}
	char *arg = std::strtok(nullptr, " \t");

	if (std::strcmp(cmd, "help") == 0 || std::strcmp(cmd, "?") == 0) {
		cmd_help();
	} else if (std::strcmp(cmd, "id") == 0) {
		cmd_id();
	} else if (std::strcmp(cmd, "clk") == 0) {
		cmd_clk();
	} else if (std::strcmp(cmd, "amp") == 0) {
		cmd_amp(arg);
	} else if (std::strcmp(cmd, "off") == 0) {
		sine_out::set_amp_codes(0.0f);
		report_amp();
	} else if (std::strcmp(cmd, "harm") == 0) {
		char *a2 = std::strtok(nullptr, " \t");
		char *a3 = std::strtok(nullptr, " \t");
		cmd_harm(arg, a2, a3);
	} else if (std::strcmp(cmd, "freq") == 0) {
		cmd_freq(arg);
	} else if (std::strcmp(cmd, "mid") == 0) {
		cmd_mid(arg);
	} else if (std::strcmp(cmd, "pga") == 0) {
		cmd_pga(arg, std::strtok(nullptr, " \t"));
	} else if (std::strcmp(cmd, "read") == 0) {
		cmd_read(arg);
	} else if (std::strcmp(cmd, "dump") == 0) {
		cmd_dump(arg);
	} else if (std::strcmp(cmd, "stat") == 0) {
		cmd_stat();
	} else {
		err("unknown command, try help");
	}
}

void consume_byte(uint8_t b)
{
	if (b == '\r' || b == '\n') {
		if (!g_discarding && g_line_len > 0u) {
			g_line[g_line_len] = '\0';
			dispatch(g_line);
		}
		g_line_len = 0;
		g_discarding = false;
		return;
	}
	if (g_discarding) {
		return;
	}
	if ((g_line_len + 1u) < kLineMax) {
		g_line[g_line_len++] = static_cast<char>(b);
	} else {
		g_discarding = true;
		g_line_len = 0;
		err("line too long");
	}
}

}

void on_uart_rx()
{
	if (!ring_push(g_rx_byte)) {
		g_rx_drop = g_rx_drop + 1u;
	}
	arm_rx();
}

void on_uart_err()
{
	clear_rx_errors();
	arm_rx();
}

void init()
{
	g_rx_head = 0;
	g_rx_tail = 0;
	g_rx_drop = 0;
	g_line_len = 0;
	g_discarding = false;
	g_read = Read::Idle;
	g_have_capture = false;
	(void)HAL_UART_AbortReceive(&huart2);
	clear_rx_errors();
	arm_rx();
	cmd_id();
	cmd_clk();
}

void poll()
{
	clear_rx_errors();
	read_poll();
	if (g_read != Read::Idle) {
		return;
	}
	uint8_t b;
	while (g_read == Read::Idle && ring_pop(b)) {
		consume_byte(b);
	}
}

}

extern "C" void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart == &huart2) {
		cli::on_uart_rx();
	}
}

extern "C" void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	if (huart == &huart2) {
		cli::on_uart_err();
	}
}
