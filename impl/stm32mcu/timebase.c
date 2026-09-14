#include "stm32f1xx_hal.h"

// I KNOW THIS COULD BE DONE EASILY WITH SYSTICK BUT FREERTOS DEMAND IT SO
// WE HAVE TO PROGRAM THE TIM4 TO GET HAL_GetTick

static TIM_HandleTypeDef tim4_tick_timer; // handle for the TIM4 device

HAL_StatusTypeDef HAL_InitTick(const uint32_t TickPriority) {
    __HAL_RCC_TIM4_CLK_ENABLE(); // enable the TIM4 clock so it clocks (wtf am i saying)

    // this finds the input clock of TIM4 so we can scale it down correctly
    // i should've hardcoded it but well we could swap hardware at last minute so
    RCC_ClkInitTypeDef clocks;
    uint32_t ignored; HAL_RCC_GetClockConfig(&clocks, &ignored);
    uint32_t timer_clock = HAL_RCC_GetPCLK1Freq();
    if (clocks.APB1CLKDivider != RCC_HCLK_DIV1) {
        timer_clock *= 2U; // no idea why
    }

    tim4_tick_timer.Instance = TIM4;
    // the prescaler + 1 before dividing, somehow, so we -1 to make it divide correctly
    tim4_tick_timer.Init.Prescaler = (timer_clock / 1000000U) - 1U;
    tim4_tick_timer.Init.CounterMode = TIM_COUNTERMODE_UP; // count up
    tim4_tick_timer.Init.Period = 1000U - 1U; // every 1000 microsec (1ms)

    // default values
    tim4_tick_timer.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    tim4_tick_timer.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_Base_Init(&tim4_tick_timer) != HAL_OK) {
        return HAL_ERROR;
    }

    // register TIM4 to the irq controller, and enable irq
    HAL_NVIC_SetPriority(TIM4_IRQn, TickPriority, 0U);
    HAL_NVIC_EnableIRQ(TIM4_IRQn);
    return HAL_TIM_Base_Start_IT(&tim4_tick_timer);
}

// the CPU IRQ handler will jump into this function when TIM4 raise IRQ
// we only interested in the periodic irq (which will call HAL_TIM_PeriodElapsedCallback)
void TIM4_IRQHandler(void) {
    HAL_TIM_IRQHandler(&tim4_tick_timer); // generic IRQ handler
}

// calls periodically by TIMs when they do periodic shit
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM4) {
        // inc HAL tick if the TIM that rasied is indeed 4
        HAL_IncTick();
    }
}
