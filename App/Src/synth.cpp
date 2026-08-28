#include "synth.hpp"

#include "main.h"
#include "scenario.hpp"

#include <algorithm>
#include <array>

namespace synth {
namespace {

alignas(4) std::array<uint32_t, kRingWords> g_ring{};
volatile uint32_t g_pos;
volatile bool g_running;
volatile bool g_done;
volatile uint32_t g_need_fill;
int32_t g_level_q12 = 4096;
int32_t g_noise_q15;
uint32_t g_rng = 0xA5A5A5A5u;
int32_t g_floor_q15;
bool g_static;                /* the DMA is now free-running on a fixed ring */
bool g_static_ok;             /* this scenario is eligible to become static */
bool g_static_silenced;
uint32_t g_static_start;
uint32_t g_static_end;

int32_t xorshift_noise()
{
	uint32_t x = g_rng;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	g_rng = x;
	return static_cast<int32_t>(static_cast<int16_t>(x >> 16));
}

/* Turn a block of signed samples into DAC codes, in place. The ring holds
   uint32_t and we stage int32_t in the same words; signed and unsigned of the
   same type may alias, so this is well defined. */
void to_dac(uint32_t *dst, uint32_t n, int32_t noise_q)
{
	const int32_t *sv = reinterpret_cast<const int32_t *>(dst);
	const int32_t level = g_level_q12;

	if (level == 4096 && noise_q == 0) {
		for (uint32_t i = 0; i < n; i++) {
			const int32_t s = std::clamp(sv[i], -kDacPeak, kDacPeak);
			dst[i] = static_cast<uint32_t>(kDacMid + s);
		}
		return;
	}
	for (uint32_t i = 0; i < n; i++) {
		int32_t s = (sv[i] * level) >> 12;
		if (noise_q != 0) {
			s += (xorshift_noise() * noise_q) >> 15;
		}
		s = std::clamp(s, -kDacPeak, kDacPeak);
		dst[i] = static_cast<uint32_t>(kDacMid + s);
	}
}

void pack_half(uint32_t offset)
{
	uint32_t *dst = &g_ring[offset];
	const uint32_t pos = g_pos;

	scenario::fill(reinterpret_cast<int32_t *>(dst), pos, kHalfWords);

	/* The bench floor runs during the preamble only. kPreamble is a whole
	   number of rings, so a half buffer never straddles the boundary and one
	   test per block is enough. */
	int32_t noise_q = g_noise_q15;
	if (pos < kPreamble && g_floor_q15 > noise_q) {
		noise_q = g_floor_q15;
	}
	to_dac(dst, kHalfWords, noise_q);

	g_pos = pos + kHalfWords;
	if (g_pos >= scenario::duration()) {
		g_done = true;
	}
}

/* Static-ring mode. kPhaseInc puts a whole number of tone cycles in one ring,
   so a ring filled from sample 0 joins itself seamlessly. Fill it once, then
   the DMA runs circular with no refill at all and a boundary glitch becomes
   impossible rather than merely unlikely. level and snr are sampled here and
   do not track changes made while the scenario runs. */
void fill_static_ring()
{
	scenario::rewind();
	scenario::fill(reinterpret_cast<int32_t *>(g_ring.data()), 0, kRingWords);
	to_dac(g_ring.data(), kRingWords, 0);
}

void silence_ring()
{
	for (uint32_t i = 0; i < kRingWords; i++) {
		g_ring[i] = static_cast<uint32_t>(kDacMid);
	}
}

void dma_ht(DMA_HandleTypeDef *hdma)
{
	(void)hdma;
	g_need_fill |= 1u;
}

void dma_tc(DMA_HandleTypeDef *hdma)
{
	(void)hdma;
	g_need_fill |= 2u;
}

void dma_err(DMA_HandleTypeDef *hdma)
{
	(void)hdma;
	g_done = true;
}

uint32_t take_need_fill()
{
	__disable_irq();
	const uint32_t need = g_need_fill;
	g_need_fill = 0;
	__enable_irq();
	return need;
}

}

void init()
{
	g_pos = 0;
	g_running = false;
	g_done = false;
	g_need_fill = 0;
	g_level_q12 = 4096;
	g_noise_q15 = 0;
	g_floor_q15 = 0;
	g_static = false;
	g_static_ok = false;
	g_static_silenced = false;
	g_static_start = 0;
	g_static_end = 0;
}

void set_level_q12(int32_t gain_q12)
{
	g_level_q12 = std::clamp<int32_t>(gain_q12, 0, 8192);
}

void set_noise_q15(int32_t noise_q15)
{
	g_noise_q15 = std::clamp<int32_t>(noise_q15, 0, 32767);
}

void set_floor_q15(int32_t floor_q15)
{
	g_floor_q15 = std::clamp<int32_t>(floor_q15, 0, 32767);
}

int32_t floor_q15()
{
	return g_floor_q15;
}

int32_t level_q12()
{
	return g_level_q12;
}

int32_t noise_q15()
{
	return g_noise_q15;
}

bool start()
{
	stop();
	scenario::rewind();
	g_pos = 0;
	g_done = false;
	g_need_fill = 0;
	g_rng = 0xA5A5A5A5u;

	uint32_t tone_start = 0;
	uint32_t tone_len = 0;
	g_static_ok = scenario::static_tone(tone_start, tone_len) && (g_noise_q15 == 0);
	g_static_start = tone_start;
	g_static_end = tone_start + tone_len;
	g_static_silenced = false;
	g_static = false;

	if (g_static_ok && tone_start == 0u) {
		fill_static_ring();
		g_static = true;
		g_pos = 0;
	} else {
		pack_half(0);
		pack_half(kHalfWords);
	}

	if (hdac1.DMA_Handle1 == nullptr) {
		return false;
	}

	hdac1.DMA_Handle1->XferHalfCpltCallback = dma_ht;
	hdac1.DMA_Handle1->XferCpltCallback = dma_tc;
	hdac1.DMA_Handle1->XferErrorCallback = dma_err;
	hdac1.DMA_Handle1->State = HAL_DMA_STATE_READY;
	hdac1.DMA_Handle1->Lock = HAL_UNLOCKED;
	hdac1.State = HAL_DAC_STATE_READY;
	hdac1.Lock = HAL_UNLOCKED;
	htim6.State = HAL_TIM_STATE_READY;
	htim6.Lock = HAL_UNLOCKED;

	if (HAL_DMA_Start_IT(hdac1.DMA_Handle1,
	                     reinterpret_cast<uint32_t>(g_ring.data()),
	                     reinterpret_cast<uint32_t>(&DAC1->DHR12R1),
	                     kRingWords) != HAL_OK) {
		return false;
	}

	if (HAL_DAC_Start(&hdac1, DAC_CHANNEL_1) != HAL_OK) {
		return false;
	}

	SET_BIT(DAC1->CR, DAC_CR_DMAEN1 | DAC_CR_EN1 | DAC_CR_TEN1);
	DAC1->DHR12R1 = g_ring[0];

	HAL_GPIO_WritePin(SYNC_GPIO_Port, SYNC_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(RUN_LED_GPIO_Port, RUN_LED_Pin, GPIO_PIN_SET);
	g_running = true;

	if (HAL_TIM_Base_Start(&htim6) != HAL_OK) {
		SET_BIT(TIM6->CR1, TIM_CR1_CEN);
	}
	SET_BIT(TIM6->CR1, TIM_CR1_CEN);
	return true;
}

void stop()
{
	g_running = false;
	g_need_fill = 0;
	HAL_GPIO_WritePin(SYNC_GPIO_Port, SYNC_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(RUN_LED_GPIO_Port, RUN_LED_Pin, GPIO_PIN_RESET);

	CLEAR_BIT(TIM6->CR1, TIM_CR1_CEN);
	(void)HAL_TIM_Base_Stop(&htim6);
	CLEAR_BIT(DAC1->CR, DAC_CR_DMAEN1);
	(void)HAL_DAC_Stop(&hdac1, DAC_CHANNEL_1);
	if (hdac1.DMA_Handle1 != nullptr) {
		if (hdac1.DMA_Handle1->State == HAL_DMA_STATE_BUSY) {
			(void)HAL_DMA_Abort(hdac1.DMA_Handle1);
		}
		hdac1.DMA_Handle1->State = HAL_DMA_STATE_READY;
		hdac1.DMA_Handle1->Lock = HAL_UNLOCKED;
	}
	htim6.State = HAL_TIM_STATE_READY;
	htim6.Lock = HAL_UNLOCKED;
	g_done = false;
	g_static = false;
	g_static_ok = false;
	g_static_silenced = false;
}

void poll()
{
	if (!g_running) {
		return;
	}
	const uint32_t need = take_need_fill();

	if (g_static) {
		/* Nothing to produce. Just account for what the DMA has played, so
		   the scenario still ends on time. */
		uint32_t adv = 0;
		if ((need & 1u) != 0u) {
			adv += kHalfWords;
		}
		if ((need & 2u) != 0u) {
			adv += kHalfWords;
		}
		if (adv != 0u) {
			g_pos += adv;
			if (!g_static_silenced && g_pos >= g_static_end) {
				silence_ring();
				g_static_silenced = true;
			}
			if (g_pos >= scenario::duration()) {
				g_done = true;
			}
		}
	} else {
		if ((need & 1u) != 0u) {
			pack_half(0);
		}
		if ((need & 2u) != 0u) {
			pack_half(kHalfWords);
		}
		/* Ring index is always pos % kRingWords, so once we have filled the
		   whole ring from the tone's first sample it already holds exactly one
		   self-joining period. Stop filling and let the DMA free-run. */
		if (g_static_ok && !g_static &&
		    g_pos >= (g_static_start + kRingWords)) {
			g_static = true;
		}
	}

	if (g_done) {
		stop();
	}
}

bool running()
{
	return g_running;
}

uint32_t sample_pos()
{
	return g_pos;
}

bool sync_line()
{
	return HAL_GPIO_ReadPin(SYNC_GPIO_Port, SYNC_Pin) == GPIO_PIN_SET;
}

}
