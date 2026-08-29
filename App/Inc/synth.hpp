#ifndef SYNTH_HPP
#define SYNTH_HPP

#include <cstdint>

namespace synth {

/* Nominal, and used to lay scenarios out in samples. The hardware does NOT run
   at this rate: MSI locked to the X2 crystal gives 999424 Hz exactly (see the
   `clk` command). Sample counts below are therefore exact and the seconds are
   not -- a "10 s" scenario emits 10'000'000 samples and takes 10.00576 s.

   Do not "fix" this by setting kRateHz to 999424. Scenario boundaries would
   stop landing on round sample counts, kPhaseInc would no longer put a whole
   number of tone cycles in one ring, and the static-ring path in synth.cpp
   would break. The host tools take the true rate from the manifest instead. */
inline constexpr uint32_t kRateHz     = 1'000'000;

/* What the DAC actually clocks at, for anything converting samples to seconds
   or bins to hertz. Measured 2026-08-29: 999430 Hz, 6 ppm from this. */
inline constexpr uint32_t kTrueRateHz = 999'424;
inline constexpr uint32_t kRingWords  = 4096;
inline constexpr uint32_t kHalfWords  = 2048;
inline constexpr int32_t  kDacMid     = 2048;
inline constexpr int32_t  kDacPeak    = 2047;
inline constexpr int32_t  kTonePeak   = 1800;
inline constexpr uint32_t kToneHz     = 107'400;
/* PLAN section 6: every scenario opens with a noise-only lead-in so BatV1's
   detector can seed its floor (kFloorSeedFrames = 16 frames = 16.4 ms). 2 s is
   what the plan asks for; 488 rings is 1.998848 s, which is 2 s to within
   0.06 % and keeps every event start ring-aligned. That alignment is what lets
   a plain-tone scenario drop into the static ring once its tone begins. */
inline constexpr uint32_t kPreamble   = 488u * kRingWords;   /* 1'998'848 */

void init();
void set_level_q12(int32_t gain_q12);
void set_noise_q15(int32_t noise_q15);   /* runs for the whole scenario */
void set_floor_q15(int32_t floor_q15);   /* runs during the preamble only */
int32_t level_q12();
int32_t noise_q15();
int32_t floor_q15();

bool start();
void stop();
void poll();

bool running();
uint32_t sample_pos();
bool sync_line();

/* Half-buffer fills that the main loop was too late to serve. The DMA replayed
   stale ring content for those 2048 samples, and pos lost its parity with the
   ring offset, so everything after the first one is wrong. Cleared by start().
   A run that reports anything but 0 is not evidence of anything. */
uint32_t underruns();

}

#endif
