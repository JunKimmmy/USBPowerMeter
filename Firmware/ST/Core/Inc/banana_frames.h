#ifndef __BANANA_FRAMES_H
#define __BANANA_FRAMES_H

#include <stdint.h>

/* 8 animation frames, each 128 cols x 8 pages = 1024 bytes in flash.
 * Generate banana_frames.c by running:
 *   python Firmware/ST/generate_frames.py
 * Until then BANANA_FRAMES_READY == 0 and banana.c falls back to the
 * procedural banana sprite.
 */
#define BANANA_FRAME_COUNT   8
#define BANANA_FRAME_COLS  128
#define BANANA_FRAME_PAGES   8

extern const uint8_t BANANA_FRAMES_READY;
extern const uint8_t banana_frames[BANANA_FRAME_COUNT][BANANA_FRAME_PAGES][BANANA_FRAME_COLS];

#endif /* __BANANA_FRAMES_H */
