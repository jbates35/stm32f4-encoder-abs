
#ifndef INC_STM32F446XX_I2C_1602_H_
#define INC_STM32F446XX_I2C_1602_H_

#include <stdint.h>
#include <stdio.h>

#include "stm32f446xx.h"
#include "stm32f446xx_dma.h"
#include "stm32f446xx_gpio.h"
#include "stm32f446xx_i2c.h"
#include "stm32f446xx_tim.h"

#define I2C_1602_GPIO_PORT GPIOB
#define I2C_1602_GPIO_SCL_PIN 8
#define I2C_1602_GPIO_SDA_PIN 9
#define I2C_1602_PORT I2C1
#define I2C_1602_PORT_EV_IRQN I2C1_EV_IRQn
#define I2C_1602_PORT_EV_IRQ_HANDLER I2C1_EV_IRQHandler
#define I2C_1602_PORT_ERR_IRQN I2C1_ER_IRQn
#define I2C_1602_PORT_ERR_IRQ_HANDLER I2C1_ER_IRQHandler

#define I2C_1602_DMA_PORT DMA1
#define I2C_1602_DMA_STREAM DMA1_Stream6
#define I2C_1602_DMA_CHANNEL 1
#define I2C_1602_DMA_STREAM_IRQN DMA1_Stream6_IRQn
#define I2C_1602_DMA_STREAM_IRQ_HANDLER DMA1_Stream6_IRQHandler

#define I2C_1602_TIM_PORT TIM5
#define I2C_1602_TIM_IRQN TIM5_IRQn
#define I2C_1602_TIM_IRQ_HANDLER TIM5_IRQHandler

typedef enum { I2C_1602_STATUS_OK = 0, I2C_1602_BAD_PERIPHERAL_SETUP } I2C1602StatusCode_t;

typedef struct {
  int mcu_freq_hz;
  uint8_t lcd_freq_hz;
  uint8_t* line1_arr;
  uint8_t line1_len;
  uint8_t* line2_arr;
  uint8_t line2_len;
} LCD1602RuntimeConfig_t;

// Step 1:
I2C1602StatusCode_t setup_1602_lcd_peripherals(LCD1602RuntimeConfig_t cfg);

// Step 2:
I2C1602StatusCode_t setup_1602_lcd_screen();
#endif
