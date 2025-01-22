/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.h
 * @brief          : Header for main.c file.
 *                   This file contains the common defines of the application.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define OTG_FS_PowerSwitchOn_Pin GPIO_PIN_0
#define OTG_FS_PowerSwitchOn_GPIO_Port GPIOC
#define USER_BUTTON_Pin GPIO_PIN_0
#define USER_BUTTON_GPIO_Port GPIOA
#define USER_BUTTON_EXTI_IRQn EXTI0_IRQn
#define KEYBOARD_C1_Pin GPIO_PIN_7
#define KEYBOARD_C1_GPIO_Port GPIOE
#define KEYBOARD_C1_EXTI_IRQn EXTI9_5_IRQn
#define KEYBOARD_R4_Pin GPIO_PIN_8
#define KEYBOARD_R4_GPIO_Port GPIOE
#define KEYBOARD_C2_Pin GPIO_PIN_9
#define KEYBOARD_C2_GPIO_Port GPIOE
#define KEYBOARD_C2_EXTI_IRQn EXTI9_5_IRQn
#define KEYBOARD_R3_Pin GPIO_PIN_10
#define KEYBOARD_R3_GPIO_Port GPIOE
#define KEYBOARD_C3_Pin GPIO_PIN_11
#define KEYBOARD_C3_GPIO_Port GPIOE
#define KEYBOARD_C3_EXTI_IRQn EXTI15_10_IRQn
#define KEYBOARD_R2_Pin GPIO_PIN_12
#define KEYBOARD_R2_GPIO_Port GPIOE
#define KEYBOARD_C4_Pin GPIO_PIN_13
#define KEYBOARD_C4_GPIO_Port GPIOE
#define KEYBOARD_C4_EXTI_IRQn EXTI15_10_IRQn
#define KEYBOARD_R1_Pin GPIO_PIN_14
#define KEYBOARD_R1_GPIO_Port GPIOE
#define LD4_GREEN_Pin GPIO_PIN_12
#define LD4_GREEN_GPIO_Port GPIOD
#define LD3_ORANGE_Pin GPIO_PIN_13
#define LD3_ORANGE_GPIO_Port GPIOD
#define LD5_RED_Pin GPIO_PIN_14
#define LD5_RED_GPIO_Port GPIOD
#define LD6_BLUE_Pin GPIO_PIN_15
#define LD6_BLUE_GPIO_Port GPIOD
#define I2C_LCD_DATA_Pin GPIO_PIN_9
#define I2C_LCD_DATA_GPIO_Port GPIOC
#define I2C_LCD_CLOCK_Pin GPIO_PIN_8
#define I2C_LCD_CLOCK_GPIO_Port GPIOA
#define AUDIO_RST_Pin GPIO_PIN_4
#define AUDIO_RST_GPIO_Port GPIOD
#define I2C_AUDIO_CLOCK_Pin GPIO_PIN_6
#define I2C_AUDIO_CLOCK_GPIO_Port GPIOB
#define I2C_AUDIO_DATA_Pin GPIO_PIN_9
#define I2C_AUDIO_DATA_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
