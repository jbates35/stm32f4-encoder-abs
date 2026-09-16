#include "stm32f446xx_i2c_1602.h"

// TODO: setup_1602_lcd_peripherals should use return codes
// TODO: Implement config variables in setup_1602_lcd_peripherals

I2C1602StatusCode_t setup_1602_lcd_peripherals(LCD1602RuntimeConfig_t cfg) {
  // First, timer setup
  timer_peri_clock_control(I2C_1602_TIM_PORT, 1);
  TimerHandle_t i2c_tim_handle = {.cfg = {.channel_1 = {.channel_mode = TIMER_CHANNEL_MODE_COMPARE,
                                                        .gpio_en = TIMER_DISABLE,
                                                        .ccr = 0,
                                                        .interrupt_en = TIMER_ENABLE},
                                          .one_shot_enabled = TIMER_DISABLE,
                                          .start_enabled = TIMER_DISABLE,
                                          .channel_count = 1,
                                          .direction = TIMER_DIR_UP,
                                          .arr = 0xF062,
                                          .prescaler = 12},
                                  .p_base_addr = I2C_1602_TIM_PORT};
  timer_init(&i2c_tim_handle);
  NVIC_EnableIRQ(I2C_1602_TIM_IRQN);

  // Then setup GPIO for I2C module
  GPIO_peri_clock_control(I2C_1602_GPIO_PORT, GPIO_CLOCK_ENABLE);
  GPIOConfig_t default_gpio_cfg = {.mode = GPIO_MODE_ALTFN,
                                   .speed = GPIO_SPEED_MEDIUM,
                                   .float_resistor = GPIO_PUPDR_NONE,
                                   .output_type = GPIO_OP_TYPE_OPENDRAIN,
                                   .alt_func_num = 4};

  GPIOHandle_t i2c_sda_handle = {.p_GPIO_addr = I2C_1602_GPIO_PORT, .cfg = default_gpio_cfg};
  i2c_sda_handle.cfg.pin_number = I2C_1602_GPIO_SDA_PIN;
  GPIO_init(&i2c_sda_handle);

  GPIOHandle_t i2c_scl_handle = {.p_GPIO_addr = I2C_1602_GPIO_PORT, .cfg = default_gpio_cfg};
  i2c_scl_handle.cfg.pin_number = I2C_1602_GPIO_SCL_PIN;
  GPIO_init(&i2c_scl_handle);

  // Setup DMA for I2C module
  DMAHandle_t dma_tx_handle = {
      .stream_addr = I2C_1602_DMA_STREAM,
      .cfg = {.in = {.addr = NULL, .type = DMA_IO_TYPE_MEMORY, .inc = DMA_IO_ARR_INCREMENT},
              .out = {.addr = &I2C_1602_PORT->DR, .type = DMA_IO_TYPE_PERIPHERAL, .inc = DMA_IO_ARR_STATIC},
              .mem_data_size = DMA_DATA_SIZE_8_BIT,
              .peri_data_size = DMA_DATA_SIZE_8_BIT,
              .dma_elements = 0,
              .channel = I2C_1602_DMA_CHANNEL,
              .priority = DMA_PRIORITY_HIGH,
              .circ_buffer = DMA_BUFFER_FINITE,
              .flow_control = DMA_PERIPH_NO_FLOW_CONTROL,
              .interrupt_en =
                  {
                      .direct_mode_error = DMA_DISABLE,
                      .transfer_error = DMA_DISABLE,
                      .full_transfer = DMA_ENABLE,
                      .half_transfer = DMA_DISABLE,
                  },
              .start_enabled = DMA_DISABLE},
  };
  dma_peri_clock_control(I2C_1602_DMA_PORT, DMA_ENABLE);
  dma_stream_init(&dma_tx_handle);

  // Setup I2C module lastly
  I2CHandle_t i2c_handle = {.addr = I2C_1602_PORT,
                            .cfg = {.peri_clock_freq_hz = (uint32_t)16E6,
                                    .device_mode = I2C_DEVICE_MODE_MASTER,
                                    .scl_mode = I2C_SCL_MODE_SPEED_SM,
                                    .interrupt_enable = I2C_ENABLE,
                                    .dma_enable = I2C_DISABLE,
                                    .enable_on_init = I2C_ENABLE}};
  i2c_peri_clock_control(I2C_1602_PORT, I2C_ENABLE);
  i2c_init(&i2c_handle);

  return I2C_1602_STATUS_OK;
}

// TODO: Implement - this takes care of actually setting up the bytes and what not to clear screen and get it ready for
// messages
I2C1602StatusCode_t setup_1602_lcd_screen() { return I2C_1602_STATUS_OK; }
