#ifndef SINE_OUT_HPP
#define SINE_OUT_HPP

#include "analysis.hpp"

#include <cstdint>

/* The 50 Hz sine on PA4 (DAC1 OUT1), paced by TIM6.

   The DMA ring holds two periods. Each half is refilled with one whole period
   in the DMA interrupt itself (the BatTester lesson: printing on the UART must
   never starve the fill). An amplitude change is applied over one period, as a
   ramp, never as a step. The DAC sits at mid-scale (amplitude 0) from boot. */
namespace sine_out {

void init();
bool start();                 /* arms the DAC and its DMA; TIM6 starts it */

void  set_amp_codes(float peak_codes);
float amp_codes();            /* the target */

/* Centre of the sine, in codes (default 2048). The caller keeps
   mid - amp >= 248 and mid + amp <= 3848 (0.2 V .. VDDA - 0.2 V). */
void  set_mid_codes(float mid_codes);
float mid_codes();

/* True once the fill has applied the current target. from_period is then the
   first period that plays the target with no ramp. Period p covers DAC
   samples p*1000 .. p*1000+999, counted from the first TIM6 trigger. */
bool settled(uint32_t &from_period);

/* A half refilled too late: the DMA had already wrapped into it. */
uint32_t underruns();

/* New period length n samples (even, 200 .. ana::kNMax). Call only with
   TIM6 stopped and the amplitude at 0; the period counter restarts at 0. */
bool reconfigure(uint32_t n);

/* Shape: sin + up to kMaxHarm harmonics, r_k sin(k w i + deg_k). `amp` stays
   the fundamental's peak. A change cross-fades over one period. The DFT
   basis stays a pure sine, so the reference measures what really flows. */
inline constexpr uint32_t kMaxHarm = 4;
struct Harm {
	uint32_t k;        /* 2 .. 20 */
	float    ratio;    /* of the fundamental's peak */
	float    deg;      /* 180 with k = 3: a peaky, rectifier-like shape */
};
float preview_peak(const Harm *h, uint32_t count);   /* max |shape|, fundamental peak 1 */
bool  set_shape(const Harm *h, uint32_t count);
uint32_t harm_count();
const Harm &harm(uint32_t j);
float wave_peak();            /* of the target shape */
uint32_t period();
const ana::Basis &basis();

}

#endif
