#include "include/rchal.h"
#include "include/platform.h"
#include "stm32f1xx_hal.h"
#include "include/cmath.h"
#include "microtime.h"
#include <fastmath.h>

#define TM1637_GPIO_PORT    GPIOB
#define TM1637_CLK_PIN      GPIO_PIN_10
#define TM1637_DIO_PIN      GPIO_PIN_11
// the sweatshop children shall not eat, esp one in QA, violating
// official tm1637 timings (if broke set to 10u)
#define TM1637_DELAY_US     (7u)

#define BTNS_GPIO_PORT      GPIOA
#define BTN_SELECT_PIN      GPIO_PIN_3
#define BTN_CANCEL_PIN      GPIO_PIN_4
#define BTN_OPTIONS_PIN     GPIO_PIN_5
#define BTN_INCR_PIN        GPIO_PIN_6
#define BTN_DECR_PIN        GPIO_PIN_7

#define HEATER_GPIO_PORT    GPIOA
#define HEATER_PIN          GPIO_PIN_10

#define BUZZER_GPIO_PORT    GPIOB
#define BUZZER_PIN          GPIO_PIN_9

#define DBG_LED_GPIO_PORT   GPIOC
#define DBG_LED_PIN         GPIO_PIN_13

#define NTC_R_REF           (12000.0f)
#define NTC_R0              (100000.0f)
#define NTC_TO_KELVIN       (298.15f)
#define NTC_BETA            (3950.0f)
#define ADC_MAX_VAL         (4095.0f)

static ADC_HandleTypeDef adc1_handle;
static bool heater_switched_on = false;

static TIM_HandleTypeDef tim4_pwm;

void rc_init_rchal(void) {
    // enable ports and ADCs
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_ADC1_CLK_ENABLE(); // ADC pins

    __HAL_RCC_TIM4_CLK_ENABLE();

    GPIO_InitTypeDef buzzer_conf = {0};
    buzzer_conf.Pin = BUZZER_PIN;
    buzzer_conf.Mode = GPIO_MODE_AF_PP; // timer drives the pin now
    buzzer_conf.Pull = GPIO_NOPULL;
    buzzer_conf.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BUZZER_GPIO_PORT, &buzzer_conf);

    RCC_ClkInitTypeDef clocks;
    uint32_t ignored; HAL_RCC_GetClockConfig(&clocks, &ignored);
    uint32_t tim4_clock = HAL_RCC_GetPCLK1Freq();
    if (clocks.APB1CLKDivider != RCC_HCLK_DIV1) {
        tim4_clock *= 2U;
    }

    tim4_pwm.Instance = TIM4;
    tim4_pwm.Init.Prescaler = (tim4_clock / 1000000U) - 1U; // 1 MHz tick
    tim4_pwm.Init.CounterMode = TIM_COUNTERMODE_UP;
    tim4_pwm.Init.Period = 1000U - 1U; // placeholder, overwritten per note
    tim4_pwm.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    tim4_pwm.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE; // glitch-free note changes
    if (HAL_TIM_PWM_Init(&tim4_pwm) != HAL_OK) {
        platform_on_error(HAL_INIT_GPIO_ANALOG); // or a buzzer-specific error code
    }

    TIM_OC_InitTypeDef oc = {0};
    oc.OCMode = TIM_OCMODE_PWM1;
    oc.Pulse = 0; // CCR = 0 means silent
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;
    oc.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&tim4_pwm, &oc, TIM_CHANNEL_4) != HAL_OK) {
        platform_on_error(HAL_INIT_GPIO_ANALOG);
    }
    HAL_TIM_PWM_Start(&tim4_pwm, TIM_CHANNEL_4);

    // buttons config (pulldown)
    GPIO_InitTypeDef btns_conf = {0};
    btns_conf.Pin = BTN_SELECT_PIN | BTN_CANCEL_PIN | BTN_OPTIONS_PIN | BTN_INCR_PIN | BTN_DECR_PIN;
    btns_conf.Mode = GPIO_MODE_INPUT;
    btns_conf.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(BTNS_GPIO_PORT, &btns_conf);

    // TM1637 DIO/CLK pins config
    HAL_GPIO_WritePin(TM1637_GPIO_PORT, TM1637_CLK_PIN | TM1637_DIO_PIN, GPIO_PIN_SET); // normalize first
    GPIO_InitTypeDef tm1637_pins_conf = {0}; // now can config
    tm1637_pins_conf.Mode = GPIO_MODE_OUTPUT_OD; // open drain
    tm1637_pins_conf.Pull = GPIO_NOPULL; // the TM1637 module has a pull up resistor already
    tm1637_pins_conf.Speed = GPIO_SPEED_FREQ_HIGH;
    tm1637_pins_conf.Pin = TM1637_CLK_PIN | TM1637_DIO_PIN;
    HAL_GPIO_Init(TM1637_GPIO_PORT, &tm1637_pins_conf);

    // LED pin config (this is low-active, so SET == off)
    HAL_GPIO_WritePin(DBG_LED_GPIO_PORT, DBG_LED_PIN, GPIO_PIN_SET); // off by default
    GPIO_InitTypeDef dbgled_conf = {0}; // now can config
    dbgled_conf.Pin = DBG_LED_PIN;
    dbgled_conf.Mode = GPIO_MODE_OUTPUT_PP;
    dbgled_conf.Pull = GPIO_NOPULL;
    dbgled_conf.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DBG_LED_GPIO_PORT, &dbgled_conf);

    // heating element pins config
    HAL_GPIO_WritePin(HEATER_GPIO_PORT, HEATER_PIN, GPIO_PIN_RESET); // OFF by default
    GPIO_InitTypeDef heater_cfg = {0};
    heater_cfg.Pin = HEATER_PIN;
    heater_cfg.Mode = GPIO_MODE_OUTPUT_PP;
    heater_cfg.Pull = GPIO_NOPULL;
    heater_cfg.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(HEATER_GPIO_PORT, &heater_cfg);

    // ADC1 initialization
    // configure PA1 and PA2 as analog pins
    GPIO_InitTypeDef gpio_adc = {0};
    gpio_adc.Pin = GPIO_PIN_1 | GPIO_PIN_2;
    gpio_adc.Mode = GPIO_MODE_ANALOG;
    gpio_adc.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio_adc);

    // ADC1 configuration
    adc1_handle.Instance = ADC1;
    adc1_handle.Init.ScanConvMode = ADC_SCAN_DISABLE;
    adc1_handle.Init.ContinuousConvMode = DISABLE;
    adc1_handle.Init.DiscontinuousConvMode = DISABLE;
    adc1_handle.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    adc1_handle.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    adc1_handle.Init.NbrOfConversion = 1;
    if (HAL_ADC_Init(&adc1_handle) != HAL_OK) {
        platform_on_error(HAL_INIT_GPIO_ANALOG);
    }

    // ADC self-calibration
    if (HAL_ADCEx_Calibration_Start(&adc1_handle) != HAL_OK) {
        platform_on_error(HAL_INIT_GPIO_ANALOG);
    }
}

void rc_tm1637dp_start(void) {
    HAL_GPIO_WritePin(TM1637_GPIO_PORT, TM1637_CLK_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(TM1637_GPIO_PORT, TM1637_DIO_PIN, GPIO_PIN_SET);
    mt_delay_blocking_us(TM1637_DELAY_US);
    HAL_GPIO_WritePin(TM1637_GPIO_PORT, TM1637_DIO_PIN, GPIO_PIN_RESET);
}

void rc_tm1637dp_write_byte(uint8_t byte) {
    // FOR OPEN DRAIN BUSES, SET is not pulling HIGH but leave it floating (release)
    for (uint8_t i = 0; i < 8; i++) {
        HAL_GPIO_WritePin(TM1637_GPIO_PORT, TM1637_CLK_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(TM1637_GPIO_PORT, TM1637_DIO_PIN,
            byte & 0b1 ? GPIO_PIN_SET : GPIO_PIN_RESET // lsb
        );
        mt_delay_blocking_us(TM1637_DELAY_US);
        byte >>= 1;
        HAL_GPIO_WritePin(TM1637_GPIO_PORT, TM1637_CLK_PIN, GPIO_PIN_SET);
        mt_delay_blocking_us(TM1637_DELAY_US);
    }
    // ignore the ACK but gives the 9th clock cycle anyway
    HAL_GPIO_WritePin(TM1637_GPIO_PORT, TM1637_CLK_PIN, GPIO_PIN_RESET);
    mt_delay_blocking_us(TM1637_DELAY_US);
    HAL_GPIO_WritePin(TM1637_GPIO_PORT, TM1637_DIO_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(TM1637_GPIO_PORT, TM1637_CLK_PIN, GPIO_PIN_SET);
    mt_delay_blocking_us(TM1637_DELAY_US);
    HAL_GPIO_WritePin(TM1637_GPIO_PORT, TM1637_CLK_PIN, GPIO_PIN_RESET);
}

void rc_tm1637dp_stop(void) {
    HAL_GPIO_WritePin(TM1637_GPIO_PORT, TM1637_CLK_PIN, GPIO_PIN_RESET);
    mt_delay_blocking_us(TM1637_DELAY_US);
    HAL_GPIO_WritePin(TM1637_GPIO_PORT, TM1637_DIO_PIN, GPIO_PIN_RESET);
    mt_delay_blocking_us(TM1637_DELAY_US);
    HAL_GPIO_WritePin(TM1637_GPIO_PORT, TM1637_CLK_PIN, GPIO_PIN_SET);
    mt_delay_blocking_us(TM1637_DELAY_US);
    HAL_GPIO_WritePin(TM1637_GPIO_PORT, TM1637_DIO_PIN, GPIO_PIN_SET);
}

bitmask8_t rc_buttons_get_state(void) {
    bitmask8_t buttons = 0u; // read button mask from the hardware btns
    if (HAL_GPIO_ReadPin(BTNS_GPIO_PORT, BTN_SELECT_PIN) == GPIO_PIN_SET)  buttons |= RC_BTN_SELECT;
    if (HAL_GPIO_ReadPin(BTNS_GPIO_PORT, BTN_CANCEL_PIN) == GPIO_PIN_SET)  buttons |= RC_BTN_CANCEL;
    if (HAL_GPIO_ReadPin(BTNS_GPIO_PORT, BTN_OPTIONS_PIN) == GPIO_PIN_SET) buttons |= RC_BTN_OPTIONS;
    if (HAL_GPIO_ReadPin(BTNS_GPIO_PORT, BTN_INCR_PIN) == GPIO_PIN_SET)    buttons |= RC_BTN_INCR;
    if (HAL_GPIO_ReadPin(BTNS_GPIO_PORT, BTN_DECR_PIN) == GPIO_PIN_SET)    buttons |= RC_BTN_DECR;
    return buttons;
}

void rc_buzzer_tone(const uint32_t freq_hz) {
    if (freq_hz == 0) {
        __HAL_TIM_SET_COMPARE(&tim4_pwm, TIM_CHANNEL_4, 0); // silent, held low
        return;
    }
    uint32_t arr = 1000000U / freq_hz - 1U;
    if (arr > 0xFFFFU) arr = 0xFFFFU;
    __HAL_TIM_SET_AUTORELOAD(&tim4_pwm, arr);
    __HAL_TIM_SET_COMPARE(&tim4_pwm, TIM_CHANNEL_4, (arr + 1U) / 2U);
}

void rc_dbg_led_set(const bool on) {
    HAL_GPIO_WritePin(DBG_LED_GPIO_PORT, DBG_LED_PIN, on ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

static uint16_t adc_read_channel(const uint32_t channel) {
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = channel;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_71CYCLES_5;
    HAL_ADC_ConfigChannel(&adc1_handle, &sConfig);

    HAL_ADC_Start(&adc1_handle);
    HAL_ADC_PollForConversion(&adc1_handle, 10);
    const uint16_t val = (uint16_t)HAL_ADC_GetValue(&adc1_handle);
    HAL_ADC_Stop(&adc1_handle);

    return val;
}

float rc_sensor_get_temp(const rc_temp_sensor_t sensor) {
    const uint32_t channel = (sensor == RC_SENSOR_POT) ? ADC_CHANNEL_1 : ADC_CHANNEL_2;

    // take the average of 8 samples
    uint32_t adc_sum = 0;
    for (int i = 0; i < 8; i++) {
        adc_sum += adc_read_channel(channel);
    }
    float raw_adc = (float)adc_sum / 8.0f;
    raw_adc = fclampf(raw_adc, 1.0f, 4094.0f);

    // R_ntc = R_ref / ((ADC_MAX / raw_adc) - 1)
    const float r_ntc = NTC_R_REF / ((ADC_MAX_VAL / raw_adc) - 1.0f);

    // beta function: 1/T = 1/T0 + (1/B) * ln(R / R0)
    const float steinhart = (1.0f / NTC_TO_KELVIN) + (1.0f / NTC_BETA) * logf(r_ntc / NTC_R0);
    const float temp_celsius = (1.0f / steinhart) - 273.15f;

    return temp_celsius;
}

void rc_heater_set(const bool on) {
    HAL_GPIO_WritePin(HEATER_GPIO_PORT, HEATER_PIN,
        ((heater_switched_on = on)) ? GPIO_PIN_SET : GPIO_PIN_RESET
    );
}

bool rc_heater_is_on(void) {
    return heater_switched_on;
}

uint32_t rc_time_current_millis(void) {
    return HAL_GetTick();
}
