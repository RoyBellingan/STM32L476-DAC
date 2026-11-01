#include "sine_out.hpp"

#include "analysis.hpp"
#include "loop_cfg.hpp"
#include "main.h"

namespace sine_out {
namespace {

/* The ring holds two periods of n samples (n set by reconfigure(), even,
   <= kNMax). The same basis serves the DAC fill and the DFT. */
alignas(4) uint32_t g_ring[2u * ana::kNMax];
ana::Basis g_basis;
uint32_t   g_n = ana::kNMax;           /* changed only with TIM6 stopped */

/* The DAC shape: sin + sum of r_k sin(k w i + phi_k), fundamental peak 1. Two
   tables: the ISR plays g_wave[g_cur]; main builds a new shape in the other
   one and sets g_pending; the next fill cross-fades over one period and
   swaps. The DFT basis stays a pure sine. */
float             g_wave[2][ana::kNMax];
float             g_wave_peak[2];
volatile uint32_t g_cur;
volatile bool     g_pending;
Harm              g_harm[kMaxHarm];
uint32_t          g_harm_n;

volatile float    g_amp_tgt;       /* written by main */
float             g_amp_cur;       /* ISR only */
volatile float    g_amp_applied;   /* the target the last fill used */
volatile float    g_mid_tgt;       /* centre code; written by main */
float             g_mid_cur;
volatile float    g_mid_applied;
volatile uint32_t g_stable_from;   /* first period with no ramp at g_amp_applied */
volatile uint32_t g_next_period;   /* the period the next fill writes */
volatile uint32_t g_underrun;

uint32_t to_code(float x)
{
	/* x > 0 always (the CLI keeps mid - amp >= 0.2 V), so +0.5 rounds. */
	int32_t r = static_cast<int32_t>(x + 0.5f);
	if (r < 0) {
		r = 0;
	}
	if (r > 4095) {
		r = 4095;
	}
	return static_cast<uint32_t>(r);
}

/* One whole period into one half of the ring. */
void fill(uint32_t offset)
{
	const float tgt = g_amp_tgt;
	const float a0  = g_amp_cur;
	const float mtgt = g_mid_tgt;
	const float m0   = g_mid_cur;
	const uint32_t p = g_next_period;
	uint32_t *dst = &g_ring[offset];
	const uint32_t n = g_n;
	const bool morph = g_pending;
	const float *w0 = g_wave[g_cur];

	if (tgt == a0 && mtgt == m0 && !morph) {
		for (uint32_t i = 0; i < n; i++) {
			dst[i] = to_code(m0 + a0 * w0[i]);
		}
	} else {
		const float *w1 = morph ? g_wave[g_cur ^ 1u] : w0;
		const float inv   = 1.0f / static_cast<float>(n);
		const float step  = (tgt - a0) * inv;
		const float mstep = (mtgt - m0) * inv;
		for (uint32_t i = 0; i < n; i++) {
			const float k = static_cast<float>(i + 1u);
			const float w = w0[i] + (w1[i] - w0[i]) * (k * inv);
			dst[i] = to_code(m0 + mstep * k + (a0 + step * k) * w);
		}
		g_amp_cur = tgt;
		g_mid_cur = mtgt;
		if (morph) {
			g_cur = g_cur ^ 1u;
			g_pending = false;
		}
		g_stable_from = p + 1u;
	}
	g_amp_applied = tgt;
	g_mid_applied = mtgt;
	g_next_period = p + 1u;
}

/* The DMA's next item, 0 .. 2N-1. */
uint32_t dma_pos()
{
	return (2u * g_n) - hdma_dac1.Instance->CNDTR;
}

void dma_ht(DMA_HandleTypeDef *hdma)
{
	(void)hdma;
	/* The first half is free only while the DMA reads the second one. */
	if (dma_pos() < g_n) {
		g_underrun = g_underrun + 1u;
	}
	fill(0);
}

void dma_tc(DMA_HandleTypeDef *hdma)
{
	(void)hdma;
	if (dma_pos() >= g_n) {
		g_underrun = g_underrun + 1u;
	}
	fill(g_n);
}

void dma_err(DMA_HandleTypeDef *hdma)
{
	(void)hdma;
	g_underrun = g_underrun + 1u;
}

/* sin(k w i + phi) from the basis: sin(kwi) cos(phi) + cos(kwi) sin(phi). */
float build_wave(float *dst, const Harm *h, uint32_t count)
{
	const uint32_t n = g_n;
	float peak = 0.0f;
	for (uint32_t i = 0; i < n; i++) {
		float v = g_basis.sin[i];
		for (uint32_t j = 0; j < count; j++) {
			const uint32_t idx = (h[j].k * i) % n;
			const float rad = h[j].deg * 0.017453293f;
			v += h[j].ratio * (g_basis.sin[idx] * __builtin_cosf(rad)
			                 + g_basis.cos[idx] * __builtin_sinf(rad));
		}
		dst[i] = v;
		const float a = (v >= 0.0f) ? v : -v;
		if (a > peak) {
			peak = a;
		}
	}
	return peak;
}

}

void init()
{
	g_n = ana::kNMax;
	g_basis.make(g_n);
	g_harm_n = 0;
	g_cur = 0;
	g_pending = false;
	g_wave_peak[0] = build_wave(g_wave[0], g_harm, 0);
	g_amp_tgt = 0.0f;
	g_amp_cur = 0.0f;
	g_amp_applied = 0.0f;
	g_mid_tgt = static_cast<float>(loopcfg::kDacMid);
	g_mid_cur = g_mid_tgt;
	g_mid_applied = g_mid_tgt;
	g_stable_from = 0;
	g_next_period = 0;
	g_underrun = 0;
	for (uint32_t i = 0; i < 2u * ana::kNMax; i++) {
		g_ring[i] = static_cast<uint32_t>(loopcfg::kDacMid);
	}
}

bool start()
{
	g_next_period = 0;
	fill(0);
	fill(g_n);

	if (hdac1.DMA_Handle1 == nullptr) {
		return false;
	}
	hdac1.DMA_Handle1->XferHalfCpltCallback = dma_ht;
	hdac1.DMA_Handle1->XferCpltCallback = dma_tc;
	hdac1.DMA_Handle1->XferErrorCallback = dma_err;

	if (HAL_DMA_Start_IT(hdac1.DMA_Handle1, reinterpret_cast<uint32_t>(g_ring),
	                     reinterpret_cast<uint32_t>(&DAC1->DHR12R1), 2u * g_n) != HAL_OK) {
		return false;
	}
	if (HAL_DAC_Start(&hdac1, DAC_CHANNEL_1) != HAL_OK) {
		return false;
	}
	SET_BIT(DAC1->CR, DAC_CR_DMAEN1 | DAC_CR_EN1 | DAC_CR_TEN1);
	DAC1->DHR12R1 = g_ring[0];
	return true;
}

void set_amp_codes(float peak_codes)
{
	if (peak_codes < 0.0f) {
		peak_codes = 0.0f;
	}
	if (peak_codes > loopcfg::kDacMaxPeak) {
		peak_codes = loopcfg::kDacMaxPeak;
	}
	g_amp_tgt = peak_codes;
}

float amp_codes()
{
	return g_amp_tgt;
}

void set_mid_codes(float mid_codes)
{
	g_mid_tgt = mid_codes;
}

float mid_codes()
{
	return g_mid_tgt;
}

bool settled(uint32_t &from_period)
{
	__disable_irq();
	const bool ok = (g_amp_applied == g_amp_tgt) && (g_mid_applied == g_mid_tgt) && !g_pending;
	from_period = g_stable_from;
	__enable_irq();
	return ok;
}

uint32_t underruns()
{
	return g_underrun;
}

bool reconfigure(uint32_t n)
{
	/* TIM6 is stopped by the caller, and the amplitude is already 0: the DAC
	   holds its last output (mid-scale) through the change. */
	CLEAR_BIT(DAC1->CR, DAC_CR_DMAEN1);
	(void)HAL_DMA_Abort(hdac1.DMA_Handle1);

	g_n = n;
	g_basis.make(n);
	g_pending = false;
	g_wave_peak[g_cur] = build_wave(g_wave[g_cur], g_harm, g_harm_n);
	g_amp_cur = g_amp_tgt;
	g_mid_cur = g_mid_tgt;
	g_stable_from = 0;
	g_next_period = 0;
	fill(0);
	fill(n);

	hdac1.DMA_Handle1->State = HAL_DMA_STATE_READY;
	hdac1.DMA_Handle1->Lock = HAL_UNLOCKED;
	if (HAL_DMA_Start_IT(hdac1.DMA_Handle1, reinterpret_cast<uint32_t>(g_ring),
	                     reinterpret_cast<uint32_t>(&DAC1->DHR12R1), 2u * n) != HAL_OK) {
		return false;
	}
	SET_BIT(DAC1->CR, DAC_CR_DMAEN1);
	DAC1->DHR12R1 = g_ring[0];
	return true;
}

uint32_t period()
{
	return g_n;
}

const ana::Basis &basis()
{
	return g_basis;
}

float preview_peak(const Harm *h, uint32_t count)
{
	/* Built in a scratch table: the spare one may still be pending. */
	static float scratch[ana::kNMax];
	return build_wave(scratch, h, count);
}

bool set_shape(const Harm *h, uint32_t count)
{
	if (count > kMaxHarm) {
		return false;
	}
	const uint32_t t0 = HAL_GetTick();
	while (g_pending) {                 /* the last change has not played yet */
		if (HAL_GetTick() - t0 > 200u) {
			return false;
		}
	}
	const uint32_t spare = g_cur ^ 1u;
	g_wave_peak[spare] = build_wave(g_wave[spare], h, count);
	for (uint32_t j = 0; j < count; j++) {
		g_harm[j] = h[j];
	}
	g_harm_n = count;
	__DMB();
	g_pending = true;
	return true;
}

uint32_t harm_count()
{
	return g_harm_n;
}

const Harm &harm(uint32_t j)
{
	return g_harm[j];
}

float wave_peak()
{
	return g_pending ? g_wave_peak[g_cur ^ 1u] : g_wave_peak[g_cur];
}

}
