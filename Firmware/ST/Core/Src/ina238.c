#include "ina238.h"
#include "main.h"

extern I2C_HandleTypeDef hi2c2;

static HAL_StatusTypeDef reg_read(uint8_t reg, uint16_t *out)
{
    uint8_t buf[2];
    HAL_StatusTypeDef s = HAL_I2C_Mem_Read(&hi2c2, INA238_I2C_ADDR,
                                             reg, I2C_MEMADD_SIZE_8BIT,
                                             buf, 2, 100);
    if (s == HAL_OK)
        *out = ((uint16_t)buf[0] << 8) | buf[1];
    return s;
}

static HAL_StatusTypeDef reg_write(uint8_t reg, uint16_t val)
{
    uint8_t buf[2] = {(uint8_t)(val >> 8), (uint8_t)(val & 0xFF)};
    return HAL_I2C_Mem_Write(&hi2c2, INA238_I2C_ADDR,
                              reg, I2C_MEMADD_SIZE_8BIT,
                              buf, 2, 100);
}

HAL_StatusTypeDef INA238_Init(void)
{
    /* 1. Confirm device ACKs on bus */
    if (HAL_I2C_IsDeviceReady(&hi2c2, INA238_I2C_ADDR, 5, 200) != HAL_OK)
        return HAL_ERROR;

    /* 2. Read MANUFACTURER_ID — must be 0x5449 ("TI") */
    uint16_t mfr;
    if (reg_read(INA238_REG_MFR_ID, &mfr) != HAL_OK)
        return HAL_ERROR;
    if (mfr != INA238_MFR_ID_EXPECTED)
        return HAL_ERROR;

    /* 3. Software reset to restore defaults, then set ADCRANGE=1 (±40.96mV, 1.25uV/LSB) */
    reg_write(INA238_REG_CONFIG, 0x8000);   /* RST bit — self-clears */
    HAL_Delay(1);
    reg_write(INA238_REG_CONFIG, 0x0010);   /* ADCRANGE=1 */

    /* ADC_CONFIG default 0xFB68: continuous all, 1052us conv time, 1x avg — keep as-is */

    return HAL_OK;
}

int32_t INA238_ReadVBUS_mV(void)
{
    uint16_t raw;
    if (reg_read(INA238_REG_VBUS, &raw) != HAL_OK)
        return -1;
    /* 3.125 mV/LSB = 3125/1000 */
    return (int32_t)(int16_t)raw * 3125 / 1000;
}

int32_t INA238_ReadVSHUNT_uV(void)
{
    uint16_t raw;
    if (reg_read(INA238_REG_VSHUNT, &raw) != HAL_OK)
        return 0;
    /* ADCRANGE=1: 1.25 uV/LSB = 125/100 */
    return (int32_t)(int16_t)raw * 125 / 100;
}

int32_t INA238_ReadCurrent_mA(void)
{
    /* I (mA) = Vshunt (µV) / Rshunt (mΩ) */
    return INA238_ReadVSHUNT_uV() / INA238_SHUNT_MOHM;
}

int16_t INA238_ReadTemp_degC(void)
{
    uint16_t raw;
    if (reg_read(INA238_REG_DIETEMP, &raw) != HAL_OK)
        return 0;
    /* Temperature in bits [15:4], 125 m°C/LSB → divide by 8 for °C */
    return (int16_t)((int16_t)raw >> 4) / 8;
}
