#include "stm32f1xx_hal.h"
#include "include/platform.h"
#include "microtime.h"

// I KNOW THIS COULD BE DONE EASILY WITH SYSTICK BUT FREERTOS DEMAND IT SO
// WE HAVE TO PROGRAM THE TIM3 TO GET HAL_GetTick

static TIM_HandleTypeDef tim3_tick_timer; // handle for the TIM3 device

HAL_StatusTypeDef HAL_InitTick(const uint32_t) {
    __HAL_RCC_TIM3_CLK_ENABLE(); // enable the TIM3 clock so it clocks (wtf am i saying)

    // this finds the input clock of TIM3 so we can scale it down correctly
    // i should've hardcoded it but well
    RCC_ClkInitTypeDef clocks;
    uint32_t ignored; HAL_RCC_GetClockConfig(&clocks, &ignored);
    uint32_t tim3_clock = HAL_RCC_GetPCLK1Freq();
    if (clocks.APB1CLKDivider != RCC_HCLK_DIV1) {
        tim3_clock *= 2U; // there's a clock multplier in between PCLK and TIM3
    }

    tim3_tick_timer.Instance = TIM3;
    // suppose the device runs at 72mhz, the prescaler basically pulls this down to 72mhz/72 = 1mhz
    // increments up every microsec
    tim3_tick_timer.Init.Prescaler = (tim3_clock / 1000000U) - 1U; // the prescaler + 1 before dividing, somehow, so -1 to compensate
    tim3_tick_timer.Init.CounterMode = TIM_COUNTERMODE_UP; // count up
    tim3_tick_timer.Init.Period = 1000U - 1U; // wraps every 1ms (this pokes the cpu since _IT)

    // default values
    tim3_tick_timer.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    tim3_tick_timer.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_Base_Init(&tim3_tick_timer) != HAL_OK) {
        return HAL_ERROR;
    }

    // register TIM3 to the irq controller, and enable irq, set prio to 0 so kernel wont mask it
    HAL_NVIC_SetPriority(TIM3_IRQn, 0U, 0U);
    HAL_NVIC_EnableIRQ(TIM3_IRQn);
    return HAL_TIM_Base_Start_IT(&tim3_tick_timer);
}

// the CPU IRQ handler will jump into this function when TIM3 raise IRQ
// we only interested in the periodic irq (which will call HAL_TIM_PeriodElapsedCallback)
void TIM3_IRQHandler(void) {
    HAL_TIM_IRQHandler(&tim3_tick_timer); // generic IRQ handler
}

// calls periodically by TIMs when they do periodic shit
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM3) {
        // inc HAL tick if the TIM that raised is indeed 4
        HAL_IncTick();
    }
}

// FOR THE TIM2 MICROSECOND CLOCK (NO IRQ)

static TIM_HandleTypeDef tim2_us_timer;
// this thing makes blockin microsecond sleep go vroom vroom
void mt_us_timer_init(void) {
    __HAL_RCC_TIM2_CLK_ENABLE(); // enable the TIM2 clock so it clocks

    RCC_ClkInitTypeDef clocks;
    uint32_t ignored; HAL_RCC_GetClockConfig(&clocks, &ignored);
    uint32_t tim2_clock = HAL_RCC_GetPCLK1Freq();
    if (clocks.APB1CLKDivider != RCC_HCLK_DIV1) {
        tim2_clock *= 2U; // there's a clock multplier in between PCLK and TIM2
    }

    tim2_us_timer.Instance = TIM2;
    // suppose the TIM2 runs at 72mhz, the prescaler basically pulls this down to 72mhz/72 = 1mhz
    // increments up every microsec
    tim2_us_timer.Init.Prescaler = (tim2_clock / 1000000U) - 1U; // the prescaler + 1 before dividing, somehow, so -1 to compensate
    tim2_us_timer.Init.CounterMode = TIM_COUNTERMODE_UP; // count up
    tim2_us_timer.Init.Period = 0xFFFFU;

    // default values
    tim2_us_timer.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    tim2_us_timer.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_Base_Init(&tim2_us_timer) != HAL_OK) {
        platform_on_error(HAL_INIT_US_SLEEP);
    }

    if (HAL_TIM_Base_Start(&tim2_us_timer) != HAL_OK) {
        platform_on_error(HAL_INIT_US_SLEEP);
    }
}

// this is NOT SUITABLE for timestamp because it wraps like crazy
uint16_t mt_time_cnt_micros(void) {
    return __HAL_TIM_GET_COUNTER(&tim2_us_timer);
}

void mt_delay_blocking_us(const uint16_t delay) {
    const uint16_t start = mt_time_cnt_micros();
    // block until current >= start + delay
    // cast it to an uint so it can wrap properly
    while ((uint16_t)(mt_time_cnt_micros() - start) < delay) {
        /*busy wait*/
    }
}