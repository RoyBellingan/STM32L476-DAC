/* Host test for Loop/Src/analysis.cpp.

     g++ -std=c++23 -O2 -Wall -Wextra -ILoop/Inc Loop/test/analysis_test.cpp \
         Loop/Src/analysis.cpp -o build/analysis_test && build/analysis_test

   A synthetic capture with a known DC, fundamental, phase, 3rd harmonic and
   noise goes through the same add_block() path as the ISR. */
#include "analysis.hpp"

#include <cmath>
#include <cstdio>
#include <numbers>
#include <random>
#include <vector>

namespace {

int g_fail = 0;

void near(const char *what, double got, double want, double tol)
{
	const bool ok = std::fabs(got - want) <= tol;
	std::printf("%s %-28s got %.6f want %.6f tol %.1e\n", ok ? "ok  " : "FAIL", what, got,
	            want, tol);
	if (!ok) {
		g_fail++;
	}
}

ana::Accum g_acc;
ana::Basis g_basis;

/* Feed `periods` periods of n samples in n/2-sample blocks, interleaved like
   the DMA ring (3 channels, this one at rank 1). `other` is a tone at
   other_ratio x the fundamental frequency, not coherent with the period. */
void feed(double dc, double a1, double ph, double a3, double noise_rms, uint32_t periods,
          unsigned seed, uint32_t n = 1000, double other = 0.0, double other_ratio = 1.0)
{
	std::mt19937 rng(seed);
	std::normal_distribution<double> nd(0.0, noise_rms);
	const double w = 2.0 * std::numbers::pi / n;
	const uint32_t h = n / 2;
	g_acc.reset(n);
	g_basis.make(n);
	std::vector<uint16_t> ring(h * 3);
	for (uint32_t p = 0; p < periods; p++) {
		for (uint32_t half = 0; half < 2; half++) {
			for (uint32_t i = 0; i < h; i++) {
				const uint32_t j = half * h + i;
				const double t = static_cast<double>(p * n + j);
				double v = dc + a1 * std::sin(w * j + ph) + a3 * std::sin(3.0 * w * j)
				         + other * std::sin(w * other_ratio * t)
				         + (noise_rms > 0 ? nd(rng) : 0.0);
				long c = std::lround(v);
				if (c < 0) c = 0;
				if (c > 4095) c = 4095;
				ring[i * 3 + 1] = static_cast<uint16_t>(c);
			}
			g_acc.add_block(&ring[1], 3, h, half * h);
		}
	}
}

}

int main()
{
	/* 1. Clean sine, 1000 codes peak, phase 30 deg, 1 % third harmonic. */
	feed(2048.0, 1000.0, std::numbers::pi / 6, 10.0, 0.0, 10, 1);
	ana::Result r = ana::analyze(g_acc, g_basis);
	near("periods", r.periods, 10, 0);
	near("mean", r.mean, 2048.0, 0.01);
	near("a1 peak", r.h_pk[1], 1000.0, 0.05);
	near("phase deg", r.ph1 * 180 / std::numbers::pi, 30.0, 0.01);
	near("h3 peak", r.h_pk[3], 10.0, 0.05);
	near("thd", r.thd, 0.01, 2e-4);
	near("ac rms", r.ac_rms, std::sqrt((1000.0 * 1000 + 100) / 2), 0.05);
	near("crest", r.crest, 1000.0 / std::sqrt((1000.0 * 1000 + 100) / 2), 0.01);

	/* 2. Noise: 2 codes rms. Raw noise must come back; the averaged period
	      must not mistake it for harmonics. */
	feed(2048.0, 1000.0, 0.0, 0.0, 2.0, 50, 2);
	r = ana::analyze(g_acc, g_basis);
	near("noise rms", r.noise_rms, std::sqrt(4.0 + 1.0 / 12), 0.05);
	near("thd (noise only)", r.thd, 0.0, 2e-4);
	near("thdn", r.thdn, std::sqrt(4.0 + 1.0 / 12) / (1000.0 / std::sqrt(2.0)), 1e-4);
	/* SINAD with 1000 pk and 2.02 rms noise: 20log10(707.1/2.02) = 50.9 dB. */
	near("sinad dB", r.sinad_db, 20 * std::log10(707.107 / std::sqrt(4.0 + 1.0 / 12)), 0.1);

	/* 3. Pure quantisation of a 12-bit full-scale-ish sine: ENOB near 12. */
	feed(2048.0, 2040.0, 0.3, 0.0, 0.0, 10, 3);
	r = ana::analyze(g_acc, g_basis);
	near("enob (quantised only)", r.enob, 12.0, 0.3);
	near("res max (codes)", r.res_max, 0.5, 0.01);

	/* 4. Period of 800 samples (62.5 Hz at 50 kSPS), phase -45 deg. */
	feed(1660.0, 900.0, -std::numbers::pi / 4, 0.0, 0.0, 10, 4, 800);
	r = ana::analyze(g_acc, g_basis);
	near("n=800 periods", r.periods, 10, 0);
	near("n=800 a1 peak", r.h_pk[1], 900.0, 0.1);   /* coherent rounding: -0.05 code */
	near("n=800 phase deg", r.ph1 * 180 / std::numbers::pi, -45.0, 0.01);
	near("n=800 h_rms", r.h_rms, 900.0 / std::sqrt(2.0), 0.05);

	/* 5. A 50 Hz tone (0.8 x a 62.5 Hz period) of 100 codes on top of the
	      source: ac_rms includes it, h_rms must not (50 periods). */
	feed(2048.0, 500.0, 0.0, 0.0, 0.0, 50, 5, 800, 100.0, 0.8);
	r = ana::analyze(g_acc, g_basis);
	near("tone: ac_rms", r.ac_rms, std::sqrt(500.0 * 500 / 2 + 100.0 * 100 / 2), 0.2);
	near("tone: h_rms (rejects it)", r.h_rms, 500.0 / std::sqrt(2.0), 0.5);

	std::printf("%s (%d failed)\n", g_fail ? "FAILED" : "PASSED", g_fail);
	return g_fail ? 1 : 0;
}
