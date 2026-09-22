
#ifndef INC_STM32F446XX_I2C_1602_H_
#define INC_STM32F446XX_I2C_1602_H_

#include <stdint.h>

#include "stm32f446xx.h"

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

typedef enum {
  I2C_1602_STATUS_OK = 0,
  I2C_1602_BAD_PERIPHERAL_SETUP,
  I2C_1602_ERROR_IN_SETUP,
  I2C_1602_NULL_PTRS,
  I2C_1602_NO_LENGTH
} I2C1602StatusCode_t;

// Helper function for things like encoder counts and what not
void convert_uint32_to_str(void* arr, int len, uint32_t num);

// Step 1:
I2C1602StatusCode_t setup_1602_lcd_peripherals(uint32_t mcu_freq_hz, uint8_t lcd_freq_hz);

// Step 2:
I2C1602StatusCode_t setup_1602_lcd_screen(void);

// Setp 3:
I2C1602StatusCode_t set_1602_lcd_str(const void* buff1, uint8_t len1, const void* buff2, uint8_t len2);

// Step 4
void enable_1602_lcd_updating(void);
void disable_1602_lcd_updating(void);

#endif
