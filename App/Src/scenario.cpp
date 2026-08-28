#include "scenario.hpp"

#include "synth.hpp"
#include "wave_table.h"

#include <array>
#include <cstddef>
#include <cstring>

namespace scenario {
namespace {

constexpr size_t kMaxEvents = 64;
constexpr int16_t kQ12One = 4096;
constexpr uint32_t kEnvEdge = 1000;
/* 440 tone cycles in exactly 4096 samples, so the phase returns to where it
   started after one DMA ring. That makes the ring self-contained: synth can
   prefill it once and let the DMA run circular forever with no refill and no
   possible boundary glitch. 440 * 2^32 / 4096 = 440 * 2^20.

   At 1 MSps this is 107421.875 Hz. The old value gave 107326.65 Hz, so this
   is also closer to the 107.4 kHz target: +204 ppm instead of -684 ppm. */
constexpr uint32_t kPhaseInc = 461373440u;

constexpr std::array<int16_t, 256> kSine = {
	     0,    804,   1608,   2410,   3212,   4011,   4808,   5602,
	  6393,   7179,   7962,   8739,   9512,  10278,  11039,  11793,
	 12539,  13279,  14010,  14732,  15446,  16151,  16846,  17530,
	 18204,  18868,  19519,  20159,  20787,  21403,  22005,  22594,
	 23170,  23731,  24279,  24811,  25329,  25832,  26319,  26790,
	 27245,  27683,  28105,  28510,  28898,  29268,  29621,  29956,
	 30273,  30571,  30852,  31113,  31356,  31580,  31785,  31971,
	 32137,  32285,  32412,  32521,  32609,  32678,  32728,  32757,
	 32767,  32757,  32728,  32678,  32609,  32521,  32412,  32285,
	 32137,  31971,  31785,  31580,  31356,  31113,  30852,  30571,
	 30273,  29956,  29621,  29268,  28898,  28510,  28105,  27683,
	 27245,  26790,  26319,  25832,  25329,  24811,  24279,  23731,
	 23170,  22594,  22005,  21403,  20787,  20159,  19519,  18868,
	 18204,  17530,  16846,  16151,  15446,  14732,  14010,  13279,
	 12539,  11793,  11039,  10278,   9512,   8739,   7962,   7179,
	  6393,   5602,   4808,   4011,   3212,   2410,   1608,    804,
	     0,   -804,  -1608,  -2410,  -3212,  -4011,  -4808,  -5602,
	 -6393,  -7179,  -7962,  -8739,  -9512, -10278, -11039, -11793,
	-12539, -13279, -14010, -14732, -15446, -16151, -16846, -17530,
	-18204, -18868, -19519, -20159, -20787, -21403, -22005, -22594,
	-23170, -23731, -24279, -24811, -25329, -25832, -26319, -26790,
	-27245, -27683, -28105, -28510, -28898, -29268, -29621, -29956,
	-30273, -30571, -30852, -31113, -31356, -31580, -31785, -31971,
	-32137, -32285, -32412, -32521, -32609, -32678, -32728, -32757,
	-32767, -32757, -32728, -32678, -32609, -32521, -32412, -32285,
	-32137, -31971, -31785, -31580, -31356, -31113, -30852, -30571,
	-30273, -29956, -29621, -29268, -28898, -28510, -28105, -27683,
	-27245, -26790, -26319, -25832, -25329, -24811, -24279, -23731,
	-23170, -22594, -22005, -21403, -20787, -20159, -19519, -18868,
	-18204, -17530, -16846, -16151, -15446, -14732, -14010, -13279,
	-12539, -11793, -11039, -10278,  -9512,  -8739,  -7962,  -7179,
	 -6393,  -5602,  -4808,  -4011,  -3212,  -2410,  -1608,   -804,
};

struct RefCall {
	uint16_t start_ms;
	uint16_t dur_ms;
	uint16_t peak;
};

constexpr std::array<RefCall, 17> kRefCalls = {{
	{ 297, 35, 3233 }, { 378, 36, 2717 }, { 464, 16, 1230 }, { 549, 27, 2801 },
	{ 713,  1, 1552 }, { 718, 24, 3587 }, { 803, 20, 3455 }, { 885, 25, 3150 },
	{ 969, 11, 1363 }, {1003,  2, 1145 }, {1016,  1, 1751 }, {1047, 18, 2656 },
	{1070, 12, 1256 }, {1094,  1, 1455 }, {1129,  2,  969 }, {1138,  2, 1013 },
	{1144,  1, 2067 },
}};

constexpr std::array<Info, static_cast<size_t>(Id::Count)> kCatalog = {{
	{"noise",     "a", "60 s noise only"},
	{"ref",       "b", "17 horseshoe calls"},
	{"tone_1s",   "c", "1 s 107 kHz"},
	{"train",     "d", "35 ms calls, 5 ms gap, 2 s"},
	{"tone_10s",  "e", "10 s 107 kHz, 5 cuts then backoff"},
	{"levels",    "f", "35 ms calls, 0 to -42 dB"},
	{"ref_noise", "g", "horseshoe plus noise"},
	{"gap_80",    "h", "two 30 ms, 80 ms gap"},
	{"gap_15",    "i", "two 30 ms, 15 ms gap"},
	{"gap_6s",    "j", "two 30 ms, 6 s gap"},
	{"step",      "",  "full-scale DAC step"},
	{"tone",      "",  "10 s 107 kHz (default)"},
}};

std::array<Event, kMaxEvents> g_ev{};
uint32_t g_nev;
uint32_t g_duration;
Id g_id = Id::Tone;
uint32_t g_evt_idx;

int16_t db_to_q12(int db)
{
	int16_t g = kQ12One;
	int n = -db / 6;
	while (n-- > 0) {
		g = static_cast<int16_t>((g * 2896) >> 12);
	}
	return g;
}

void add_event(uint32_t start, uint32_t length, uint16_t seg,
               int16_t gain_q12, uint16_t flags)
{
	if (g_nev >= kMaxEvents) {
		return;
	}
	g_ev[g_nev] = Event{start, length, seg, gain_q12, flags};
	g_nev++;
}

uint32_t env_q12(uint32_t local, uint32_t length)
{
	if (length == 0) {
		return kQ12One;
	}
	uint32_t edge = kEnvEdge;
	if (edge * 2u > length) {
		edge = length / 2u;
	}
	if (edge == 0) {
		return kQ12One;
	}
	auto ramp = [edge](uint32_t t) -> uint32_t {
		const uint32_t idx = 64u + (t * 128u) / edge;
		const int32_t c = kSine[idx & 255u];
		return static_cast<uint32_t>((kQ12One - ((c * kQ12One) >> 15)) / 2);
	};
	if (local < edge) {
		return ramp(local);
	}
	if (local + edge >= length) {
		return ramp(length - 1u - local);
	}
	return kQ12One;
}

void finish_duration(uint32_t content_end)
{
	g_duration = content_end + (synth::kRateHz / 10u);
}

void build_a()
{
	g_duration = synth::kPreamble + 60u * synth::kRateHz;
}

void build_ref_pass()
{
	if (wave_seg_count < 17u) {
		g_duration = synth::kPreamble;
		return;
	}
	uint32_t end = synth::kPreamble;
	for (uint32_t i = 0; i < 17u; i++) {
		const uint32_t start =
			synth::kPreamble + static_cast<uint32_t>(kRefCalls[i].start_ms) * 1000u;
		const uint32_t len = wave_seg[i].length;
		add_event(start, len, static_cast<uint16_t>(i), kQ12One, 0);
		if (start + len > end) {
			end = start + len;
		}
	}
	finish_duration(end);
}

void build_c()
{
	add_event(synth::kPreamble, synth::kRateHz, kSegTone, kQ12One, 0);
	finish_duration(synth::kPreamble + synth::kRateHz);
}

void build_d()
{
	constexpr uint32_t call = 35000;
	constexpr uint32_t gap = 5000;
	uint32_t t = synth::kPreamble;
	const uint32_t stop = synth::kPreamble + 2u * synth::kRateHz;
	while (t + call <= stop && g_nev < kMaxEvents) {
		add_event(t, call, kSegTone, kQ12One, kEvtEnvelope);
		t += call + gap;
	}
	finish_duration(stop);
}

void build_e()
{
	add_event(synth::kPreamble, 10u * synth::kRateHz, kSegTone, kQ12One, 0);
	finish_duration(synth::kPreamble + 10u * synth::kRateHz);
}

void build_f()
{
	uint32_t t = synth::kPreamble;
	constexpr uint32_t call = 35000;
	constexpr uint32_t gap = 200000;
	for (int db = 0; db >= -42; db -= 6) {
		add_event(t, call, kSegTone, db_to_q12(db), kEvtEnvelope);
		t += call + gap;
	}
	finish_duration(t);
}

void build_pair(uint32_t gap)
{
	constexpr uint32_t call = 30000;
	add_event(synth::kPreamble, call, kSegTone, kQ12One, kEvtEnvelope);
	add_event(synth::kPreamble + call + gap, call, kSegTone, kQ12One, kEvtEnvelope);
	finish_duration(synth::kPreamble + call + gap + call);
}

void build_step()
{
	add_event(synth::kPreamble, synth::kRateHz, kSegStep, kQ12One, 0);
	finish_duration(synth::kPreamble + synth::kRateHz);
}

void build_tone()
{
	add_event(synth::kPreamble, 10u * synth::kRateHz, kSegTone, kQ12One, 0);
	finish_duration(synth::kPreamble + 10u * synth::kRateHz);
}

void rebuild()
{
	g_nev = 0;
	g_duration = synth::kPreamble;
	g_evt_idx = 0;
	g_ev.fill(Event{});

	switch (g_id) {
	case Id::A:    build_a(); break;
	case Id::B:    build_ref_pass(); break;
	case Id::C:    build_c(); break;
	case Id::D:    build_d(); break;
	case Id::E:    build_e(); break;
	case Id::F:    build_f(); break;
	case Id::G:    build_ref_pass(); break;
	case Id::H:    build_pair(80000); break;
	case Id::I:    build_pair(15000); break;
	case Id::J:    build_pair(6u * synth::kRateHz); break;
	case Id::Step: build_step(); break;
	case Id::Tone: build_tone(); break;
	default:       build_tone(); break;
	}
}

}

void init()
{
	g_id = Id::Tone;
	rebuild();
}

bool select(const char *n)
{
	if (n == nullptr) {
		return false;
	}
	for (size_t i = 0; i < kCatalog.size(); i++) {
		const Info &e = kCatalog[i];
		if (std::strcmp(n, e.name) == 0 ||
		    (e.alias[0] != '\0' && std::strcmp(n, e.alias) == 0) ||
		    (static_cast<Id>(i) == Id::E && std::strcmp(n, "tone_2s") == 0)) {
			g_id = static_cast<Id>(i);
			rebuild();
			return true;
		}
	}
	return false;
}

const char *name()
{
	return kCatalog[static_cast<size_t>(g_id)].name;
}

std::span<const Info> catalog()
{
	return kCatalog;
}

Id id()
{
	return g_id;
}

bool needs_table()
{
	return g_id == Id::B || g_id == Id::G;
}

bool table_ready()
{
	return wave_seg_count >= 17u;
}

std::span<const Event> events()
{
	return std::span<const Event>(g_ev.data(), g_nev);
}

uint32_t duration()
{
	return g_duration;
}

void rewind()
{
	g_evt_idx = 0;
}

namespace {

/* The emitters below must produce bit-identical results to the old
   per-sample path: compute the raw segment value first, then apply the event
   gain. Do not fold the gain into the amplitude -- the truncation differs. */

void emit_silence(int32_t *dst, uint32_t n)
{
	std::memset(dst, 0, static_cast<size_t>(n) * sizeof(int32_t));
}

void emit_tone(int32_t *dst, uint32_t n, uint32_t local, uint32_t length,
               int32_t gain, bool env)
{
	uint32_t phase = local * kPhaseInc;
	if (!env && gain == kQ12One) {
		for (uint32_t i = 0; i < n; i++) {
			dst[i] = (synth::kTonePeak * kSine[phase >> 24]) >> 15;
			phase += kPhaseInc;
		}
		return;
	}
	for (uint32_t i = 0; i < n; i++) {
		int32_t amp = synth::kTonePeak;
		if (env) {
			amp = (amp * static_cast<int32_t>(env_q12(local + i, length))) >> 12;
		}
		const int32_t s = (amp * kSine[phase >> 24]) >> 15;
		dst[i] = (s * gain) >> 12;
		phase += kPhaseInc;
	}
}

void emit_step(int32_t *dst, uint32_t n, uint32_t local, int32_t gain)
{
	const int32_t hi = (synth::kDacPeak * gain) >> 12;
	for (uint32_t i = 0; i < n; i++) {
		dst[i] = ((local + i) >= 1000u) ? hi : 0;
	}
}

void emit_table(int32_t *dst, uint32_t n, uint16_t id, uint32_t local,
                int32_t gain)
{
	if (id >= wave_seg_count) {
		emit_silence(dst, n);
		return;
	}
	const wave_seg_t &seg = wave_seg[id];
	uint32_t avail = (local < seg.length) ? (seg.length - local) : 0u;
	if (avail > n) {
		avail = n;
	}
	const int16_t *src = &wave_table[seg.offset + local];
	if (gain == kQ12One) {
		for (uint32_t i = 0; i < avail; i++) {
			dst[i] = src[i];
		}
	} else {
		for (uint32_t i = 0; i < avail; i++) {
			dst[i] = (static_cast<int32_t>(src[i]) * gain) >> 12;
		}
	}
	if (avail < n) {
		emit_silence(dst + avail, n - avail);
	}
}

void emit_event(int32_t *dst, uint32_t n, const Event &e, uint32_t local)
{
	const int32_t gain = e.gain_q12;
	if (e.segment_id == kSegSilence) {
		emit_silence(dst, n);
	} else if (e.segment_id == kSegTone) {
		emit_tone(dst, n, local, e.length, gain,
		          (e.flags & kEvtEnvelope) != 0);
	} else if (e.segment_id == kSegStep) {
		emit_step(dst, n, local, gain);
	} else {
		emit_table(dst, n, e.segment_id, local, gain);
	}
}

}

void fill(int32_t *dst, uint32_t pos, uint32_t n)
{
	uint32_t done = 0;

	while (done < n) {
		uint32_t run = n - done;

		if (g_nev == 0) {
			emit_silence(dst + done, run);
			return;
		}

		if (pos < g_ev[0].start_sample) {
			const uint32_t to_first = g_ev[0].start_sample - pos;
			if (to_first < run) {
				run = to_first;
			}
			emit_silence(dst + done, run);
			done += run;
			pos += run;
			continue;
		}

		while ((g_evt_idx + 1u) < g_nev &&
		       pos >= g_ev[g_evt_idx + 1u].start_sample) {
			g_evt_idx++;
		}

		const Event &e = g_ev[g_evt_idx];

		/* Events may overlap. Never run past the next one's start. */
		if ((g_evt_idx + 1u) < g_nev) {
			const uint32_t to_next = g_ev[g_evt_idx + 1u].start_sample - pos;
			if (to_next < run) {
				run = to_next;
			}
		}

		const uint32_t local = pos - e.start_sample;
		if (e.length != 0u && local >= e.length) {
			emit_silence(dst + done, run);
			done += run;
			pos += run;
			continue;
		}
		if (e.length != 0u && (e.length - local) < run) {
			run = e.length - local;
		}

		emit_event(dst + done, run, e, local);
		done += run;
		pos += run;
	}
}

int32_t sample(uint32_t pos)
{
	int32_t s = 0;
	fill(&s, pos, 1);
	return s;
}

bool static_tone(uint32_t &start, uint32_t &length)
{
	if (g_nev != 1u) {
		return false;
	}
	const Event &e = g_ev[0];
	if (e.segment_id != kSegTone || e.length == 0u) {
		return false;
	}
	if (e.gain_q12 != kQ12One || (e.flags & kEvtEnvelope) != 0u) {
		return false;
	}
	if ((e.start_sample % synth::kRingWords) != 0u) {
		return false;
	}
	start = e.start_sample;
	length = e.length;
	return true;
}

}
