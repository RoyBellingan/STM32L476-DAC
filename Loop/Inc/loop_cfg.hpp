#ifndef LOOP_CFG_HPP
#define LOOP_CFG_HPP

#include <cstdint>

/* Timing of the loop image. One timer (TIM6) paces the DAC and the ADC, so
   every ADC scan has a fixed position against the DAC sample: scan j sees the
   DAC value of trigger j. The sine is one period of n DAC samples (`freq`),
   so every capture is a whole number of periods, with no leakage and no
   window cut (digitalSpine todo/fluxStationLoopSource.md §5.2). */
namespace loopcfg {

/* MSI range 6 locked to the X2 crystal: 122 x 32768 Hz x 40 / 2. Only true
   when `clk` reports lse=1; free-running MSI was measured -6329 ppm. */
inline constexpr uint32_t kSysclkHz         = 79'953'920u;
inline constexpr uint32_t kTimDiv           = 1599u;    /* TIM6 ARR = 1598 */
inline constexpr uint32_t kSamplesDefault  = 1000u;   /* 50.00245 Hz */
inline constexpr uint32_t kSamplesMin      = 200u;    /* 250 Hz */

inline constexpr double kFsHz = static_cast<double>(kSysclkHz) / kTimDiv;   /* 50 002.45 */
inline double f0_hz(uint32_t n) { return kFsHz / static_cast<double>(n); }

/* Flux boards capture 231 samples at 2890 Hz = 79.9 ms (Flux ADCParam.h):
   whole cycles at 50 Hz (4) and at 62.5 Hz (5, n = 800), not at 60 Hz (4.8). */

/* USART2, ST-LINK VCP. BRR = 174 gives 459 505 baud (-0.28 %). Change the PC
   side too (terminal, tools/loop_snap.py). */
inline constexpr uint32_t kUartBaud = 460'800u;

/* DAC: buffer on, 0.2 V .. VREF+ - 0.2 V (DS10198 VDAC_OUT). The mid code is
   1.65 V at VDDA 3.3 V, so 1790 codes (1.44 V) is the largest peak that stays
   inside that range. */
inline constexpr int32_t  kDacMid      = 2048;
inline constexpr float    kDacMaxPeak  = 1790.0f;

/* ADC capture limits for `read`. */
inline constexpr uint32_t kReadDefaultPeriods = 10u;
inline constexpr uint32_t kReadMaxPeriods     = 500u;   /* 10 s; acc stays in int32 */
/* Whole periods between the end of the DAC ramp (or the `read` command) and
   the first captured period, so that the amplitude can stabilize. */
inline constexpr uint32_t kReadSettlePeriods  = 2u;

}

#endif
