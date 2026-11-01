#ifndef ADC_IN_HPP
#define ADC_IN_HPP

#include "analysis.hpp"

#include <cstdint>

/* ADC1, triggered by TIM6_TRGO (EXTSEL 13), the same trigger as the DAC. Each
   trigger converts one scan of kNch channels; DMA1 channel 1 moves the scans
   into a circular ring of one period (1000 scans). The ring halves are handled
   in the DMA interrupt: VDDA from VREFINT always, and the capture
   accumulators while a capture runs.

   Programmed at register level: the project's HAL is 1.13.6 and has no ADC
   driver; the one on disk (Cube FW L4 1.18.2) is a different version. */
namespace adc_in {

inline constexpr uint32_t kNch = 5;

struct ChanInfo {
	const char *name;
	const char *pins;
	uint8_t     channel;
	bool        diff;
	float       hold_us;   /* end of the sampling window after the trigger */
};

const ChanInfo &chan(uint32_t c);
inline constexpr uint32_t kVrefIdx = 4;
inline constexpr uint32_t kPgaOutIdx = 1;
inline constexpr uint32_t kPgaInIdx = 2;

void init();                       /* configures and arms; TIM6 starts it */

/* New period length n (even, <= ana::kNMax). Call only with TIM6 stopped;
   the scan counter restarts at 0, in step with sine_out::reconfigure(). */
void reconfigure(uint32_t n);
uint32_t period_scans();

uint32_t current_period();         /* period of the last block handled */

/* Accumulate `periods` whole periods, starting at period start_period. The
   accumulators are reset here, so call it only when not busy(). */
bool begin(uint32_t start_period, uint32_t periods);
bool busy();
bool done();
bool capture_bad();                /* a half was lost during the capture */
void release();                    /* back to idle after done() */
uint32_t capture_start();
uint32_t capture_periods();

const ana::Accum &accum(uint32_t c);
const uint16_t *raw_first_period();   /* n scans x kNch, from the last capture */

float vdda();                      /* V, from the latest half (500 VREFINT samples) */
float vdda_from_mean(double vrefint_mean_codes);
uint16_t vrefint_cal();

uint32_t lost_halves();
uint32_t overruns();
uint32_t dma_errors();

}

#endif
