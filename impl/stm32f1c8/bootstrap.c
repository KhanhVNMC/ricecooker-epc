#include "include/platform.h"
#include "include/rchal.h"
#include "include/ecclayer.h"
#include "stm32f1xx_hal.h"
#include "microtime.h"

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

// this setups necessary oscillator and sysclock inside the mcu
static void stm32_clock_init(void) {
    // this configs the clock generating hardware (oscillator)
    RCC_OscInitTypeDef oscillator = {0};
    oscillator.OscillatorType = RCC_OSCILLATORTYPE_HSE; // HIGH SPEED EXTERNAL oscil
    oscillator.HSEState = RCC_HSE_ON;
    oscillator.HSEPredivValue = RCC_HSE_PREDIV_DIV1;

    oscillator.PLL.PLLState = RCC_PLL_ON;
    oscillator.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    oscillator.PLL.PLLMUL = RCC_PLL_MUL9; // 8 * 9 = 72 mhz

    if (HAL_RCC_OscConfig(&oscillator) != HAL_OK) {
        platform_on_error(HAL_INIT_OSCILLATOR);
    }

    // this configs the receiving end of the oscillator (where it goes)
    RCC_ClkInitTypeDef clocks = {0};
    clocks.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clocks.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK; // 72mhz

    clocks.AHBCLKDivider = RCC_SYSCLK_DIV1; // HCLCK | cpu, ram, etc: 72/1 = 72mhz
    clocks.APB1CLKDivider = RCC_HCLK_DIV2; // PCLK1 | TIM2, tim4 etc: 72/2 = 36mhz
    clocks.APB2CLKDivider = RCC_HCLK_DIV1; // PCLK2 | ADC1, USB etc: 72mhz

    if (HAL_RCC_ClockConfig(&clocks, FLASH_LATENCY_2) != HAL_OK) {
        platform_on_error(HAL_INIT_SYSCLK);
    }

    // this configure the downstream consumer of APB2
    RCC_PeriphCLKInitTypeDef periph = {0};
    periph.PeriphClockSelection = RCC_PERIPHCLK_ADC | RCC_PERIPHCLK_USB;
    periph.AdcClockSelection = RCC_ADCPCLK2_DIV6; // ADC1 can only run at 12mhz
    periph.UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5; // USB runs at 48mhz

    if (HAL_RCCEx_PeriphCLKConfig(&periph) != HAL_OK) {
        platform_on_error(HAL_INIT_SYSCLK);
    }
}

// called to setup necessary platform bullshit
static void platform_init_primitives(void) {
    HAL_Init(); // HAL
    stm32_clock_init(); // osil and PLL
    mt_us_timer_init(); // us timer
    rc_init_rchal(); // rice cooker hal
    ecc_init_stack();
}

// global entrypoint
int main(void) {
    platform_init_primitives();
    platform_main(); // this should NEVER return
    platform_on_error(APPLICATION_INIT);
}
