#ifndef __BANANA_H
#define __BANANA_H

/* DANCE_Run — space shooter mini-game with arcade music.
 *
 * Ship auto-fires; player steers up/down to destroy asteroids.
 * Score (0000-9999) shown top-right.  Returns when BTN_PAGE pressed.
 *
 * BTN_AUX   : move ship UP
 * BTN_RESET : move ship DOWN
 * BTN_PAGE  : exit → return to power meter
 */
void DANCE_Run(void);

#endif /* __BANANA_H */
