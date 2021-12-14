#include "main.h"
#include "pca9538.h"
#include "kb.h"
#include "utils.h"

#define KBRD_RD_ADDR 0xE3
#define KBRD_WR_ADDR 0xE2
#define ROW1 0x1E
#define ROW2 0x3D
#define ROW3 0x7B
#define ROW4 0xF7

int kb_state = 0;
uint8_t kb_result = 0;
uint8_t kb_current_row = 0;

HAL_StatusTypeDef kb_continue(void) {
	static uint8_t buf[4];
	uint8_t key;
	uint8_t kb_in;

	switch (kb_state) {
	case 0:
		buf[0] = 0x70;
		buf[1] = buf[2] = buf[3] = 0;

		kb_state = 0;
		CHK_HAL(PCA9538_Write_Register(KBRD_WR_ADDR, CONFIG, buf))
		kb_state = 1;

		return HAL_OK;
	case 1:
		buf[0] = 0;

		kb_state = 0;
		CHK_HAL(PCA9538_Write_Register(KBRD_WR_ADDR, OUTPUT_PORT, buf))
		kb_state = 2;

		return HAL_OK;
	case 2:
		buf[0] = kb_current_row;

		kb_state = 0;
		CHK_HAL(PCA9538_Write_Register(KBRD_WR_ADDR, OUTPUT_PORT, buf))
		kb_state = 3;

		return HAL_OK;
	case 3:
		buf[0] = 0;

		kb_state = 0;
		CHK_HAL(PCA9538_Read_Inputs(KBRD_RD_ADDR, buf))
		kb_state = 4;

		break;
	case 4:
		kb_state = 0;
		kb_in = buf[0] & 0x70;
		key = 0;
		if (!(kb_in & 0x10))
			key |= 0x04;
		if (!(kb_in & 0x20))
			key |= 0x02;
		if (!(kb_in & 0x40))
			key |= 0x01;

		kb_result = key;
		break;
	}
	return HAL_OK;
}

uint8_t kb_read(void)
{
    static uint8_t const rows[4] = {0xF7, 0x7B, 0x3D, 0x1E};
    static int current_row = 0;
    static int row_result[4] = {0, 0, 0, 0};

    if (kb_state == 0)
    {
        if (row_result[current_row] != kb_result)
        {
            if (kb_result & 1)
                return 3 * current_row + 1;
            if (kb_result & 2)
                return 3 * current_row + 2;
            if (kb_result & 4)
                return 3 * current_row + 3;
        }

        row_result[current_row] = kb_result;
        current_row = (current_row + 1) % 4;
        kb_current_row = rows[current_row];
        kb_continue();
    }
    return 0;
}
