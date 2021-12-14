#ifndef INC_KEYBOARD_H_
#define INC_KEYBOARD_H_

HAL_StatusTypeDef kb_continue(void);
uint8_t kb_read(void);

extern int kb_state;

#endif /* INC_KEYBOARD_H_ */
