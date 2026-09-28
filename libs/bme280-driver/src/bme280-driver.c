#include "bme280-driver.h"
#include "bme280-regs.h"
#include <stdio.h>

static bme280_ctx_t bme280_ctx = {0};

void bme280_read_calibration()
{
    uint8_t buf1[26];
    bme280_read_regs(0x88, buf1, 26);

    calib.dig_T1 = (buf1[1] << 8) | buf1[0];
    calib.dig_T2 = (buf1[3] << 8) | buf1[2];
    calib.dig_T3 = (buf1[5] << 8) | buf1[4];

    calib.dig_P1 = (buf1[7] << 8) | buf1[6];
    calib.dig_P2 = (buf1[9] << 8) | buf1[8];
    calib.dig_P3 = (buf1[11] << 8) | buf1[10];
    calib.dig_P4 = (buf1[13] << 8) | buf1[12];
    calib.dig_P5 = (buf1[15] << 8) | buf1[14];
    calib.dig_P6 = (buf1[17] << 8) | buf1[16];
    calib.dig_P7 = (buf1[19] << 8) | buf1[18];
    calib.dig_P8 = (buf1[21] << 8) | buf1[20];
    calib.dig_P9 = (buf1[23] << 8) | buf1[22];

    calib.dig_H1 = buf1[25];

    uint8_t buf2[7];
    bme280_read_regs(0xE1, buf2, 7);

    calib.dig_H2 = (buf2[1] << 8) | buf2[0];
    calib.dig_H3 = buf2[2];
    calib.dig_H4 = (buf2[3] << 4) | (buf2[4] & 0x0F);
    calib.dig_H5 = (buf2[5] << 4) | (buf2[4] >> 4);
    calib.dig_H6 = (int8_t)buf2[6];
}

void bme280_init(bme280_i2c_read i2c_read, bme280_i2c_write i2c_write)
{
    bme280_ctx.i2c_read = i2c_read;
    bme280_ctx.i2c_write = i2c_write;

    uint8_t id_reg_buf[1] = {0};
    bme280_read_regs(BME280_REG_ID, id_reg_buf, sizeof(id_reg_buf));
    if (id_reg_buf[0] != 0x60)
    {
        printf("Error: Device not found or wrong device\n");
        // while(1);
    }

    uint8_t ctrl_hum_reg_value = 0;
    ctrl_hum_reg_value |= (0b001 << 0); // osrs_h[2:0] = oversampling 1
    bme280_write_reg(BME280_REG_CTRL_HUM, ctrl_hum_reg_value);

    uint8_t config_reg_value = 0;
    config_reg_value |= (0b0 << 0); // spi3w_en[0:0] = false
    config_reg_value |= (0b000 << 2); // filter[4:2] = Filter off
    config_reg_value |= (0b001 << 5); // t_sb[7:5] = 62.5 ms
    bme280_write_reg(BME280_REG_CONFIG, config_reg_value);

    uint8_t ctrl_meas_reg_value = 0;
    ctrl_meas_reg_value |= (0b001 << 5); // osrs_t[2:0] = oversampling 1
    ctrl_meas_reg_value |= (0b001 << 2); // osrs_p[2:3] = oversampling 1
    ctrl_meas_reg_value |= (0b11 << 0); // mode[7:5] = normal mode
    bme280_write_reg(BME280_REG_CTRL_MEAS, ctrl_meas_reg_value);  

    bme280_read_calibration();
}


void bme280_read_regs(uint8_t start_reg_address, uint8_t* buffer, uint8_t length)
{
    uint8_t data[1] = {start_reg_address};
    bme280_ctx.i2c_write(data, sizeof(data));
    bme280_ctx.i2c_read(buffer, length);
    
}

void bme280_write_reg(uint8_t reg_address, uint8_t value)
{
    uint8_t data[2] = {reg_address, value};
    bme280_ctx.i2c_write(data, sizeof(data));
    
}

// температура и давление 20-битные: MSB, LSB и старшие 4 бита XLSB
uint32_t bme280_read_temp_raw()
{
	uint8_t read[3] = {0};
	bme280_read_regs(BME280_REG_TEMP_MSB, read, sizeof(read));
	uint32_t value = ((uint32_t)read[0] << 12) | ((uint32_t)read[1] << 4) | ((uint32_t)read[2] >> 4);
	return value;
}

uint32_t bme280_read_pressure_raw()
{
    uint8_t read[3] = {0};
    bme280_read_regs(BME280_REG_PRESS_MSB, read, sizeof(read));
    uint32_t value = ((uint32_t)read[0] << 12) | ((uint32_t)read[1] << 4) | ((uint32_t)read[2] >> 4);
    return value;
}

uint16_t bme280_read_humidity_raw()
{
    uint8_t read[2] = {0};
    bme280_read_regs(BME280_REG_HUM_MSB, read, sizeof(read));
    uint16_t value = ((uint16_t)read[0] << 8) | ((uint16_t)read[1]);
    return value;
}

float bme280_get_temperature()
{
    int32_t adc_T = bme280_read_temp_raw();

    float var1 = (adc_T / 16384.0 - calib.dig_T1 / 1024.0) * calib.dig_T2;
    float var2 = ((adc_T / 131072.0 - calib.dig_T1 / 8192.0) *
                 (adc_T / 131072.0 - calib.dig_T1 / 8192.0)) * calib.dig_T3;

    t_fine = (int32_t)(var1 + var2);
    return (var1 + var2) / 5120.0;
}

float bme280_get_pressure()
{
    int32_t adc_P = bme280_read_pressure_raw();

    float var1 = t_fine / 2.0 - 64000.0;
    float var2 = var1 * var1 * calib.dig_P6 / 32768.0;
    var2 = var2 + var1 * calib.dig_P5 * 2.0;
    var2 = var2 / 4.0 + calib.dig_P4 * 65536.0;
    var1 = (calib.dig_P3 * var1 * var1 / 524288.0 + calib.dig_P2 * var1) / 524288.0;
    var1 = (1.0 + var1 / 32768.0) * calib.dig_P1;

    if (var1 == 0) return 0;

    float p = 1048576.0 - adc_P;
    p = (p - var2 / 4096.0) * 6250.0 / var1;
    var1 = calib.dig_P9 * p * p / 2147483648.0;
    var2 = p * calib.dig_P8 / 32768.0;

    p = p + (var1 + var2 + calib.dig_P7) / 16.0;
    return p;
}

float bme280_get_humidity()
{
    int32_t adc_H = bme280_read_humidity_raw();

    float h = t_fine - 76800.0;

    h = (adc_H - (calib.dig_H4 * 64.0 + calib.dig_H5 / 16384.0 * h)) *
        (calib.dig_H2 / 65536.0 *
        (1.0 + calib.dig_H6 / 67108864.0 * h *
        (1.0 + calib.dig_H3 / 67108864.0 * h)));

    h = h * (1.0 - calib.dig_H1 * h / 524288.0);

    if (h > 100.0) h = 100.0;
    if (h < 0.0) h = 0.0;

    return h;
}