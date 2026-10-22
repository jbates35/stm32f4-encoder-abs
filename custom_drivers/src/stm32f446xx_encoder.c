#include "stm32f446xx_encoder.h"
#include "stm32f446xx_tim.h"

#include <stdio.h>
#include <string.h>

int prev_count;
long current_ticks;

EncoderStatus_t init_encoder(TIM_TypeDef *tim_port) {
  int smcr_word = 0;
  int ccer_word = 0;
  int cr1_word = 0;
  int ccmr1_word = 0;
  int arr_word = 0;

  // Steps for turning on encoder mode (page 484 in the ref manual)
  // 1. Write SMS='011' in TIMx_SMCR register (this is for quad enc)
  smcr_word |= (0x3 << TIM_SMCR_SMS_Pos);

  // 2. SElect the TI1 and TI@ polarity:
  // PRogram the CC1P and CC2P bits in TIMx_CCER.
  // Input filter can be programmed as well (but this
  // has a driver on it). CC1NP and CC2NP must be kept low.
  // As a note, refer to Table 107 for counting direction vs encoder signals

  // 3. Configure DIR (for direction) and TIMx_ARR register (for auto reload value it counts up to or down from)
  arr_word = 0xFFFF;

  // 4. Map inputs: CC1S='01' in TIMx_CCMR1 register, CC2S='01' on TIMx_CCMR1 register
  ccmr1_word |= (0x1 << TIM_CCMR1_CC2S_Pos) | (0x1 << TIM_CCMR1_CC1S_Pos);

  // 5a. CC1P='0', CC1NP='0', IC1F='0000 (TIMx_CCER register)'
  // 5b. CC2P='0', CC2NP='0', IC2F='0000 (TIMx_CCER register)'
  ccer_word |= TIM_CCER_CC1E | TIM_CCER_CC2E;

  // 6. CEN = 1 ( and dir set to rising)
  cr1_word |= TIM_CR1_CEN;

  timer_peri_clock_control(tim_port, 1);

  current_ticks = 0;
  prev_count = tim_port->CNT;
	
  tim_port->ARR = arr_word;
  tim_port->CCMR1 = ccmr1_word;
  tim_port->SMCR = smcr_word;
  tim_port->CCER = ccer_word;
  tim_port->CR1 = cr1_word;

  return ENCODER_STATUS_OK;
}

EncoderStatus_t update_tick_count(TIM_TypeDef *tim_port) {
    if(tim_port==NULL) {
      return ENCODER_STATUS_NULL_PTR;
    }
    int count = tim_port->CNT;
    int delta = count - prev_count;
    prev_count = count;
  
    // Handle rollovers
    if (delta < -32768) delta += 65536;
    else if (delta > 32768) delta -= 65536;

    current_ticks = current_ticks + delta;

    return ENCODER_STATUS_OK;
}
