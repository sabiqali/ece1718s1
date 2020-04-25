#ifndef AUX_FUNCTIONS_H
#define AUX_FUNCTIONS_H

void Pinmux_Config(void);
void I2C0_Init(void);
void ADXL345_REG_WRITE(uint8_t, uint8_t);
void ADXL345_REG_READ(uint8_t, uint8_t *);
void ADXL345_REG_MULTI_READ(uint8_t, uint8_t[], uint8_t);
void ADXL345_Init(void);
void ADXL345_Calibrate(void);
bool ADXL345_WasActivityUpdated(void);
bool ADXL345_IsDataReady(void);
void ADXL345_XYZ_Read(int16_t[]);
void ADXL345_IdRead(uint8_t *);
void format_read_data(void);
int check_I2C(void);

#endif /*AUX_FUNCTIONS_H*/
