/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <string.h>
#include <stdio.h>
#include "uart_driver.h"
#include "kb.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

typedef enum {
	DIF_LVL_EASY = 0, DIF_LVL_MEDIUM = 1, DIF_LVL_HARD = 2
} DIFFICULT_LEVEL;

typedef enum {
	GAME_MODE_LED = 0, GAME_MODE_SOUND = 1, GAME_MODE_LED_AND_SOUND,
} GAME_MODE;

typedef enum {
	LED_COLOR_GREEN = 0, LED_COLOR_RED = 1, LED_COLOR_YELLOW = 2
} LED_COLOR;

typedef enum {
	LED_BRIGHTNESS_20 = 200,	// 20% of 1000
	LED_BRIGHTNESS_50 = 500,	// 50% of 1000
	LED_BRIGHTNESS_100 = 1000	// 100% of 1000
} LED_BRIGHTNESS;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define GAME_SEQUENCE_LEN 21
#define ANSWER_VARIANTS_LEN 9
#define NOTE_BASE_FREQ 100000
#define GAME_MODES_LEN 3
#define GAME_DIF_LVLS_LEN 3

#define KEY_BUFFER_LEN 8

#define PCA9538_INPUT_REGISTER 0x00
#define PCA9538_CONFIG_REGISTER 0x03
#define PCA9538_ADDR 0xE2

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

const uint32_t step_duration[3] = { 9999, 4999, 2499 }; // step durations in ms for easy, medium and hard LEVEL
const uint32_t note_T[ANSWER_VARIANTS_LEN] = {
NOTE_BASE_FREQ / 262,	// C
NOTE_BASE_FREQ / 294, 	// D
NOTE_BASE_FREQ / 330, 	// E
NOTE_BASE_FREQ / 349, 	// F
NOTE_BASE_FREQ / 392, 	// G
NOTE_BASE_FREQ / 440, 	// A
NOTE_BASE_FREQ / 494, 	// B
NOTE_BASE_FREQ / 523, 	// C+
NOTE_BASE_FREQ / 587	// D+
};
const LED_COLOR led_color[ANSWER_VARIANTS_LEN] = { LED_COLOR_GREEN,
		LED_COLOR_RED, LED_COLOR_YELLOW,

		LED_COLOR_GREEN, LED_COLOR_RED, LED_COLOR_YELLOW,

		LED_COLOR_GREEN, LED_COLOR_RED, LED_COLOR_YELLOW };
const LED_BRIGHTNESS led_brightness[ANSWER_VARIANTS_LEN] = { LED_BRIGHTNESS_20,
		LED_BRIGHTNESS_50, LED_BRIGHTNESS_100,

		LED_BRIGHTNESS_50, LED_BRIGHTNESS_100, LED_BRIGHTNESS_20,

		LED_BRIGHTNESS_100, LED_BRIGHTNESS_20, LED_BRIGHTNESS_50, };
const uint16_t keys_sequence[GAME_SEQUENCE_LEN] = { 1, 5, 0, 3, 7, 4, 3, 8, 2,
		5, 8, 2, 4, 6, 1, 0, 3, 6, 2, 8, 7 };

uint8_t current_step = 0;
uint8_t current_step_button_pressed = 0;
DIFFICULT_LEVEL dif_lvl = DIF_LVL_EASY;
GAME_MODE game_mode = GAME_MODE_LED_AND_SOUND;
uint8_t in_game = 0;

uint8_t keyboard_test_mode = 0;

uint8_t game_progress[GAME_SEQUENCE_LEN] = { 0 };

uint8_t keys_buffer[KEY_BUFFER_LEN];
uint8_t* keys_buffer_read_p;
uint8_t* keys_buffer_write_p;

// Messages
const char *MSG_GAME_DIFFICULTY_CHANGED[GAME_DIF_LVLS_LEN] = {
		"Game difficulty changed to easy\r\n",
		"Game difficulty changed to medium\r\n",
		"Game difficulty changed to hard\r\n" };
const char *MSG_GAME_MODE_CHANGED[GAME_MODES_LEN] = {
		"Game mode changed to 'only LEDs'\r\n",
		"Game mode changed to 'only sound'\r\n",
		"Game mode changed to 'LEDs and sound'\r\n" };

const char *MSG_GAME_STARTED = "Game is started\r\n";
const char *MSG_GAME_STOPPED = "Game is stopped\r\n";
const char *MSG_GAME_FINISHED = "Game is finished\r\n";

char *MSG_KEY_PRESSED = "Pressed key - #\r\n";
const uint8_t KEY_IN_MSG_KEY_PRESSED_INS_POS = 14;

const char *MSG_KEY_COLOR[ANSWER_VARIANTS_LEN] = {
		"Key LED color GREEN with brightness 20%\r\n",
		"Key LED color RED with brightness 50%\r\n",
		"Key LED color YELLOW with brightness 100%\r\n",
		"Key LED color GREEN with brightness 50%\r\n",
		"Key LED color RED with brightness 100%\r\n",
		"Key LED color YELLOW with brightness 20%\r\n",
		"Key LED color GREEN with brightness 100%\r\n",
		"Key LED color RED with brightness 20%\r\n",
		"Key LED color YELLOW with brightness 50%\r\n" };

const char *MSG_KEY_SOUND[ANSWER_VARIANTS_LEN] = {
		"Key sound frequency is 262\r\n", "Key sound frequency is 294\r\n",
		"Key sound frequency is 330\r\n", "Key sound frequency is 349\r\n",
		"Key sound frequency is 392\r\n", "Key sound frequency is 440\r\n",
		"Key sound frequency is 494\r\n", "Key sound frequency is 523\r\n",
		"Key sound frequency is 587\r\n" };

const char *MSG_GAME_RESULT = "Result:\r\n";
const char *MSG_KEY_CORRECT = "Correct\r\n";
const char *MSG_KEY_INCORRECT = "Fail\r\n";
char *MSG_CORRECT_ANSWER = "Key: # - ";
const uint8_t KEY_IN_MSG_CORRECT_ANSWER_INS_POS = 5;
const char *MSG_SCORE_TEMPLATE = "Score: %d of %d\r\n";

const char *MSG_KEYBOARD_TEST_MODE = "Keyboard test mode\r\n";
const char *MSG_APPLICATION_MODE = "Application mode\r\n";
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

void start_game();
void stop_game();

void switch_game_mode();
void switch_game_difficulty_lvl();
void start_or_stop_game();

void process_key(uint8_t key);

void set_color(uint8_t key);
void set_sound(uint8_t key);

uint8_t read_key_from_keys_buffer();
void write_key_to_keys_buffer(uint8_t key);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void set_color(uint8_t key) {
	uint16_t brightness = led_brightness[key];
	switch (led_color[key]) {
	case LED_COLOR_GREEN:
		htim4.Instance->CCR2 = brightness;
		htim4.Instance->CCR3 = 0;
		htim4.Instance->CCR4 = 0;
		break;
	case LED_COLOR_RED:
		htim4.Instance->CCR2 = 0;
		htim4.Instance->CCR3 = 0;
		htim4.Instance->CCR4 = brightness;
		break;
	case LED_COLOR_YELLOW:
		htim4.Instance->CCR2 = 0;
		htim4.Instance->CCR3 = brightness;
		htim4.Instance->CCR4 = 0;
		break;
	}
}

void set_sound(uint8_t key) {
	__HAL_TIM_SET_AUTORELOAD(&htim1, note_T[key]); // timer counter period
	htim1.Instance->CCR1 = note_T[key] / 2; // PWM Pulse (max sound 50%)
}

void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c == &hi2c1 && kb_state)
        kb_continue();
}

void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c == &hi2c1 && kb_state)
        kb_continue();
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if (htim->Instance == TIM6) {
		uint8_t key = 0;

		static uint8_t prev_confirmed_key = 0;
		static uint8_t prev_key = 0;
		uint8_t current_key = kb_read();

		// key is really pressed or it is just an contact bounce?
		if (current_key == prev_key){
			if (current_key != prev_confirmed_key){
				prev_confirmed_key = current_key;
				key = current_key;
			}
		} else
			prev_key = current_key;

		// if key isn't pressed
		if (!key)
			return;

		write_key_to_keys_buffer(key);
		return;
	}

	if (htim->Instance == TIM7) {
		if (in_game) {

			current_step++;
			current_step_button_pressed = 0;

			// if game ended
			if (current_step >= GAME_SEQUENCE_LEN) {
				stop_game();
			}

			uint16_t current_step_key = keys_sequence[current_step];

			// regulate color
			if (game_mode != GAME_MODE_SOUND)
				set_color(current_step_key);

			// regulate sound
			if (game_mode != GAME_MODE_LED)
				set_sound(current_step_key);
		} else {
			HAL_TIM_Base_Stop_IT(&htim7); // stop steps timer
			HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_2); // stop led timer
			HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_3); // stop led timer
			HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_4); // stop led timer
			HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1); // stop sound timer
		}
		return;
	}
}

	void start_game() {

		send_msg(MSG_GAME_STARTED);

		memset(game_progress, 0, GAME_SEQUENCE_LEN * sizeof(uint8_t));

		current_step = 0;
		current_step_button_pressed = 0;
		in_game = 1;

		uint8_t current_step_key = keys_sequence[current_step];

		// set steps timer
		__HAL_TIM_SET_AUTORELOAD(&htim7, step_duration[dif_lvl]);
		HAL_TIM_Base_Start_IT(&htim7);

		// set color timer
		if (game_mode != GAME_MODE_SOUND) {
			set_color(current_step_key);
			HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2);
			HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);
			HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);
		}

		// set sound timer
		if (game_mode != GAME_MODE_LED) {
			set_sound(current_step_key);
			HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
		}
	}

	void stop_game() {
		in_game = 0;

		HAL_TIM_Base_Stop_IT(&htim7); // stop steps timer

		if (game_mode != GAME_MODE_SOUND){
			HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_2); // stop led timer
			HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_3); // stop led timer
			HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_4); // stop led timer
		}

		if (game_mode != GAME_MODE_LED)
			HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1); // stop sound timer

		if (current_step >= GAME_SEQUENCE_LEN) {
			send_msg(MSG_GAME_FINISHED);
			send_msg(MSG_GAME_RESULT);

			int score = 0;
			for (int i = 0; i < GAME_SEQUENCE_LEN; i++) {
				MSG_CORRECT_ANSWER[KEY_IN_MSG_CORRECT_ANSWER_INS_POS] = '0'
						+ keys_sequence[i] + 1;
				send_msg(MSG_CORRECT_ANSWER);
				if (game_progress[i]) {
					send_msg(MSG_KEY_CORRECT);
					score++;
				} else
					send_msg(MSG_KEY_INCORRECT);
			}

			char msg[32] = { 0 };
			sprintf(msg, MSG_SCORE_TEMPLATE, score, GAME_SEQUENCE_LEN);
			send_msg(msg);
		} else {
			send_msg(MSG_GAME_STOPPED);
		}
	}

	void switch_game_mode() {
		if (in_game == 0) {
			game_mode = (game_mode + 1) % GAME_MODES_LEN;

			send_msg(MSG_GAME_MODE_CHANGED[game_mode]);
		}
	}

	void switch_game_difficulty_lvl() {
		if (in_game == 0) {
			dif_lvl = (dif_lvl + 1) % GAME_DIF_LVLS_LEN;

			send_msg(MSG_GAME_DIFFICULTY_CHANGED[dif_lvl]);
		}
	}

	void start_or_stop_game() {
		if (in_game)
			stop_game();
		else
			start_game();
	}

	void process_key(uint8_t key) {
		if (keyboard_test_mode){
			char msg[5] = {0};
			sprintf(msg, "%d\r\n", (int)key);
			send_msg(msg); // send notification about pressed button
			return;
		}

		switch (key) {
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
			MSG_KEY_PRESSED[KEY_IN_MSG_KEY_PRESSED_INS_POS] = '0' + key; // change key number in message
			send_msg(MSG_KEY_PRESSED); // send notification about pressed button


			if (in_game) {
				if (current_step_button_pressed){
					game_progress[current_step] = 0;
				}else{
					if (key - 1 == keys_sequence[current_step]) // current key is correct
						game_progress[current_step] = 1;
					else
						game_progress[current_step] = 0;

					current_step_button_pressed = 1;
				}
			} else {
				send_msg(MSG_KEY_COLOR[key - 1]);
				send_msg(MSG_KEY_SOUND[key - 1]);

				set_color(key - 1);
				set_sound(key - 1);

				HAL_TIM_Base_Start_IT(&htim7); // start steps timer
				HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2); 	// start led timer
				HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3); 	// start led timer
				HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4); 	// start led timer
				HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1); 	// start sound timer
			}
			break;
		case 10:
			switch_game_difficulty_lvl();
			break;
		case 11:
			switch_game_mode();
			break;
		case 12:
			start_or_stop_game();
			break;
		}
	}

	uint8_t read_key_from_keys_buffer() {
		// critical section
		uint32_t primask_bit = __get_PRIMASK(); /**< backup PRIMASK bit */
		__disable_irq(); /**< Disable all interrupts by setting PRIMASK bit on Cortex*/

		uint8_t key = *(keys_buffer_read_p++);
		if (keys_buffer_read_p >= keys_buffer + KEY_BUFFER_LEN)
			keys_buffer_read_p = keys_buffer;

		__set_PRIMASK(primask_bit); /**< Restore PRIMASK bit*/

		return key;
	}

	void write_key_to_keys_buffer(uint8_t key) {
		// critical section
		uint32_t primask_bit = __get_PRIMASK(); /**< backup PRIMASK bit */
		__disable_irq(); /**< Disable all interrupts by setting PRIMASK bit on Cortex*/

		*(keys_buffer_write_p++) = key;

		if (keys_buffer_write_p >= keys_buffer + KEY_BUFFER_LEN)
			keys_buffer_write_p = keys_buffer;

		__set_PRIMASK(primask_bit); /**< Restore PRIMASK bit*/
	}

	uint8_t is_button_pressed()
	{
		static uint8_t btn_pressed = 0;

		if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_15) == GPIO_PIN_RESET)
		{
			if (!btn_pressed)
				btn_pressed = 1;
		}
		else if (btn_pressed)
		{
			btn_pressed = 0;
			return 1;
		}

		return 0;
	}

	/* USER CODE END 0 */

	/**
	 * @brief  The application entry point.
	 * @retval int
	 */
	int main(void) {
		/* USER CODE BEGIN 1 */

		/* USER CODE END 1 */

		/* MCU Configuration--------------------------------------------------------*/

		/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
		HAL_Init();

		/* USER CODE BEGIN Init */

		/* USER CODE END Init */

		/* Configure the system clock */
		SystemClock_Config();

		/* USER CODE BEGIN SysInit */

		/* USER CODE END SysInit */

		/* Initialize all configured peripherals */
		MX_GPIO_Init();
		MX_I2C1_Init();
		MX_TIM6_Init();
		MX_TIM7_Init();
		MX_TIM1_Init();
		MX_TIM4_Init();
		/* USER CODE BEGIN 2 */

		/* USER CODE END 2 */

		/* Infinite loop */
		/* USER CODE BEGIN WHILE */
		keys_buffer_read_p = keys_buffer;
		keys_buffer_write_p = keys_buffer;
		HAL_TIM_Base_Start_IT(&htim6);
		uint8_t key;
		while (1) {
			/* USER CODE END WHILE */

			/* USER CODE BEGIN 3 */

			if (is_button_pressed()){
				if (keyboard_test_mode){
					keyboard_test_mode = 0;
					send_msg(MSG_APPLICATION_MODE);
				} else {
					if (in_game)
						stop_game();

					send_msg(MSG_KEYBOARD_TEST_MODE);
					keyboard_test_mode = 1;
				}
			}


			key = 0;

			if (keys_buffer_read_p != keys_buffer_write_p)
				key = read_key_from_keys_buffer();

			if (key != 0)
				process_key(key);
			else
				HAL_Delay(100);
		}
		/* USER CODE END 3 */
	}

	/**
	 * @brief System Clock Configuration
	 * @retval None
	 */
	void SystemClock_Config(void) {
		RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
		RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

		/** Configure the main internal regulator output voltage
		 */
		__HAL_RCC_PWR_CLK_ENABLE();
		__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
		/** Initializes the RCC Oscillators according to the specified parameters
		 * in the RCC_OscInitTypeDef structure.
		 */
		RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
		RCC_OscInitStruct.HSEState = RCC_HSE_ON;
		RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
		RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
		RCC_OscInitStruct.PLL.PLLM = 16;
		RCC_OscInitStruct.PLL.PLLN = 192;
		RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
		RCC_OscInitStruct.PLL.PLLQ = 4;
		if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
			Error_Handler();
		}
		/** Initializes the CPU, AHB and APB buses clocks
		 */
		RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
				| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
		RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
		RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
		RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
		RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

		if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4)
				!= HAL_OK) {
			Error_Handler();
		}
	}

	/* USER CODE BEGIN 4 */

	/* USER CODE END 4 */

	/**
	 * @brief  This function is executed in case of error occurrence.
	 * @retval None
	 */
	void Error_Handler(void) {
		/* USER CODE BEGIN Error_Handler_Debug */
		/* User can add his own implementation to report the HAL error return state */
		__disable_irq();
		while (1) {
		}
		/* USER CODE END Error_Handler_Debug */
	}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

	/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
