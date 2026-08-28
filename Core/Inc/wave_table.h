#ifndef WAVE_TABLE_H
#define WAVE_TABLE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint32_t offset;
	uint32_t length;
} wave_seg_t;

extern const int16_t wave_table[];
extern const uint32_t wave_table_len;
extern const wave_seg_t wave_seg[];
extern const uint32_t wave_seg_count;
extern const uint32_t wave_rate_hz;

#ifdef __cplusplus
}
#endif

#endif /* WAVE_TABLE_H */
