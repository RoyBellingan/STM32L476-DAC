#include "analysis.hpp"

#include <cmath>
#include <numbers>

namespace ana {
namespace {

int64_t acc_sum(const Accum &a)
{
	int64_t sum = 0;
	for (uint32_t i = 0; i < a.n; i++) {
		sum += a.acc[i];
	}
	return sum;
}

/* The table value in Q30. The float value has at most 24 significant bits and
   |x| <= 1, so the result is exact: no table precision is lost. */
inline int32_t q30(float x)
{
	return static_cast<int32_t>(x * 1073741824.0f);
}

}

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

double mean(const Accum &a)
{
	if (a.n == 0u || a.count < a.n) {
		return 0.0;
	}
	return static_cast<double>(acc_sum(a)) / static_cast<double>(a.count);
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

	const int64_t sum = acc_sum(a);
	r.mean = static_cast<double>(sum) / static_cast<double>(a.count);
	/* The sample loops use acc[i] - m. m is the DC of the accumulator, to the
	   nearest integer. The codes are >= 0, so sum >= 0. */
	const int32_t m = static_cast<int32_t>((sum + n / 2u) / n);

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
	/* Integer sums. The worst case is |acc - m| <= 4095 p, table <= 2^30,
	   n <= 1000 terms: 4.4e15 p, which fits an int64 up to p = 2000.
	   `read` stops at 500. */
	const double dft_scale = 2.0 / static_cast<double>(n) * inv_p / 1073741824.0;
	for (uint32_t k = 1; k <= kmax; k++) {
		int64_t sx = 0;
		int64_t sy = 0;
		uint32_t idx = 0;
		for (uint32_t i = 0; i < n; i++) {
			const int32_t v = a.acc[i] - m;
			sx += static_cast<int64_t>(v) * q30(b.sin[idx]);
			sy += static_cast<int64_t>(v) * q30(b.cos[idx]);
			idx += k;
			if (idx >= n) {
				idx -= n;
			}
		}
		const double xs = static_cast<double>(sx) * dft_scale;
		const double yc = static_cast<double>(sy) * dft_scale;
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
	const float inv_pf = static_cast<float>(inv_p);
	const float x1f = static_cast<float>(x1);
	const float y1f = static_cast<float>(y1);
	const float off = static_cast<float>(r.mean - static_cast<double>(m) * inv_p);
	float worst = 0.0f;
	for (uint32_t i = 0; i < n; i++) {
		const float fit = off + x1f * b.sin[i] + y1f * b.cos[i];
		const float d = std::fabs(static_cast<float>(a.acc[i] - m) * inv_pf - fit);
		if (d > worst) {
			worst = d;
		}
	}
	r.res_max = static_cast<double>(worst);

	if (r.ac_rms > 0.0) {
		const double up = static_cast<double>(a.vmax) - r.mean;
		const double dn = r.mean - static_cast<double>(a.vmin);
		r.crest = ((up > dn) ? up : dn) / r.ac_rms;
	}
	return r;
}

}
