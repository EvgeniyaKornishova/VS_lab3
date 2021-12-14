#include "main.h"
#include "i2c.h"
#include "pca9538.h"

HAL_StatusTypeDef PCA9538_Read_Register(uint16_t addr, pca9538_regs_t reg,
		uint8_t *buf) {
	HAL_StatusTypeDef retcode;
	while ((retcode = HAL_I2C_Mem_Read_IT(&hi2c1, addr | 1, reg, 1, buf, 1))
			== HAL_BUSY)
		;
	return retcode;
}

HAL_StatusTypeDef PCA9538_Write_Register(uint16_t addr, pca9538_regs_t reg,
		uint8_t *buf) {
	HAL_StatusTypeDef retcode;
	while ((retcode = HAL_I2C_Mem_Write_IT(&hi2c1, addr & 0xFFFE, reg, 1, buf,
			1)) == HAL_BUSY)
		;
	return retcode;
}

HAL_StatusTypeDef PCA9538_Read_Inputs(uint16_t addr, uint8_t *buf) {
	return PCA9538_Read_Register(addr, INPUT_PORT, buf);
}
