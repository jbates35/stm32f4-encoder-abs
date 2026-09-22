
#include "stm32f446xx_i2c_1602.h"

#include <stdio.h>
#include <string.h>

#include "stm32f446xx_dma.h"
#include "stm32f446xx_gpio.h"
#include "stm32f446xx_i2c.h"
#include "stm32f446xx_tim.h"

// START OF 1602 I2C MACROS
#define LCD_I2C_ADDR_VDD 0x27  // When A0/1/2 are all HIGH

// START OF 1602 LCD MACROS
#define LCD_CLEAR_DISPLAY 0x1
#define LCD_RETURN_HOME 0x2
#define LCD_ENTRY_MODE_MASK 0x4
#define LCD_ENTRY_MODE_INC 0x2
#define LCD_ENTRY_MODE_SHIFT 0x1
#define LCD_DISPLAY_CFG_MASK 0x8
#define LCD_DISPLAY_ON_MASK 0x4
#define LCD_CURSOR_ON_MASK 0x2
#define LCD_BLINK_ON_MASK 0x1
#define LCD_DISPLAY_CURSOR_LEFT 0x18
#define LCD_DISPLAY_CURSOR_RIGHT 0x1C
#define LCD_SHIFT_CURSOR_LEFT 0x10
#define LCD_SHIFT_CURSOR_RIGHT 0x14
#define LCD_FUNCTION_MASK 0x20
#define LCD_DATA_LENGTH_MASK 0x10
#define LCD_DISPLAY_TWO_LINES 0x8
#define LCD_CHAR_FONT_MASK 0x4
#define LCD_SET_CGRAM_ADDR_MASK 0x40
#define LCD_SET_DDRAM_ADDR_MASK 0x80
#define LCD_RS_OFF_MASK 0x0
#define LCD_RS_ON_MASK 0x1
#define LCD_RW_WRITE_MASK 0x0
#define LCD_RW_READ_MASK 0x2
#define LCD_CLOCK_HIGH_MASK 0x4
#define LCD_CLOCK_LOW_MASK 0
#define LCD_BACKLIGHT_ON_MASK 0x8
#define LCD_BACKLIGHT_OFF_MASK 0
#define LCD_JUMP_FIRST_LINE 0x80   // Used to set DDRAM to first line
#define LCD_JUMP_SECOND_LINE 0xC0  // Used to set DDRAM to second line

#define SIZEOF(arr) ((unsigned int)sizeof(arr) / sizeof(arr[0]))

#define VERY_FAST 16
#define FAST 10000
#define MEDIUM 300000
#define SLOW 1000000
#define WAIT(CNT)                                          \
  do {                                                     \
    for (int sleep_cnt = 0; sleep_cnt < CNT; sleep_cnt++); \
  } while (0)

typedef enum { LCD_RS_INST_WR = 0, LCD_RS_DDR_WR = 1 } lcd_rs_type_t;

typedef struct {
  uint8_t buff[136];
  int len;
  uint8_t lcd_enabled;
  uint8_t lines_being_updated;
} lcd_lines_t;
lcd_lines_t lcd_lines;

static inline void set_bytes_arr(uint8_t* arr, const lcd_rs_type_t rs, const uint8_t word) {
  uint8_t rs_mask = (rs == LCD_RS_DDR_WR) ? LCD_RS_ON_MASK : LCD_RS_OFF_MASK;
  uint8_t upp = (0xF0 & word);
  uint8_t low = (0xF0 & (word << 4));

  uint8_t clk_hi = LCD_CLOCK_HIGH_MASK | LCD_BACKLIGHT_ON_MASK;
  uint8_t clk_lo = LCD_CLOCK_LOW_MASK | LCD_BACKLIGHT_ON_MASK;

  arr[0] = upp | rs_mask | clk_hi;
  arr[1] = upp | rs_mask | clk_lo;
  arr[2] = low | rs_mask | clk_hi;
  arr[3] = low | rs_mask | clk_lo;
}

void convert_uint32_to_str(void* arr, int len, uint32_t num) {
  if (len > 16) len = 16;
  for (int i = 0; i < len; i++) ((uint8_t*)arr)[i] = ' ';

  if (num == 0) {
    ((uint8_t*)arr)[len - 1] = '0';
    return;
  }

  while (num > 0 && len > 0) {
    const int i = len - 1;

    if (num > 0 && len > 0) {
      ((uint8_t*)arr)[i] = '0' + (num % 10);
      num = num / 10;
    } else {
      ((uint8_t*)arr)[i] = ' ';
    }

    len--;
  }
}

I2C1602StatusCode_t setup_1602_lcd_peripherals(uint32_t mcu_freq_hz, uint8_t lcd_freq_hz) {
  int ticks_hz = mcu_freq_hz / lcd_freq_hz;

  int prescaler = 1;
  while ((ticks_hz / (prescaler + 1)) > 65535) {
    prescaler *= 2;
  }
  int ticks_hz_modified = ticks_hz / (prescaler + 1);

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
                                          .arr = ticks_hz_modified,
                                          .prescaler = prescaler},
                                  .p_base_addr = I2C_1602_TIM_PORT};
  if (timer_init(&i2c_tim_handle) < 0) return I2C_1602_BAD_PERIPHERAL_SETUP;
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
  if (dma_stream_init(&dma_tx_handle) < 0) return I2C_1602_BAD_PERIPHERAL_SETUP;

  // Setup I2C module lastly
  I2CHandle_t i2c_handle = {.addr = I2C_1602_PORT,
                            .cfg = {.peri_clock_freq_hz = (uint32_t)mcu_freq_hz,
                                    .device_mode = I2C_DEVICE_MODE_MASTER,
                                    .scl_mode = I2C_SCL_MODE_SPEED_SM,
                                    .interrupt_enable = I2C_ENABLE,
                                    .dma_enable = I2C_DISABLE,
                                    .enable_on_init = I2C_ENABLE}};
  i2c_peri_clock_control(I2C_1602_PORT, I2C_ENABLE);
  if (i2c_init(&i2c_handle) != I2C_STATUS_OK) return I2C_1602_BAD_PERIPHERAL_SETUP;

  WAIT(MEDIUM);  // Give enough time for i2c module to initialize

  return I2C_1602_STATUS_OK;
}

// TODO: Implement - this takes care of actually setting up the bytes and what not to clear screen and get it ready for
// messages
I2C1602StatusCode_t setup_1602_lcd_screen(void) {
  uint8_t bytes[4];
  // Refer to page 39 of manual
  // Function set 4 bit (DB5 = 1, DB4=1, RS=0, RW=0)
  uint8_t fn_set_4b = LCD_FUNCTION_MASK | LCD_DISPLAY_TWO_LINES;
  set_bytes_arr(bytes, LCD_RS_INST_WR, fn_set_4b);
  if (i2c_master_send(I2C_1602_PORT, bytes, 2, LCD_I2C_ADDR_VDD, I2C_STOP) != I2C_STATUS_OK) return I2C_1602_STATUS_OK;
  WAIT(FAST);
  if (i2c_master_send(I2C_1602_PORT, bytes, SIZEOF(bytes), LCD_I2C_ADDR_VDD, I2C_STOP) != I2C_STATUS_OK)
    return I2C_1602_STATUS_OK;
  WAIT(FAST);

  // Display on (DB5-7 = 0, then DB5-7 = 1, RS/RW = 0)
  uint8_t disp_on = LCD_DISPLAY_CFG_MASK | LCD_DISPLAY_ON_MASK | LCD_CURSOR_ON_MASK;
  set_bytes_arr(bytes, LCD_RS_INST_WR, disp_on);
  if (i2c_master_send(I2C_1602_PORT, bytes, SIZEOF(bytes), LCD_I2C_ADDR_VDD, I2C_STOP) != I2C_STATUS_OK)
    return I2C_1602_ERROR_IN_SETUP;
  WAIT(FAST);

  // Entry mode set (DB5-7 = 0,  then DB5-6 = 1, RS/RW=0)
  uint8_t entry_mode_set = LCD_ENTRY_MODE_MASK | LCD_ENTRY_MODE_INC;
  set_bytes_arr(bytes, LCD_RS_INST_WR, entry_mode_set);
  if (i2c_master_send(I2C_1602_PORT, bytes, SIZEOF(bytes), LCD_I2C_ADDR_VDD, I2C_STOP) != I2C_STATUS_OK)
    return I2C_1602_ERROR_IN_SETUP;
  WAIT(FAST);

  // Clear screen
  uint8_t clr_disp = LCD_CLEAR_DISPLAY;
  set_bytes_arr(bytes, LCD_RS_INST_WR, clr_disp);
  if (i2c_master_send(I2C_1602_PORT, bytes, SIZEOF(bytes), LCD_I2C_ADDR_VDD, I2C_STOP) != I2C_STATUS_OK)
    return I2C_1602_ERROR_IN_SETUP;
  WAIT(MEDIUM);

  // Disable lcd_lines for now
  memset(&lcd_lines.buff, 0, SIZEOF(lcd_lines.buff));
  lcd_lines.len = 0;
  lcd_lines.lcd_enabled = 0;
  lcd_lines.lines_being_updated = 0;

  // Turn on NVICs now
  NVIC_EnableIRQ(I2C_1602_DMA_STREAM_IRQN);
  NVIC_EnableIRQ(I2C_1602_PORT_EV_IRQN);
  NVIC_EnableIRQ(I2C_1602_PORT_ERR_IRQN);

  return I2C_1602_STATUS_OK;
}

I2C1602StatusCode_t set_1602_lcd_str(const void* buff1, uint8_t len1, const void* buff2, uint8_t len2) {
  int buff_cnt = 0;
  lcd_lines.lines_being_updated = 1;

  len1 = (len1 > 16) ? 16 : len1;
  len2 = (len2 > 16) ? 16 : len2;

  set_bytes_arr(lcd_lines.buff + buff_cnt, LCD_RS_INST_WR, LCD_JUMP_FIRST_LINE);
  buff_cnt += 4;

  for (int i = 0; i < len1; i++) {
    set_bytes_arr(lcd_lines.buff + buff_cnt, LCD_RS_DDR_WR, ((uint8_t*)buff1)[i]);
    buff_cnt += 4;
  }

  for (int i = len1; i < 16; i++) {
    set_bytes_arr(lcd_lines.buff + buff_cnt, LCD_RS_DDR_WR, ' ');
    buff_cnt += 4;
  }

  set_bytes_arr(lcd_lines.buff + buff_cnt, LCD_RS_INST_WR, LCD_JUMP_SECOND_LINE);
  buff_cnt += 4;

  for (int i = 0; i < len2; i++) {
    set_bytes_arr(lcd_lines.buff + buff_cnt, LCD_RS_DDR_WR, ((uint8_t*)buff2)[i]);
    buff_cnt += 4;
  }

  for (int i = len2; i < 16; i++) {
    set_bytes_arr(lcd_lines.buff + buff_cnt, LCD_RS_DDR_WR, ' ');
    buff_cnt += 4;
  }

  lcd_lines.len = buff_cnt;

  I2CDMAConfig_t dma_config = {.address = LCD_I2C_ADDR_VDD,
                               .tx = {.buff = lcd_lines.buff, .len = lcd_lines.len},
                               .rx = {.buff = NULL, .len = 0},
                               .tx_stream = I2C_1602_DMA_STREAM,
                               .dma_set_buffer_cb = dma_set_buffer,
                               .dma_start_transfer_cb = dma_start_transfer,
                               .circular = I2C_INTERRUPT_NON_CIRCULAR,
                               .callback = NULL};
  if (i2c_setup_interrupt_dma(I2C_1602_PORT, &dma_config) != I2C_STATUS_OK) {
    lcd_lines.lines_being_updated = 0;
    return I2C_1602_ERROR_IN_SETUP;
  }

  lcd_lines.lines_being_updated = 0;
  return I2C_1602_STATUS_OK;
}

// Control funcs
void enable_1602_lcd_updating(void) { lcd_lines.lcd_enabled = 1; }
void disable_1602_lcd_updating(void) { lcd_lines.lcd_enabled = 0; }

// NVIC Definitions here:
void I2C_PORT_EV_IRQ_HANDLER(void) { i2c_dma_irq_handling_start(I2C_1602_PORT); }

void I2C_PORT_ERR_IRQ_HANDLER(void) {
  I2CIRQType_t irq_error = i2c_irq_error_handling(I2C_1602_PORT);
  if (irq_error == I2C_IRQ_TYPE_ERROR_ACKFAIL) {
    printf("Error...\n");
    i2c_start_interrupt_dma(I2C_1602_PORT);
  }
}

void I2C_DMA_TX_STREAM_IRQ_HANDLER(void) {
  if (dma_irq_handling(I2C_1602_DMA_STREAM, DMA_INTERRUPT_TYPE_FULL_TRANSFER_COMPLETE))
    i2c_dma_irq_handling_end(I2C_1602_PORT, I2C_TXRX_DIR_SEND);
}

// TImer logic for sending the word itself
void I2C_TIM_IRQ_HANDLER(void) {
  if (timer_irq_handling(I2C_1602_TIM_PORT, 1)) {
    if (lcd_lines.lines_being_updated || lcd_lines.lcd_enabled) return;
    i2c_start_interrupt_dma(I2C_1602_PORT);
  }
}
