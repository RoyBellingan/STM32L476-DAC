#include "analysis.hpp"

#include <cmath>
#include <numbers>

namespace ana {

void Basis::make(uint32_t n_samples)
{
	n = (n_samples > kNMax) ? kNMax : n_samples;
	const double w = 2.0 * std::numbers::pi / static_cast<double>(n);
	for (uint32_t i = 0; i < n; i++) {
		sin[i] = static_cast<float>(std::sin(w * static_cast<double>(i)));
		cos[i] = static_cast<float>(std::cos(w * static_cast<double>(i)));
	}
}

void Accum::reset(uint32_t n_samples)
{
	n = (n_samples > kNMax) ? kNMax : n_samples;
	for (uint32_t i = 0; i < kNMax; i++) {
		acc[i] = 0;
	}
	sumsq = 0;
	count = 0;
	clip  = 0;
	vmin  = 0xFFFFu;
	vmax  = 0;
}

void Accum::add_block(const uint16_t *src, uint32_t stride, uint32_t m,
                      uint32_t phase0)
{
	int32_t *dst = &acc[phase0];
	uint64_t sq  = 0;
	uint32_t lo  = vmin;
	uint32_t hi  = vmax;
	uint32_t cl  = 0;

	for (uint32_t i = 0; i < m; i++) {
		const uint32_t v = src[i * stride];
		dst[i] += static_cast<int32_t>(v);
		sq += static_cast<uint64_t>(v * v);
		if (v < lo) {
			lo = v;
		}
		if (v > hi) {
			hi = v;
		}
		if (v == 0u || v >= 4095u) {
			cl++;
		}
	}
	sumsq += sq;
	count += m;
	clip  += cl;
	vmin   = static_cast<uint16_t>(lo);
	vmax   = static_cast<uint16_t>(hi);
}

Result analyze(const Accum &a, const Basis &b)
{
	Result r{};
	r.vmin = a.vmin;
	r.vmax = a.vmax;
	r.clip = a.clip;
	const uint32_t n = a.n;
	if (n == 0u || n != b.n || a.count < n) {
		return r;
	}
	r.periods = a.count / n;
	const double inv_p = 1.0 / static_cast<double>(r.periods);

	int64_t sum = 0;
	for (uint32_t i = 0; i < n; i++) {
		sum += a.acc[i];
	}
	r.mean = static_cast<double>(sum) / static_cast<double>(a.count);

	/* E[v^2] - E[v]^2 in double: sumsq < 2^53 for any capture <= 500 periods. */
	double ac_ms = static_cast<double>(a.sumsq) / static_cast<double>(a.count)
	             - r.mean * r.mean;
	if (ac_ms < 0.0) {
		ac_ms = 0.0;
	}
	r.ac_rms = std::sqrt(ac_ms);

	/* DFT of the averaged period at bins 1..kHarm. X = A cos(ph), Y = A sin(ph)
	   for v = A sin(w i + ph): the same convention as Flux rms_signal.cpp. */
	double x1 = 0.0;
	double y1 = 0.0;
	double harm_ms = 0.0;   /* sum of (A_k^2 / 2), k = 2..kHarm */
	const uint32_t kmax = (kHarm < n / 2u) ? kHarm : (n / 2u - 1u);
	for (uint32_t k = 1; k <= kmax; k++) {
		double xs = 0.0;
		double yc = 0.0;
		uint32_t idx = 0;
		for (uint32_t i = 0; i < n; i++) {
			const double v = static_cast<double>(a.acc[i]) * inv_p - r.mean;
			xs += v * static_cast<double>(b.sin[idx]);
			yc += v * static_cast<double>(b.cos[idx]);
			idx += k;
			if (idx >= n) {
				idx -= n;
			}
		}
		xs *= 2.0 / static_cast<double>(n);
		yc *= 2.0 / static_cast<double>(n);
		r.h_pk[k] = std::sqrt(xs * xs + yc * yc);
		if (k == 1) {
			x1 = xs;
			y1 = yc;
			r.ph1 = std::atan2(yc, xs);
		} else {
			harm_ms += 0.5 * r.h_pk[k] * r.h_pk[k];
		}
	}

	const double a1 = r.h_pk[1];
	const double a1_ms = 0.5 * a1 * a1;
	r.h_rms = std::sqrt(a1_ms + harm_ms);
	const double rest_ms = (ac_ms > a1_ms) ? (ac_ms - a1_ms) : 0.0;
	double noise_ms = rest_ms - harm_ms;
	if (noise_ms < 0.0) {
		noise_ms = 0.0;
	}
	r.noise_rms = std::sqrt(noise_ms);

	if (a1 > 1e-6) {
		r.thd  = std::sqrt(harm_ms / a1_ms);
		r.thdn = std::sqrt(rest_ms / a1_ms);
		if (rest_ms > 0.0) {
			r.sinad_db = 10.0 * std::log10(a1_ms / rest_ms);
			r.enob = (r.sinad_db - 1.76 + 20.0 * std::log10(2048.0 / a1)) / 6.02;
		}
	}

	/* How far the averaged period departs from mean + fundamental. */
	double worst = 0.0;
	for (uint32_t i = 0; i < n; i++) {
		const double fit = r.mean + x1 * static_cast<double>(b.sin[i])
		                 + y1 * static_cast<double>(b.cos[i]);
		const double d = std::fabs(static_cast<double>(a.acc[i]) * inv_p - fit);
		if (d > worst) {
			worst = d;
		}
	}
	r.res_max = worst;

	if (r.ac_rms > 0.0) {
		const double up = static_cast<double>(a.vmax) - r.mean;
		const double dn = r.mean - static_cast<double>(a.vmin);
		r.crest = ((up > dn) ? up : dn) / r.ac_rms;
	}
	return r;
}

}
