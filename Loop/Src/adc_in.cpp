#include "adc_in.hpp"

#include "loop_cfg.hpp"
#include "main.h"

#include <cstring>

namespace adc_in {
namespace {

/* The ring holds one period of n scans (n set by reconfigure(), even, <=
   kNMax). Each half is n/2 scans. */
constexpr uint32_t kMaxItems = ana::kNMax * kNch;

/* ADC kernel clock: HCLK / 2 = 39.98 MHz, synchronous (CKMODE = 10), so the
   delay from the TIM6 trigger to the sampling is fixed.

   Sample times (SMPR codes): 5 = 92.5 cycles, 6 = 247.5 cycles. VREFINT needs
   at least 4 us (DS10198), 247.5 cycles is 6.2 us. Conversion = sample + 12.5.
   Scan: 4 x 105 + 260 = 680 cycles = 17.0 us of the 20.0 us (799.5 ADC
   cycles) between triggers. Do not add a rank without removing one.
   hold_us = end of the sampling window of each rank after the trigger.

   `p` (PGA output) comes before `i` (PGA input) and `m` (PGA VINM): sampling
   those pins kicks the PGA, and it has recovered long before the next
   trigger. */
constexpr double kAdcClkMHz = static_cast<double>(loopcfg::kSysclkHz) / 2.0 / 1e6;
constexpr float hold(double cycles) { return static_cast<float>(cycles / kAdcClkMHz); }

constexpr ChanInfo kChans[kNch] = {
	{"a", "PA1=A1 se", 6, false, hold(92.5)},
	{"p", "PB0=A3 pga-out", 15, false, hold(105.0 + 92.5)},
	{"i", "PA6=D12 pga-in", 11, false, hold(210.0 + 92.5)},
	{"m", "PA7=D11 pga-vinm", 12, false, hold(315.0 + 92.5)},
	{"vref", "VREFINT", 0, false, hold(420.0 + 247.5)},
};
/* 2026-11-01: channel "d" (PC0-PC1 differential) removed after its result
   (plan §7.1: ~3 mV step at 0 V differential). The DIFSEL code stays. */
constexpr uint8_t kSmp[kNch] = {5, 5, 5, 5, 6};
static_assert(kChans[kVrefIdx].channel == 0, "kVrefIdx must point at VREFINT");
static_assert(kNch <= 8, "SQR1 holds 4 ranks, SQR2 5 more");

/* RM0351: VREFINT_CAL, raw data at VDDA = VREF+ = 3.0 V, 30 degC. */
const volatile uint16_t *const kVrefintCal =
	reinterpret_cast<const volatile uint16_t *>(0x1FFF75AAu);

enum : uint32_t { kIdle = 0, kArmed, kRunning, kDone };

alignas(4) uint16_t g_ring[kMaxItems];
uint16_t   g_raw[kMaxItems];
ana::Accum g_acc[kNch];

uint32_t          g_n = ana::kNMax;  /* period, scans; changed only with TIM6 stopped */
volatile uint32_t g_scan;          /* global index of the next block's first scan */
volatile uint32_t g_vref_sum;      /* last half, n/2 VREFINT samples */
volatile uint32_t g_state;
volatile uint32_t g_start;
volatile uint32_t g_periods;
volatile bool     g_bad;
volatile uint32_t g_lost;
volatile uint32_t g_ovr;
volatile uint32_t g_dma_err;

/* ADC_CR bits that hardware clears ("rs"): never write them back as 1. */
constexpr uint32_t kCrRs = ADC_CR_ADCAL | ADC_CR_JADSTP | ADC_CR_ADSTP | ADC_CR_JADSTART
                         | ADC_CR_ADSTART | ADC_CR_ADDIS | ADC_CR_ADEN;

void cr_write(uint32_t clear, uint32_t set)
{
	ADC1->CR = (ADC1->CR & ~(kCrRs | clear)) | set;
}

void wait_cycles(uint32_t n)
{
	for (uint32_t i = 0; i < n; i++) {
		__NOP();
	}
}

void gpio_init()
{
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	GPIO_InitTypeDef g = {};
	g.Mode = GPIO_MODE_ANALOG_ADC_CONTROL;   /* also sets GPIOx_ASCR on the L47x */
	g.Pull = GPIO_NOPULL;
	g.Pin = GPIO_PIN_1;
	HAL_GPIO_Init(GPIOA, &g);
	g.Pin = GPIO_PIN_0 | GPIO_PIN_1;
	HAL_GPIO_Init(GPIOC, &g);
}

void dma_init()
{
	__HAL_RCC_DMA1_CLK_ENABLE();
	DMA1_Channel1->CCR = 0;
	DMA1_CSELR->CSELR &= ~DMA_CSELR_C1S;     /* request 0 = ADC1 */
	DMA1_Channel1->CPAR = reinterpret_cast<uint32_t>(&ADC1->DR);
	DMA1_Channel1->CMAR = reinterpret_cast<uint32_t>(g_ring);
	DMA1_Channel1->CNDTR = g_n * kNch;
	DMA1->IFCR = DMA_IFCR_CGIF1;
	DMA1_Channel1->CCR = DMA_CCR_PL_1 | DMA_CCR_MSIZE_0 | DMA_CCR_PSIZE_0 | DMA_CCR_MINC
	                   | DMA_CCR_CIRC | DMA_CCR_HTIE | DMA_CCR_TCIE | DMA_CCR_TEIE;
	DMA1_Channel1->CCR |= DMA_CCR_EN;
	HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 1, 0);
	HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
}

void adc_init()
{
	__HAL_RCC_ADC_CLK_ENABLE();
	MODIFY_REG(RCC->CCIPR, RCC_CCIPR_ADCSEL, RCC_CCIPR_ADCSEL);   /* SYSCLK; unused in sync mode */

	/* All ADCs are off here, so the common register can be written. */
	ADC123_COMMON->CCR = ADC_CCR_CKMODE_1 | ADC_CCR_VREFEN;

	cr_write(ADC_CR_DEEPPWD, 0);
	cr_write(0, ADC_CR_ADVREGEN);
	HAL_Delay(1);                   /* tADCVREG_STUP 20 us; VREFINT start-up */

	uint32_t difsel = 0;
	for (const ChanInfo &c : kChans) {
		if (c.diff) {
			difsel |= 1u << c.channel;
		}
	}
	ADC1->DIFSEL = difsel;

	/* Calibrate both modes; the factors for each are kept. */
	cr_write(ADC_CR_ADCALDIF, ADC_CR_ADCAL);
	while ((ADC1->CR & ADC_CR_ADCAL) != 0u) {
	}
	cr_write(0, ADC_CR_ADCALDIF | ADC_CR_ADCAL);
	while ((ADC1->CR & ADC_CR_ADCAL) != 0u) {
	}
	cr_write(ADC_CR_ADCALDIF, 0);
	wait_cycles(100);               /* >= 4 ADC clocks before ADEN */

	ADC1->ISR = ADC_ISR_ADRDY;
	cr_write(0, ADC_CR_ADEN);
	while ((ADC1->ISR & ADC_ISR_ADRDY) == 0u) {
	}

	uint32_t smpr1 = 0;
	uint32_t smpr2 = 0;
	uint32_t sqr1 = kNch - 1u;
	uint32_t sqr2 = 0;
	for (uint32_t r = 0; r < kNch; r++) {
		const uint32_t ch = kChans[r].channel;
		if (ch < 10u) {
			smpr1 |= static_cast<uint32_t>(kSmp[r]) << (3u * ch);
		} else {
			smpr2 |= static_cast<uint32_t>(kSmp[r]) << (3u * (ch - 10u));
		}
		if (r < 4u) {
			sqr1 |= ch << (6u + 6u * r);   /* SQ1..SQ4 at bits 6, 12, 18, 24 */
		} else {
			sqr2 |= ch << (6u * (r - 4u)); /* SQ5..SQ9 at bits 0, 6, ... */
		}
	}
	ADC1->SMPR1 = smpr1;
	ADC1->SMPR2 = smpr2;
	ADC1->SQR1 = sqr1;
	ADC1->SQR2 = sqr2;
	ADC1->CFGR2 = 0;

	/* One scan per TIM6_TRGO rising edge (EXTSEL 13), DMA circular,
	   12 bits right-aligned. OVRMOD = 1 keeps the DMA running; an overrun is
	   counted, and it makes the capture invalid. */
	ADC1->CFGR = ADC_CFGR_JQDIS | ADC_CFGR_DMAEN | ADC_CFGR_DMACFG
	           | (13u << ADC_CFGR_EXTSEL_Pos) | ADC_CFGR_EXTEN_0 | ADC_CFGR_OVRMOD;

	ADC1->ISR = ADC_ISR_OVR;
	ADC1->IER = ADC_IER_OVRIE;
	HAL_NVIC_SetPriority(ADC1_2_IRQn, 2, 0);
	HAL_NVIC_EnableIRQ(ADC1_2_IRQn);
}

void on_half(uint32_t h)
{
	const uint32_t n = g_n;
	const uint32_t half_scans = n / 2u;
	const uint32_t half_items = half_scans * kNch;

	/* The half is complete only while the DMA writes the other one. */
	const uint32_t pos = n * kNch - DMA1_Channel1->CNDTR;
	const bool late = (h == 0u) ? (pos < half_items) : (pos >= half_items);

	uint32_t block = g_scan;
	if ((block % n) != h * half_scans) {
		block += half_scans;       /* a whole half went missing: keep the phase */
		g_lost = g_lost + 1u;
		if (g_state == kRunning) {
			g_bad = true;
		}
	}
	if (late) {
		g_lost = g_lost + 1u;
		if (g_state == kRunning) {
			g_bad = true;
		}
	}

	const uint32_t period = block / n;
	const uint32_t phase0 = block % n;
	const uint16_t *base = &g_ring[h * half_items];

	uint32_t s = 0;
	for (uint32_t i = 0; i < half_scans; i++) {
		s += base[i * kNch + kVrefIdx];
	}
	g_vref_sum = s;

	if (g_state == kArmed && phase0 == 0u && period >= g_start) {
		g_start = period;
		g_state = kRunning;
	}
	if (g_state == kRunning) {
		for (uint32_t c = 0; c < kNch; c++) {
			g_acc[c].add_block(base + c, kNch, half_scans, phase0);
		}
		if (period == g_start) {
			std::memcpy(&g_raw[phase0 * kNch], base, half_items * sizeof(uint16_t));
		}
		if (period == g_start + g_periods - 1u && phase0 == n - half_scans) {
			g_state = kDone;
		}
	}
	g_scan = block + half_scans;
}

}

const ChanInfo &chan(uint32_t c)
{
	return kChans[c];
}

void init()
{
	g_scan = 0;
	g_state = kIdle;
	g_lost = 0;
	g_ovr = 0;
	g_dma_err = 0;
	g_vref_sum = 0;
	gpio_init();
	adc_init();
	dma_init();
	cr_write(0, ADC_CR_ADSTART);    /* armed: converts on each TIM6_TRGO */
}

void reconfigure(uint32_t n)
{
	/* TIM6 is stopped by the caller: no trigger can arrive. */
	cr_write(0, ADC_CR_ADSTP);
	while ((ADC1->CR & ADC_CR_ADSTART) != 0u) {
	}
	DMA1_Channel1->CCR &= ~DMA_CCR_EN;
	/* A conversion done before the stop would make a DMA request as soon as
	   the channel is on, and shift the channel order by one. Read DR to
	   clear EOC, and clear the flags. */
	(void)ADC1->DR;
	ADC1->ISR = ADC_ISR_EOC | ADC_ISR_EOS | ADC_ISR_OVR;
	DMA1->IFCR = DMA_IFCR_CGIF1;

	g_n = n;
	g_scan = 0;
	g_state = kIdle;
	g_vref_sum = 0;
	DMA1_Channel1->CNDTR = n * kNch;
	DMA1_Channel1->CCR |= DMA_CCR_EN;
	cr_write(0, ADC_CR_ADSTART);
}

uint32_t period_scans()
{
	return g_n;
}

uint32_t current_period()
{
	return g_scan / g_n;
}

bool begin(uint32_t start_period, uint32_t periods)
{
	if (g_state == kArmed || g_state == kRunning) {
		return false;
	}
	g_state = kIdle;
	for (ana::Accum &a : g_acc) {
		a.reset(g_n);
	}
	g_bad = false;
	g_start = start_period;
	g_periods = periods;
	__DMB();
	g_state = kArmed;
	return true;
}

bool busy()
{
	return g_state == kArmed || g_state == kRunning;
}

bool done()
{
	return g_state == kDone;
}

bool capture_bad()
{
	return g_bad;
}

void release()
{
	if (g_state == kDone) {
		g_state = kIdle;
	}
}

uint32_t capture_start()
{
	return g_start;
}

uint32_t capture_periods()
{
	return g_periods;
}

const ana::Accum &accum(uint32_t c)
{
	return g_acc[c];
}

const uint16_t *raw_first_period()
{
	return g_raw;
}

uint16_t vrefint_cal()
{
	return *kVrefintCal;
}

float vdda_from_mean(double vrefint_mean_codes)
{
	if (vrefint_mean_codes <= 0.0) {
		return 0.0f;
	}
	return static_cast<float>(3.0 * static_cast<double>(*kVrefintCal) / vrefint_mean_codes);
}

float vdda()
{
	const uint32_t s = g_vref_sum;
	return vdda_from_mean(static_cast<double>(s) / (g_n / 2u));
}

uint32_t lost_halves()
{
	return g_lost;
}

uint32_t overruns()
{
	return g_ovr;
}

uint32_t dma_errors()
{
	return g_dma_err;
}

/* Called from the IRQ handlers below. */
void irq_dma()
{
	const uint32_t isr = DMA1->ISR;
	if ((isr & DMA_ISR_TEIF1) != 0u) {
		DMA1->IFCR = DMA_IFCR_CTEIF1;
		g_dma_err = g_dma_err + 1u;
	}
	if ((isr & DMA_ISR_HTIF1) != 0u) {
		DMA1->IFCR = DMA_IFCR_CHTIF1;
		on_half(0);
	}
	if ((isr & DMA_ISR_TCIF1) != 0u) {
		DMA1->IFCR = DMA_IFCR_CTCIF1;
		on_half(1);
	}
}

void irq_adc()
{
	if ((ADC1->ISR & ADC_ISR_OVR) != 0u) {
		ADC1->ISR = ADC_ISR_OVR;
		g_ovr = g_ovr + 1u;
		if (g_state == kRunning) {
			g_bad = true;
		}
	}
}

}

extern "C" void DMA1_Channel1_IRQHandler(void)
{
	adc_in::irq_dma();
}

extern "C" void ADC1_2_IRQHandler(void)
{
	adc_in::irq_adc();
}
