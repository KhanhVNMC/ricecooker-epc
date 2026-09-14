#include "include/platform.h"
#include "stm32f1xx_hal.h"

// this setups necessary oscillator and sysclock inside the mcu
void stm32_clock_init(void) {
    // this configs the clock generating hardware (oscillator)
    RCC_OscInitTypeDef oscillator = {0};
    oscillator.OscillatorType = RCC_OSCILLATORTYPE_HSI; // HIGH SPEED INTERNAL oscil
    oscillator.HSIState = RCC_HSI_ON;
    oscillator.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    oscillator.PLL.PLLState = RCC_PLL_NONE;

    if (HAL_RCC_OscConfig(&oscillator) != HAL_OK) {
        platform_on_error(HAL_INIT_OSCILLATOR);
    }

    // this configs the receiving end of the oscillator (where it goes)
    RCC_ClkInitTypeDef clocks = {0};
    clocks.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clocks.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    clocks.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clocks.APB1CLKDivider = RCC_HCLK_DIV1;
    clocks.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&clocks, FLASH_LATENCY_0) != HAL_OK) {
        platform_on_error(HAL_INIT_SYSCLK);
    }
}

// called to setup necessary platform bullshit
void platform_init_primitives(void) {
    stm32_clock_init();
}

// called when there's an unrecoverable error
void platform_on_error(const platform_error_t) {
    __disable_irq();
    for (;;) { }
}

// turn on clocks to AFIO and PWR (PMU-ish)
void HAL_MspInit(void) {
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();
}

// global entrypoint
int main(void) {
    HAL_Init();
    platform_main(); // this should NEVER return
    platform_on_error(APPLICATION_INIT);
}
