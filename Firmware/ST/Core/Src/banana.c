#include "banana.h"
#include "oled.h"
#include "main.h"
#include <string.h>

extern I2C_HandleTypeDef hi2c2;
extern TIM_HandleTypeDef htim3;
extern void Buzzer_Tone(uint32_t freq_hz, uint32_t duration_ms);

/* ═══════════════════════════════════════════════════════════════════
 * Framebuffer
 * ═══════════════════════════════════════════════════════════════════ */
static uint8_t fb[8][128];
static void fb_clear(void) { memset(fb, 0, sizeof(fb)); }

static void fb_px(int x, int y)
{
    if ((unsigned)x < 128u && (unsigned)y < 64u)
        fb[y>>3][x] |= 1u << (y&7);
}

static void fb_line(int x0, int y0, int x1, int y1)
{
    int dx =  (x1>x0 ? x1-x0 : x0-x1);
    int dy = -(y1>y0 ? y1-y0 : y0-y1);
    int sx = x0<x1 ? 1 : -1, sy = y0<y1 ? 1 : -1, err = dx+dy;
    while (1) {
        fb_px(x0, y0);
        if (x0==x1 && y0==y1) break;
        int e2 = 2*err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

static void fb_circle(int cx, int cy, int r)
{
    int x = r, y = 0, err = 1-r;
    while (x >= y) {
        fb_px(cx+x,cy+y); fb_px(cx-x,cy+y);
        fb_px(cx+x,cy-y); fb_px(cx-x,cy-y);
        fb_px(cx+y,cy+x); fb_px(cx-y,cy+x);
        fb_px(cx+y,cy-x); fb_px(cx-y,cy-x);
        y++;
        if (err < 0) err += 2*y+1;
        else { x--; err += 2*(y-x+1); }
    }
}

static void fb_str(int x, int page, const char *s)
{
    while (*s && x <= 122) {
        uint8_t c = (uint8_t)*s++;
        if (c < 0x20 || c > 0x7E) c = 0x20;
        for (uint8_t i = 0; i < 5 && (x+i) < 128; i++)
            fb[page][x+i] |= font5x7[c-0x20][i];
        x += 6;
    }
}

static void fb_flush(void)
{
    uint8_t buf[129]; buf[0] = 0x40;
    for (uint8_t p = 0; p < 8; p++) {
        OLED_SetCursor(p, 0);
        memcpy(buf+1, fb[p], 128);
        HAL_I2C_Master_Transmit(&hi2c2, OLED_I2C_ADDR, buf, 129, 100);
    }
}

/* ═══════════════════════════════════════════════════════════════════
 * Buzzer
 * ═══════════════════════════════════════════════════════════════════ */
static void buzz(uint32_t freq)
{
    if (!freq) { HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_3); return; }
    uint32_t arr = (1000000UL/freq) - 1;
    __HAL_TIM_SET_AUTORELOAD(&htim3, arr);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, arr/2);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
}

/* ═══════════════════════════════════════════════════════════════════
 * MIDI melody
 * ═══════════════════════════════════════════════════════════════════ */
#define N_MUSIC 46

static const uint16_t M_FREQ[N_MUSIC] = {
    784, 523, 659, 784, 523, 659, 784, 494, 622, 784,
    988, 880, 784, 466, 587, 784, 466, 587, 784, 554,
    659, 784, 988, 880, 988,1047, 988,1047, 880,1047,
    988, 880, 784, 740, 784, 659, 554, 587, 659, 698,
    659, 698, 494, 659, 587, 523
};
static const uint16_t M_DUR[N_MUSIC] = {
    100, 100, 100, 299, 100, 100, 100, 100, 100, 100,
    100, 711, 100, 100, 100, 299, 100, 100, 100, 100,
    100, 100, 100, 498, 100, 100, 100, 100, 199, 100,
    100, 100, 100, 100, 100, 299, 100, 100, 100, 100,
    100, 100, 299, 100, 100, 711
};
static const uint16_t M_GAP[N_MUSIC] = {
    128,  14, 128,  43,  14, 128,  14,  14,  14,  14,
    128,  85, 128,  14, 128,  43,  14, 128,  14,  14,
     14,  14, 128, 185,  14, 128,  14, 128, 142,  14,
    128,  14, 128,  14, 128,  43,  14, 128,  14, 128,
     14, 128,  43,  14, 128,   0
};

/* ═══════════════════════════════════════════════════════════════════
 * LCG random  (seeded from HAL_GetTick at game start)
 * ═══════════════════════════════════════════════════════════════════ */
static uint32_t rng_s;
static uint16_t rnd16(uint16_t n)
{
    rng_s = rng_s * 1664525u + 1013904223u;
    return (uint16_t)((rng_s >> 16) % n);
}

/* ═══════════════════════════════════════════════════════════════════
 * Game constants
 * ═══════════════════════════════════════════════════════════════════ */
#define SHIP_X        2    /* leftmost pixel of ship body */
#define SHIP_W       13    /* ship width incl. nose       */
#define BULLET_SPD    6    /* pixels per game frame        */
#define MAX_BULLETS   4
#define MAX_ASTROS    5
#define FIRE_EVERY    5    /* frames between auto-shots   */

/* ═══════════════════════════════════════════════════════════════════
 * Game state  (re-initialised every call to DANCE_Run)
 * ═══════════════════════════════════════════════════════════════════ */
static int16_t ship_y;

static int8_t  b_act[MAX_BULLETS];
static int16_t b_x  [MAX_BULLETS];
static int8_t  b_y  [MAX_BULLETS];

static int8_t  a_act[MAX_ASTROS];
static int16_t a_x  [MAX_ASTROS];
static int8_t  a_y  [MAX_ASTROS];
static int8_t  a_r  [MAX_ASTROS];
static int8_t  a_spd[MAX_ASTROS];

static uint16_t score;

/* Stars: x positions scroll left; y fixed */
static uint8_t       star_x[8];
static const uint8_t STAR_Y[8] = { 9, 19, 28, 38, 14, 33, 46, 23 };

/* ═══════════════════════════════════════════════════════════════════
 * Sprite drawing
 * ═══════════════════════════════════════════════════════════════════ */

/* Ship centred vertically at sy, fixed at SHIP_X */
static void draw_ship(int sy, uint8_t fr)
{
    /* Fuselage */
    fb_line(SHIP_X+2, sy, SHIP_X+10, sy);
    /* Nose */
    fb_px(SHIP_X+11, sy);
    fb_px(SHIP_X+12, sy);
    /* Cockpit bump */
    fb_px(SHIP_X+6, sy-1);
    fb_px(SHIP_X+6, sy+1);
    /* Upper wing */
    fb_line(SHIP_X+3, sy-1, SHIP_X+9, sy-1);
    fb_px(SHIP_X+8, sy-2);
    fb_px(SHIP_X+6, sy-3);
    /* Lower wing */
    fb_line(SHIP_X+3, sy+1, SHIP_X+9, sy+1);
    fb_px(SHIP_X+8, sy+2);
    fb_px(SHIP_X+6, sy+3);
    /* Engine exhaust — flickers each frame */
    if (fr & 1) {
        fb_px(SHIP_X+1, sy);
        fb_px(SHIP_X,   sy-1);
        fb_px(SHIP_X,   sy+1);
    } else {
        fb_px(SHIP_X+1, sy-1);
        fb_px(SHIP_X+1, sy+1);
        fb_px(SHIP_X-1, sy);
    }
}

static void draw_bullet(int bx, int by)
{
    fb_px(bx, by); fb_px(bx+1, by); fb_px(bx+2, by);
}

static void draw_asteroid(int ax, int ay, int r)
{
    fb_circle(ax, ay, r);
    /* Surface detail */
    fb_px(ax - r/2, ay - r/3);
    fb_px(ax + r/3, ay + r/2);
}

static void draw_stars(void)
{
    for (int i = 0; i < 8; i++)
        fb_px((int)star_x[i], (int)STAR_Y[i]);
}

static void draw_score(void)
{
    char buf[5];
    buf[0] = '0' + (char)((score/1000) % 10);
    buf[1] = '0' + (char)((score/100)  % 10);
    buf[2] = '0' + (char)((score/10)   % 10);
    buf[3] = '0' + (char)( score       % 10);
    buf[4] = '\0';
    fb_str(97, 0, buf);
}

/* ═══════════════════════════════════════════════════════════════════
 * DANCE_Run — space shooter
 *
 * BTN_AUX   : move ship UP
 * BTN_RESET : move ship DOWN
 * BTN_PAGE  : exit to power meter
 * ═══════════════════════════════════════════════════════════════════ */
void DANCE_Run(void)
{
    /* Init game state */
    rng_s   = HAL_GetTick();
    ship_y  = 35;
    score   = 0;

    for (int i = 0; i < MAX_BULLETS; i++) b_act[i] = 0;
    for (int i = 0; i < MAX_ASTROS;  i++) a_act[i] = 0;
    for (int i = 0; i < 8; i++) star_x[i] = (uint8_t)(5 + i * 15);

    /* Pre-spawn two asteroids so the screen isn't empty */
    a_act[0]=1; a_x[0]=100; a_y[0]=20; a_r[0]=4; a_spd[0]=2;
    a_act[1]=1; a_x[1]=70;  a_y[1]=45; a_r[1]=3; a_spd[1]=1;

    /* Music sequencer state */
    uint32_t m_note  = 0;
    uint32_t m_start = HAL_GetTick();
    uint8_t  m_phase = 0;
    buzz(M_FREQ[0]);

    /* Game timing */
    uint32_t last_frame = 0;
    uint32_t frame      = 0;
    uint32_t spawn_tmr  = 0;

    while (1)
    {
        uint32_t now = HAL_GetTick();

        /* ── BTN_PAGE : exit ───────────────────────────────────── */
        if (HAL_GPIO_ReadPin(BTN_PAGE_GPIO_Port, BTN_PAGE_Pin) == GPIO_PIN_RESET) {
            HAL_Delay(20);
            if (HAL_GPIO_ReadPin(BTN_PAGE_GPIO_Port, BTN_PAGE_Pin) == GPIO_PIN_RESET) {
                buzz(0);
                Buzzer_Tone(1200, 80);
                Buzzer_Tone(1200, 80);
                OLED_Clear();
                return;
            }
        }

        /* ── Arcade music sequencer ────────────────────────────── */
        if (m_phase == 0) {
            if (now - m_start >= M_DUR[m_note]) {
                buzz(0);
                if (M_GAP[m_note] > 0) { m_phase = 1; m_start = now; }
                else {
                    m_note = (m_note + 1) % N_MUSIC;
                    buzz(M_FREQ[m_note]); m_start = now;
                }
            }
        } else {
            if (now - m_start >= M_GAP[m_note]) {
                m_note = (m_note + 1) % N_MUSIC;
                buzz(M_FREQ[m_note]); m_start = now; m_phase = 0;
            }
        }

        /* ── Game frame at 50 ms (20 fps) ─────────────────────── */
        if (now - last_frame < 50) continue;
        last_frame = now;

        /* Input — move ship */
        if (HAL_GPIO_ReadPin(BTN_AUX_GPIO_Port,   BTN_AUX_Pin)   == GPIO_PIN_RESET) ship_y -= 3;
        if (HAL_GPIO_ReadPin(BTN_RESET_GPIO_Port, BTN_RESET_Pin) == GPIO_PIN_RESET) ship_y += 3;
        if (ship_y < 11) ship_y = 11;
        if (ship_y > 54) ship_y = 54;

        /* Auto-fire */
        if (frame % FIRE_EVERY == 0) {
            for (int i = 0; i < MAX_BULLETS; i++) {
                if (!b_act[i]) {
                    b_act[i] = 1;
                    b_x[i]   = (int16_t)(SHIP_X + SHIP_W + 1);
                    b_y[i]   = (int8_t)ship_y;
                    break;
                }
            }
        }

        /* Move bullets right */
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (!b_act[i]) continue;
            b_x[i] += BULLET_SPD;
            if (b_x[i] > 127) b_act[i] = 0;
        }

        /* Move asteroids left */
        for (int i = 0; i < MAX_ASTROS; i++) {
            if (!a_act[i]) continue;
            a_x[i] -= a_spd[i];
            if (a_x[i] + a_r[i] < 0) a_act[i] = 0;
        }

        /* Spawn one asteroid at variable interval */
        spawn_tmr++;
        uint32_t spawn_int = (score < 15) ? 60u : (score < 40) ? 40u : 25u;
        if (spawn_tmr >= spawn_int) {
            spawn_tmr = 0;
            for (int i = 0; i < MAX_ASTROS; i++) {
                if (!a_act[i]) {
                    a_act[i] = 1;
                    a_x[i]   = 127;
                    a_y[i]   = (int8_t)(10 + rnd16(44));
                    a_r[i]   = (int8_t)(3  + rnd16(4));
                    a_spd[i] = (int8_t)(1  + rnd16(3));
                    break;
                }
            }
        }

        /* Collision: bullet vs asteroid */
        for (int b = 0; b < MAX_BULLETS; b++) {
            if (!b_act[b]) continue;
            for (int a = 0; a < MAX_ASTROS; a++) {
                if (!a_act[a]) continue;
                int dx = (int)b_x[b] - (int)a_x[a];
                int dy = (int)b_y[b] - (int)a_y[a];
                int rr = (int)a_r[a] + 2;
                if (dx*dx + dy*dy <= rr*rr) {
                    b_act[b] = 0;
                    a_act[a] = 0;
                    if (score < 9999) score++;
                }
            }
        }

        /* Scroll stars slowly left */
        if (frame % 3 == 0) {
            for (int i = 0; i < 8; i++)
                star_x[i] = (star_x[i] <= 1) ? 127 : star_x[i] - 1;
        }

        /* Draw frame */
        fb_clear();
        draw_stars();
        for (int i = 0; i < MAX_BULLETS; i++)
            if (b_act[i]) draw_bullet((int)b_x[i], (int)b_y[i]);
        for (int i = 0; i < MAX_ASTROS; i++)
            if (a_act[i]) draw_asteroid((int)a_x[i], (int)a_y[i], (int)a_r[i]);
        draw_ship((int)ship_y, (uint8_t)frame);
        draw_score();
        fb_flush();

        frame++;
    }
}
