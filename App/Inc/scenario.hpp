#ifndef SCENARIO_HPP
#define SCENARIO_HPP

#include <cstdint>
#include <span>

namespace scenario {

inline constexpr uint16_t kSegSilence = 0xFFFF;
inline constexpr uint16_t kSegTone    = 0xFFFE;
inline constexpr uint16_t kSegStep    = 0xFFFD;
inline constexpr uint16_t kEvtEnvelope = 0x0001;

struct Event {
	uint32_t start_sample;
	uint32_t length;
	uint16_t segment_id;
	int16_t  gain_q12;
	uint16_t flags;
};

enum class Id {
	A, B, C, D, E, F, G, H, I, J, Step, Tone, Count
};

struct Info {
	const char *name;
	const char *alias;
	const char *blurb;
};

void init();
bool select(const char *name);
const char *name();
Id id();
std::span<const Info> catalog();
bool needs_table();
bool table_ready();

std::span<const Event> events();
uint32_t duration();
void rewind();

/* Write n signed samples for positions [pos, pos+n) into dst.

   This is the hot path. It splits the request into runs that share one event
   and one segment kind, then uses a tight loop per run, so the event lookup
   and the segment dispatch happen once per run instead of once per sample.
   pos must not go backwards between calls without a rewind(). */
void fill(int32_t *dst, uint32_t pos, uint32_t n);

/* One sample, defined in terms of fill() so the two cannot drift apart. */
int32_t sample(uint32_t pos);

/* True if the whole scenario is a single plain tone -- unity gain, no
   envelope, no gaps -- that begins on a ring boundary. synth streams the
   preamble, then stops filling once the ring holds one whole tone period,
   because kPhaseInc makes that period self-joining. */
bool static_tone(uint32_t &start, uint32_t &length);

}

#endif
