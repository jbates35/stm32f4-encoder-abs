
#ifndef INC_STM32F446XX_ENC_H_
#define INC_STM32F446XX_ENC_H_

#include <stdint.h>

#include "stm32f446xx.h"

#define SIZEOF(arr) ((unsigned int)sizeof(arr) / sizeof(arr[0]))

#define VERY_FAST 16
#define FAST 10000
#define MEDIUM 300000
#define SLOW 1000000
#define WAIT(CNT)                                          \
  do {                                                     \
    for (int sleep_cnt = 0; sleep_cnt < CNT; sleep_cnt++); \
  } while (0)

typedef enum {
	ENCODER_STATUS_OK = 0
} EncoderStatus_t;

EncoderStatus_t init_encoder(TIM_TypeDef *tim_port, int* starting_tick);

#endif
