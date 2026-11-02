#ifndef ANALYSIS_HPP
#define ANALYSIS_HPP

/* Measurement of one ADC channel over whole periods. No HAL: this file also
   builds on the host (Loop/test/analysis_test.cpp).

   The ADC is paced by the same timer as the DAC, so sample j of a period
   always falls at the same point of the sine. The ISR therefore adds each
   sample into acc[phase] (synchronous averaging): after p periods, acc / p is
   one period with the incoherent part (noise, mains hum at another frequency)
   reduced by sqrt(p), and the harmonics are exact DFT bins. The raw sum of
   squares is kept as well, so the total RMS still includes all the noise.

   The period length n (samples) is set at run time (`freq`), up to kNMax. */

#include <cstdint>

namespace ana {

inline constexpr uint32_t kNMax = 1000;   /* samples per period, maximum */
inline constexpr uint32_t kHarm = 20;     /* harmonics used for THD and noise */

/* sin and cos of one period of n samples. */
struct Basis {
	uint32_t n;
	float    sin[kNMax];
	float    cos[kNMax];

	void make(uint32_t n_samples);
};

struct Accum {
	uint32_t n;        /* period length the capture uses */
	int32_t  acc[kNMax];
	uint64_t sumsq;
	uint32_t count;
	uint32_t clip;     /* samples at code 0 or 4095 */
	uint16_t vmin;
	uint16_t vmax;

	void reset(uint32_t n_samples);

	/* m samples, src[0], src[stride], ..., for phases phase0 .. phase0+m-1.
	   Runs in the ADC DMA interrupt: keep it short. */
	void add_block(const uint16_t *src, uint32_t stride, uint32_t m,
	               uint32_t phase0);
};

struct Result {
	uint32_t periods;
	double   mean;              /* codes */
	double   ac_rms;            /* codes, raw samples, DC removed: all of it */
	double   h_rms;             /* codes, sqrt(sum of h_pk^2/2), k = 1..kHarm: coherent part only */
	double   h_pk[kHarm + 1];   /* codes, peak; h_pk[1] is the fundamental */
	double   ph1;               /* rad, fundamental as h_pk[1] * sin(w*i + ph1) */
	double   thd;               /* ratio, harmonics 2..kHarm of the averaged period */
	double   thdn;              /* ratio, everything but the fundamental, raw samples */
	double   sinad_db;
	double   enob;              /* referred to full scale (2048 codes peak) */
	double   noise_rms;         /* codes, raw minus DC and harmonics 1..kHarm */
	double   res_max;           /* codes, max |averaged period - mean - fundamental| */
	double   crest;             /* max |sample - mean| / ac_rms; a sine is 1.414 */
	uint16_t vmin;
	uint16_t vmax;
	uint32_t clip;
};

/* Mean in codes, for a channel that needs nothing else (VREFINT). 0 until the
   accumulator holds one whole period. */
double mean(const Accum &a);

/* a.n must equal b.n. Runs on the Cortex-M4 too, where the FPU has single
   precision only and each double operation runs in software. So the sample
   loops use int64 and float. Capture limit: 2000 periods (see analysis.cpp). */
Result analyze(const Accum &a, const Basis &b);

}

#endif
