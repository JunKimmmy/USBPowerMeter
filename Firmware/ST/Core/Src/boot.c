#include "boot.h"
#include "oled.h"
#include "main.h"
#include <string.h>

extern I2C_HandleTypeDef hi2c2;

static uint8_t bfb[8][128];

static void bfb_px(int x, int y)
{
    if ((unsigned)x < 128u && (unsigned)y < 64u)
        bfb[y>>3][x] |= 1u << (y&7);
}

static void bfb_hline(int x0, int x1, int y)
{
    for (int x = x0; x <= x1; x++) bfb_px(x, y);
}

static void bfb_vline(int x, int y0, int y1)
{
    for (int y = y0; y <= y1; y++) bfb_px(x, y);
}

/* 1x font at pixel (x, y) */
static void bfb_str_y(int x, int y, const char *s)
{
    while (*s && x < 128) {
        uint8_t ci = (uint8_t)*s++ - 0x20;
        if (ci >= 95) ci = 0;
        for (int c = 0; c < 5; c++) {
            uint8_t b = font5x7[ci][c];
            for (int r = 0; r < 7; r++)
                if (b & (1 << r)) bfb_px(x + c, y + r);
        }
        x += 6;
    }
}

/* 2x font at pixel (x, y): 12px/char wide, 14px tall */
static void bfb_str2x(int x, int y, const char *s)
{
    while (*s) {
        uint8_t ci = (uint8_t)*s++ - 0x20;
        if (ci >= 95) ci = 0;
        for (int c = 0; c < 5; c++) {
            uint8_t b = font5x7[ci][c];
            for (int r = 0; r < 7; r++) {
                if (b & (1 << r)) {
                    int px = x + c * 2, py = y + r * 2;
                    bfb_px(px,     py);     bfb_px(px + 1, py);
                    bfb_px(px,     py + 1); bfb_px(px + 1, py + 1);
                }
            }
        }
        x += 12;
    }
}

static void bfb_flush(void)
{
    uint8_t buf[129]; buf[0] = 0x40;
    for (uint8_t p = 0; p < 8; p++) {
        OLED_SetCursor(p, 0);
        memcpy(buf + 1, bfb[p], 128);
        HAL_I2C_Master_Transmit(&hi2c2, OLED_I2C_ADDR, buf, 129, 100);
    }
}

static void bfb_flush_page(uint8_t p)
{
    uint8_t buf[129]; buf[0] = 0x40;
    OLED_SetCursor(p, 0);
    memcpy(buf + 1, bfb[p], 128);
    HAL_I2C_Master_Transmit(&hi2c2, OLED_I2C_ADDR, buf, 129, 100);
}

void BOOT_Run(void)
{
    memset(bfb, 0, sizeof(bfb));

    /* ── Phase 1: CRT-style scan-line wipe ──────────────────────────
       A full-bright band sweeps down page by page, then clears itself. */
    {
        uint8_t line[129]; line[0] = 0x40;
        memset(line + 1, 0xFF, 128);
        for (uint8_t p = 0; p < 8; p++) {
            OLED_SetCursor(p, 0);
            HAL_I2C_Master_Transmit(&hi2c2, OLED_I2C_ADDR, line, 129, 100);
            HAL_Delay(22);
            memset(line + 1, 0x00, 128);
            OLED_SetCursor(p, 0);
            HAL_I2C_Master_Transmit(&hi2c2, OLED_I2C_ADDR, line, 129, 100);
            memset(line + 1, 0xFF, 128);
        }
    }

    /* ── Phase 2: static layout ──────────────────────────────────────

       y=0..1   top H-arms of corner brackets
       y=0..13  left/right V-arms of corner brackets
       y=3..16  "USB" in 2x font, centered
       y=10,11  flanking double-lines left and right of "USB"
       y=19     separator line with small center diamond (y=18..21)
       y=22..28 "POWER METER" 1x centered
       y=31     "v1.0" 1x, center-right
       y=36     dotted accent line
       y=39..50 progress bar outline
       y=54..60 status text
       y=50..61 bottom V-arms
       y=62,63  bottom H-arms                                          */

    /* Corner L-brackets — 2px thick, 20px long arms */
    bfb_hline(0, 19, 0);    bfb_hline(0, 19, 1);      /* TL H */
    bfb_hline(108, 127, 0); bfb_hline(108, 127, 1);   /* TR H */
    bfb_hline(0, 19, 62);   bfb_hline(0, 19, 63);     /* BL H */
    bfb_hline(108, 127, 62); bfb_hline(108, 127, 63); /* BR H */
    bfb_vline(0, 2, 13);    bfb_vline(1, 2, 13);      /* TL V */
    bfb_vline(126, 2, 13);  bfb_vline(127, 2, 13);    /* TR V */
    bfb_vline(0, 50, 61);   bfb_vline(1, 50, 61);     /* BL V */
    bfb_vline(126, 50, 61); bfb_vline(127, 50, 61);   /* BR V */

    /* "USB" 2x centered: 3 chars × 12 = 36px → x=(128-36)/2=46, y=3 */
    bfb_str2x(46, 3, "USB");

    /* Flanking double-lines at y=10,11 (mid-height of 2x letters) */
    bfb_hline(3, 41,  10);  bfb_hline(3, 41,  11);
    bfb_hline(86, 124, 10); bfb_hline(86, 124, 11);

    /* Separator at y=20, split at centre for a small diamond accent */
    bfb_hline(3, 61, 20);
    bfb_hline(67, 124, 20);
    /* diamond centred at x=64, y=20 */
    bfb_px(64, 18);
    bfb_px(63, 19); bfb_px(65, 19);
    bfb_px(62, 20); bfb_px(66, 20);
    bfb_px(63, 21); bfb_px(65, 21);
    bfb_px(64, 22);

    /* "POWER METER" 1x centered: 11 chars × 6 = 66px → x=(128-66)/2=31, y=22 */
    bfb_str_y(31, 22, "POWER METER");

    /* "v1.0" 1x, nudged right of centre: x=72, y=31 */
    bfb_str_y(72, 31, "v1.0");

    /* Dotted accent line at y=36 */
    for (int x = 4; x < 124; x += 4) bfb_px(x, 36);

    /* Progress bar outline: x=10..117, y=39..50
       Top border at y=39 (page 4, bit 7), bottom at y=50 (page 6, bit 2)
       Left/right walls at x=10 and x=117 spanning y=39..50             */
    bfb_hline(10, 117, 39);
    bfb_hline(10, 117, 50);
    bfb_vline(10,  39, 50);
    bfb_vline(117, 39, 50);

    /* "INITIALIZING" 1x centered: 12 chars × 6 = 72px → x=(128-72)/2=28, y=54 */
    bfb_str_y(28, 54, "INITIALIZING");

    bfb_flush();

    /* ── Phase 3: animated segmented progress bar ────────────────────
       10 segments, each 8px wide with a 2px gap.
       Total: 10×8 + 9×2 = 98px, centred in 107px interior → x_start=14.
       Fill interior y=41..48 (entirely in pages 5 and 6).              */
    for (int seg = 0; seg < 10; seg++) {
        int sx = 14 + seg * 10;
        for (int x = sx; x < sx + 8; x++)
            for (int y = 41; y <= 48; y++)
                bfb_px(x, y);
        bfb_flush_page(5);   /* y=40..47 */
        bfb_flush_page(6);   /* y=48..55 */
        HAL_Delay(70);
    }

    /* ── Phase 4: "SYSTEM READY" replaces "INITIALIZING" ────────────
       Text at y=54..60 touches page 6 (bits 6,7 → mask 0xC0)
       and page 7 (bits 0..4 → mask 0x1F).                            */
    for (int x = 0; x < 128; x++) {
        bfb[6][x] &= ~0xC0;
        bfb[7][x] &= ~0x1F;
    }
    /* "SYSTEM READY" 12 chars × 6 = 72px → x=28 */
    bfb_str_y(28, 54, "SYSTEM READY");
    bfb_flush_page(6);
    bfb_flush_page(7);

    HAL_Delay(800);
    OLED_Clear();
}
