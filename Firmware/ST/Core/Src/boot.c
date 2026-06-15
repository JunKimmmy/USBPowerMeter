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
static void bfb_str(int x, int page, const char *s)
{
    while (*s && x < 128) {
        uint8_t ci = (uint8_t)*s++ - 0x20;
        if (ci >= 95) ci = 0;
        for (uint8_t b = 0; b < 5 && (x+b) < 128; b++)
            bfb[page][x+b] |= font5x7[ci][b];
        x += 6;
    }
}
static void bfb_flush(void)
{
    uint8_t buf[129]; buf[0] = 0x40;
    for (uint8_t p = 0; p < 8; p++) {
        OLED_SetCursor(p, 0);
        memcpy(buf+1, bfb[p], 128);
        HAL_I2C_Master_Transmit(&hi2c2, OLED_I2C_ADDR, buf, 129, 100);
    }
}
static void bfb_flush_page(uint8_t p)
{
    uint8_t buf[129]; buf[0] = 0x40;
    OLED_SetCursor(p, 0);
    memcpy(buf+1, bfb[p], 128);
    HAL_I2C_Master_Transmit(&hi2c2, OLED_I2C_ADDR, buf, 129, 100);
}

/* Small diamond mark drawn at (cx,cy) */
static void bfb_diamond(int cx, int cy)
{
    bfb_px(cx,   cy-1);
    bfb_px(cx-1, cy);  bfb_px(cx+1, cy);
    bfb_px(cx,   cy+1);
}

void BOOT_Run(void)
{
    memset(bfb, 0, sizeof(bfb));

    /* ── Outer double border ── */
    bfb_hline(0, 127, 0);  bfb_hline(0, 127, 1);
    bfb_hline(0, 127, 62); bfb_hline(0, 127, 63);
    bfb_vline(0, 2, 61);   bfb_vline(1, 2, 61);
    bfb_vline(126, 2, 61); bfb_vline(127, 2, 61);

    /* Diamond accents at inner border corners */
    bfb_diamond(4, 4);
    bfb_diamond(123, 4);
    bfb_diamond(4, 59);
    bfb_diamond(123, 59);

    /* Mid-border accent dots */
    bfb_px(0, 32); bfb_px(1, 32); bfb_px(126, 32); bfb_px(127, 32);

    /* ── Title box: x=5..122, y=6..25 ── */
    bfb_hline(5, 122, 6);
    bfb_hline(5, 122, 25);
    bfb_vline(5, 7, 24);
    bfb_vline(122, 7, 24);
    /* Thicker corners — extra 1-px inset brackets */
    bfb_px(6, 7);  bfb_px(7, 7);   bfb_px(6, 8);    /* top-left  */
    bfb_px(121, 7); bfb_px(120, 7); bfb_px(121, 8);  /* top-right */
    bfb_px(6, 24); bfb_px(7, 24);  bfb_px(6, 23);   /* bot-left  */
    bfb_px(121, 24); bfb_px(120, 24); bfb_px(121, 23); /* bot-right */

    /* ── "USB POWER METER" centred in box ── */
    /* inner box x=6..121 = 116 px; 15 chars * 6 = 90 px; pad = (116-90)/2 = 13 */
    bfb_str(6 + 13, 1, "USB POWER METER");   /* page 1: y=8..15 */

    /* ── "V1.0" centred below ── */
    /* 4 chars * 6 = 24 px; pad = (116-24)/2 = 46 */
    bfb_str(6 + 46, 2, "V1.0");              /* page 2: y=16..23 */

    /* ── "INITIALIZING" centred in main area ── */
    /* inner x=2..125 = 124 px; 12 chars*6 = 72; pad = (124-72)/2 = 26 */
    bfb_str(2 + 26, 4, "INITIALIZING");      /* page 4: y=32..39 */

    /* ── Progress bar outline — entirely in page 6 (y=48..55) ── */
    /* Left/right walls = all bits */
    bfb[6][10]  = 0xFF;
    bfb[6][117] = 0xFF;
    /* Top (y=48, bit 0) and bottom (y=55, bit 7) */
    for (int x = 11; x <= 116; x++) {
        bfb[6][x] |= 0x01;   /* top    */
        bfb[6][x] |= 0x80;   /* bottom */
    }

    /* Flush everything except the animated fill */
    bfb_flush();

    /* ── Animate progress bar: fill y=50..53 (bits 2-5 = 0x3C) ──
       104 columns (x=12..115), 4 columns per flush, 26 steps × 18 ms  */
    for (uint8_t step = 0; step < 104; step++) {
        bfb[6][12 + step] |= 0x3C;
        if ((step & 3) == 3) {
            bfb_flush_page(6);
            HAL_Delay(18);
        }
    }
    bfb_flush_page(6);

    /* ── "SYSTEM  READY" replaces "INITIALIZING" ── */
    memset(bfb[4], 0, 128);
    bfb_str(2 + 26, 4, "SYSTEM READY");   /* centred same as INITIALIZING */
    bfb_flush_page(4);

    HAL_Delay(700);
    OLED_Clear();
}
