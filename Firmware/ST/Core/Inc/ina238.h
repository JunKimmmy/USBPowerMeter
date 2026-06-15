#ifndef __INA238_H
#define __INA238_H

#include "stm32g0xx_hal.h"

/* 7-bit address 0x40 (1000000b, A0=GND A1=GND) shifted for HAL */
#define INA238_I2C_ADDR   (0x40 << 1)

/* Register addresses */
#define INA238_REG_CONFIG      0x00
#define INA238_REG_ADC_CONFIG  0x01
#define INA238_REG_SHUNT_CAL   0x02
#define INA238_REG_VSHUNT      0x04
#define INA238_REG_VBUS        0x05
#define INA238_REG_DIETEMP     0x06
#define INA238_REG_CURRENT     0x07
#define INA238_REG_POWER       0x08
#define INA238_REG_MFR_ID      0x3E
#define INA238_REG_DEVICE_ID   0x3F

#define INA238_MFR_ID_EXPECTED  0x5449u  /* "TI" */
#define INA238_SHUNT_MOHM       5        /* 5 mΩ shunt resistor on this PCB */

HAL_StatusTypeDef INA238_Init(void);
int32_t  INA238_ReadVBUS_mV(void);      /* bus voltage in mV  */
int32_t  INA238_ReadVSHUNT_uV(void);   /* shunt voltage in uV (ADCRANGE=1) */
int32_t  INA238_ReadCurrent_mA(void);  /* current in mA, computed from shunt */
int16_t  INA238_ReadTemp_degC(void);   /* die temperature in °C */

#endif /* __INA238_H */
