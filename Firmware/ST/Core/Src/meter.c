#include "meter.h"
#include "oled.h"
#include "ina238.h"
#include "main.h"
#include <string.h>
#include <stdio.h>

extern I2C_HandleTypeDef hi2c2;
extern void Buzzer_Tone(uint32_t freq_hz, uint32_t duration_ms);
extern uint8_t g_muted;
void DANCE_Run(void);

/* ═══════════════════════════════════════════════════════════════════
 * Framebuffer
 * ═══════════════════════════════════════════════════════════════════ */
static uint8_t mfb[8][128];

static void fb_clear(void) { memset(mfb, 0, sizeof(mfb)); }

static void fb_px(int x, int y)
{
    if ((unsigned)x < 128u && (unsigned)y < 64u)
        mfb[y >> 3][x] |= 1u << (y & 7);
}

static void fb_hline(int x0, int x1, int y)
{
    for (int x = x0; x <= x1; x++) fb_px(x, y);
}

static void fb_vline(int x, int y0, int y1)
{
    for (int y = y0; y <= y1; y++) fb_px(x, y);
}

/* Bresenham line */
static void fb_line(int x0, int y0, int x1, int y1)
{
    int dx =  (x1 > x0 ? x1 - x0 : x0 - x1);
    int dy = -(y1 > y0 ? y1 - y0 : y0 - y1);
    int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1, e = dx + dy;
    while (1) {
        fb_px(x0, y0);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * e;
        if (e2 >= dy) { e += dy; x0 += sx; }
        if (e2 <= dx) { e += dx; y0 += sy; }
    }
}

static void fb_flush(void)
{
    uint8_t buf[129]; buf[0] = 0x40;
    for (uint8_t p = 0; p < 8; p++) {
        OLED_SetCursor(p, 0);
        memcpy(buf + 1, mfb[p], 128);
        HAL_I2C_Master_Transmit(&hi2c2, OLED_I2C_ADDR, buf, 129, 100);
    }
}

/* ── 1x font at pixel (x, y) ─────────────────────────────────────── */
static void fb_str(int x, int y, const char *s)
{
    while (*s && x < 128) {
        uint8_t ci = (uint8_t)*s++ - 0x20;
        if (ci >= 95) ci = 0;
        for (int c = 0; c < 5; c++) {
            uint8_t b = font5x7[ci][c];
            for (int r = 0; r < 7; r++)
                if (b & (1 << r)) fb_px(x + c, y + r);
        }
        x += 6;
    }
}

/* ── 2x font at pixel (x, y): 12px/char wide, 14px tall ─────────── */
static void fb_str2x(int x, int y, const char *s)
{
    while (*s) {
        uint8_t ci = (uint8_t)*s++ - 0x20;
        if (ci >= 95) ci = 0;
        for (int c = 0; c < 5; c++) {
            uint8_t b = font5x7[ci][c];
            for (int r = 0; r < 7; r++) {
                if (b & (1 << r)) {
                    int px = x + c * 2, py = y + r * 2;
                    fb_px(px,     py);   fb_px(px + 1, py);
                    fb_px(px,     py + 1); fb_px(px + 1, py + 1);
                }
            }
        }
        x += 12;
    }
}

/* Centred helpers */
static void fb_str_c(int y, const char *s)
{
    int w = (int)strlen(s) * 6;
    fb_str((128 - w) / 2, y, s);
}

static void fb_str2x_c(int y, const char *s)
{
    int w = (int)strlen(s) * 12;
    int x = (128 - w) / 2;
    if (x < 0) x = 0;
    fb_str2x(x, y, s);
}

/* ═══════════════════════════════════════════════════════════════════
 * Graph data  (circular buffer, 128 samples)
 * ═══════════════════════════════════════════════════════════════════ */
#define GRAPH_LEN  128u

static int32_t  g_cur[GRAPH_LEN];   /* mA  */
static int32_t  g_pwr[GRAPH_LEN];   /* mW  */
static uint32_t g_head  = 0;
static uint32_t g_count = 0;
static int64_t  g_sum_cur = 0, g_sum_pwr = 0;
static int32_t  g_max_cur = 1,  g_max_pwr = 1;

static void graph_push(int32_t cur_ma, int32_t pwr_mw)
{
    if (g_count >= GRAPH_LEN) {
        g_sum_cur -= g_cur[g_head];
        g_sum_pwr -= g_pwr[g_head];
    }
    g_cur[g_head] = cur_ma;
    g_pwr[g_head] = pwr_mw;
    g_sum_cur += cur_ma;
    g_sum_pwr += pwr_mw;
    g_head = (g_head + 1) & (GRAPH_LEN - 1);
    if (g_count < GRAPH_LEN) g_count++;
    if (cur_ma > g_max_cur) g_max_cur = cur_ma;
    if (pwr_mw > g_max_pwr) g_max_pwr = pwr_mw;
}

static void graph_reset(void)
{
    memset(g_cur, 0, sizeof(g_cur));
    memset(g_pwr, 0, sizeof(g_pwr));
    g_head = 0; g_count = 0;
    g_sum_cur = 0; g_sum_pwr = 0;
    g_max_cur = 1; g_max_pwr = 1;
}

/* ═══════════════════════════════════════════════════════════════════
 * Button helper — debounce + wait-for-release
 * ═══════════════════════════════════════════════════════════════════ */
static uint8_t btn_read(uint16_t pin, GPIO_TypeDef *port)
{
    if (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_RESET) {
        HAL_Delay(20);
        if (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_RESET) {
            while (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_RESET);
            return 1;
        }
    }
    return 0;
}

/* ═══════════════════════════════════════════════════════════════════
 * Screen 0 — Main Meter
 *
 *  y=0:         top border line
 *  y=1..7:      "USB POWER METER"  (1x, centred inside header box)
 *  y=8:         bottom of header box / separator
 *  y=10..23:    voltage  2x value  e.g. "5.120V"
 *  y=25:        section divider
 *  y=27..40:    current  2x value  e.g. "1.234A"
 *  y=41:        section divider
 *  y=42..55:    power    2x value  e.g. "6.318W"
 *  y=57..63:    footer   1x running averages
 * ═══════════════════════════════════════════════════════════════════ */
static void draw_meter(int32_t vbus_mv, int32_t cur_ma, int32_t pwr_mw,
                       int32_t avg_cur, int32_t avg_pwr)
{
    fb_clear();
    char buf[36], buf2[36];

    /* ═══════════════════════════════════════════════════════════════
     * Layout  (128 × 64)
     *
     *  y= 0         top border
     *  y= 2.. 8     "VOLT"  |  "AMPS"   (1x labels, centred in halves)
     *  y= 9         thin underline in each column
     *  y=11..24     5.12    |  1.23     (2x values, centred in halves)
     *  y=25..31     V       |  A        (1x units)
     *  y=32..33     ══ double separator ══
     *  y=37..50     6.318 W             (2x power, full-width centred)
     *  y=55         footer separator
     *  y=57..63     Avg 1.23A  1.23W   (1x footer)
     *  x=63         vertical column divider  (y=0..31)
     * ═══════════════════════════════════════════════════════════════ */

    /* ── Top border ── */
    fb_hline(0, 127, 0);

    /* ── Column divider ── */
    fb_vline(63, 0, 31);

    /* ── Column labels ── */
    fb_str(19, 2, "VOLT");     /* centre in x=0..62: (62-24)/2 = 19 */
    fb_str(84, 2, "AMPS");     /* centre in x=64..127: 64+(63-24)/2 = 84 */

    /* ── Thin underline below labels ── */
    fb_hline(1,  61,  9);
    fb_hline(65, 126, 9);

    /* ── 2x values, dynamically centred within each half ── */
    snprintf(buf,  sizeof(buf),  "%ld.%02ld", vbus_mv / 1000, (vbus_mv % 1000) / 10);
    snprintf(buf2, sizeof(buf2), "%ld.%02ld", cur_ma  / 1000, (cur_ma  % 1000) / 10);

    {   /* voltage — centre in x=0..62 */
        int w = (int)strlen(buf) * 12;
        int x = (62 - w) / 2; if (x < 0) x = 0;
        fb_str2x(x, 11, buf);
    }
    {   /* current — centre in x=64..127 */
        int w = (int)strlen(buf2) * 12;
        int x = 64 + (63 - w) / 2; if (x < 64) x = 64;
        fb_str2x(x, 11, buf2);
    }

    /* ── Unit labels, centred in each half ── */
    fb_str(28, 26, "V");       /* centre in x=0..62:  (62-6)/2 = 28 */
    fb_str(93, 26, "A");       /* centre in x=64..127: 64+(63-6)/2 = 93 */

    /* ── Double section separator ── */
    fb_hline(0, 127, 32);
    fb_hline(0, 127, 33);

    /* ── Power value, large and centred ── */
    snprintf(buf, sizeof(buf), "%ld.%03ldW", pwr_mw / 1000, pwr_mw % 1000);
    fb_str2x_c(37, buf);       /* 2x, 14px tall → fills y=37..50 */

    /* ── Footer separator ── */
    fb_hline(0, 127, 55);

    /* ── Footer: running averages ── */
    if (g_count > 0) {
        snprintf(buf, sizeof(buf), "Avg %ld.%02ldA  %ld.%02ldW",
                 avg_cur / 1000, (avg_cur % 1000) / 10,
                 avg_pwr / 1000, (avg_pwr % 1000) / 10);
        fb_str_c(57, buf);
    }
}

/* ═══════════════════════════════════════════════════════════════════
 * Screen 1/2 — Rolling Graph
 *
 *  y=0..7:   title + live reading  (1x)
 *  y=8..15:  "Avg:x.xx  Max:y.yy" stats (1x)
 *  y=16:     graph top border
 *  y=17..63: line plot (47px tall × 128px wide)
 *            dashed horizontal = running average
 * ═══════════════════════════════════════════════════════════════════ */
#define GRAPH_TOP     17
#define GRAPH_BOT     63
#define GRAPH_H       (GRAPH_BOT - GRAPH_TOP)   /* 46 px */

static void draw_graph(uint8_t is_cur)
{
    fb_clear();

    /* Live reading from most-recent sample */
    int32_t live_val = 0, avg_val = 0, max_val = 1;
    if (g_count) {
        uint32_t idx = (g_head - 1 + GRAPH_LEN) & (GRAPH_LEN - 1);
        live_val = is_cur ? g_cur[idx] : g_pwr[idx];
        avg_val  = (int32_t)((is_cur ? g_sum_cur : g_sum_pwr) / (int64_t)g_count);
        max_val  = is_cur ? g_max_cur : g_max_pwr;
        if (max_val < 1) max_val = 1;
    }

    char buf[36];

    /* ── Header line: screen title + live reading ── */
    snprintf(buf, sizeof(buf), "%s %ld.%03ld%c",
             is_cur ? "CURRENT" : "POWER  ",
             live_val / 1000, live_val % 1000,
             is_cur ? 'A' : 'W');
    fb_str(0, 0, buf);

    /* Screen indicator dots top-right */
    fb_px(124, 3); fb_px(124, 3);
    if (is_cur) { fb_px(121, 3); fb_px(124, 3); }   /* two dots = screen 1 */
    else        { fb_px(118, 3); fb_px(121, 3); fb_px(124, 3); } /* three = screen 2 */

    /* ── Stats line ── */
    snprintf(buf, sizeof(buf), "Avg:%ld.%02ld  Max:%ld.%02ld",
             avg_val / 1000, (avg_val % 1000) / 10,
             max_val / 1000, (max_val % 1000) / 10);
    fb_str(0, 8, buf);

    /* ── Graph border ── */
    fb_hline(0, 127, GRAPH_TOP - 1);

    /* Max scale label inside top-right of graph area */
    snprintf(buf, sizeof(buf), "%ld.%01ld",
             max_val / 1000, (max_val % 1000) / 100);
    int lbl_x = 127 - (int)strlen(buf) * 6;
    if (lbl_x < 0) lbl_x = 0;
    fb_str(lbl_x, GRAPH_TOP, buf);

    /* ── Scale: add ~15% headroom so the max line doesn't touch the top ── */
    int32_t disp_max = max_val + max_val / 6;
    if (disp_max < 1) disp_max = 1;

    /* ── Dashed average line ── */
    if (g_count > 0) {
        int avg_y = GRAPH_BOT - (int)(((int64_t)avg_val * GRAPH_H) / disp_max);
        if (avg_y < GRAPH_TOP)  avg_y = GRAPH_TOP;
        if (avg_y > GRAPH_BOT)  avg_y = GRAPH_BOT;
        for (int x = 0; x < 128; x += 3)
            fb_px(x, avg_y);
    }

    /* ── Line plot: newest sample at x=127, oldest at x=127-g_count+1 ── */
    if (g_count > 0) {
        int prev_x = -1, prev_y = -1;
        for (uint32_t i = 0; i < g_count; i++) {
            uint32_t idx = (g_head - g_count + i + GRAPH_LEN) & (GRAPH_LEN - 1);
            int x = (int)(127 - (int32_t)g_count + 1 + (int32_t)i);
            if (x < 0 || x > 127) { prev_x = -1; continue; }

            int32_t val = is_cur ? g_cur[idx] : g_pwr[idx];
            int h = (int)(((int64_t)val * GRAPH_H) / disp_max);
            if (h > GRAPH_H) h = GRAPH_H;
            if (h < 0)       h = 0;
            int y = GRAPH_BOT - h;

            if (prev_x >= 0)
                fb_line(prev_x, prev_y, x, y);
            else
                fb_px(x, y);

            prev_x = x; prev_y = y;
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════
 * METER_Run — never returns
 * ═══════════════════════════════════════════════════════════════════ */
void METER_Run(void)
{
    uint8_t  screen     = 0;   /* 0=meter  1=I-graph  2=P-graph */
    uint32_t last_tick  = 0;
    uint8_t  reset_clicks = 0;
    uint32_t reset_window = 0;   /* tick of first click in current sequence */

    graph_reset();
    OLED_Clear();

    while (1) {
        uint32_t now = HAL_GetTick();

        /* ── Sample + redraw at 200 ms intervals ── */
        if (now - last_tick >= 200u) {
            last_tick = now;

            int32_t vbus_mv = INA238_ReadVBUS_mV();
            int32_t cur_ma  = INA238_ReadCurrent_mA();
            if (vbus_mv < 0) vbus_mv = 0;
            if (cur_ma  < 0) cur_ma  = 0;
            int32_t pwr_mw = (int32_t)(((int64_t)vbus_mv * cur_ma) / 1000);

            graph_push(cur_ma, pwr_mw);

            int32_t avg_cur = g_count ? (int32_t)(g_sum_cur / (int64_t)g_count) : 0;
            int32_t avg_pwr = g_count ? (int32_t)(g_sum_pwr / (int64_t)g_count) : 0;

            if      (screen == 0) draw_meter(vbus_mv, cur_ma, pwr_mw, avg_cur, avg_pwr);
            else if (screen == 1) draw_graph(1);   /* current */
            else                  draw_graph(0);   /* power   */

            if (g_muted) fb_str(111, 2, "M");
            fb_flush();
        }

        /* ── BTN_PAGE: previous screen ── */
        if (btn_read(BTN_PAGE_Pin, BTN_PAGE_GPIO_Port)) {
            if (!g_muted) { Buzzer_Tone(1200, 80); Buzzer_Tone(1200, 80); }
            screen = (screen + 2) % 3u;
        }

        /* ── BTN_AUX: next screen ── */
        if (btn_read(BTN_AUX_Pin, BTN_AUX_GPIO_Port)) {
            if (!g_muted) { Buzzer_Tone(1200, 80); Buzzer_Tone(1200, 80); }
            screen = (screen + 1) % 3u;
        }

        /* ── BTN_RESET: hold 800ms = mute toggle; short press = home / Easter egg ── */
        if (HAL_GPIO_ReadPin(BTN_RESET_GPIO_Port, BTN_RESET_Pin) == GPIO_PIN_RESET) {
            HAL_Delay(20);
            if (HAL_GPIO_ReadPin(BTN_RESET_GPIO_Port, BTN_RESET_Pin) == GPIO_PIN_RESET) {
                uint32_t press_t = HAL_GetTick();
                uint8_t held = 0;
                while (HAL_GPIO_ReadPin(BTN_RESET_GPIO_Port, BTN_RESET_Pin) == GPIO_PIN_RESET) {
                    if (HAL_GetTick() - press_t >= 800u) { held = 1; break; }
                }
                while (HAL_GPIO_ReadPin(BTN_RESET_GPIO_Port, BTN_RESET_Pin) == GPIO_PIN_RESET);
                if (held) {
                    g_muted ^= 1;
                } else {
                    if (!g_muted) { Buzzer_Tone(1200, 80); Buzzer_Tone(1200, 80); }
                    uint32_t t = HAL_GetTick();
                    if (reset_clicks == 0 || (t - reset_window) > 2000u) {
                        reset_clicks = 1;
                        reset_window = t;
                    } else {
                        reset_clicks++;
                    }
                    if (reset_clicks >= 4) {
                        reset_clicks = 0;
                        DANCE_Run();
                        OLED_Clear();
                    } else {
                        screen = 0;
                    }
                }
            }
        }
    }
}
