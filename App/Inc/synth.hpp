#ifndef SYNTH_HPP
#define SYNTH_HPP

#include <cstdint>

namespace synth {

inline constexpr uint32_t kRateHz     = 1'000'000;
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

}

#endif
