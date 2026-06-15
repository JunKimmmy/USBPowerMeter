#ifndef __METER_H
#define __METER_H

/* METER_Run — main power-meter UI loop (never returns).
 * Cycles between three screens via BTN_PAGE:
 *   0  Main meter   — large 2x V / I / P readings + running average footer
 *   1  Current graph — rolling line graph with avg / max overlay
 *   2  Power graph   — rolling line graph with avg / max overlay
 * BTN_AUX on graph screens resets graph history.
 * BTN_RESET enters the banana space-shooter Easter egg.
 */
void METER_Run(void);

#endif /* __METER_H */
