#ifndef __BOOT_H
#define __BOOT_H

/* BOOT_Run — one-shot boot screen.
 * Displays "USB POWER METER / V1" header and animates a sweeping
 * power waveform across the bottom half, then clears and returns
 * so the caller can enter the main power-meter loop. */
void BOOT_Run(void);

#endif /* __BOOT_H */
