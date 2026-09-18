#include "include/rchal.h"
#include "stm32f1xx_hal_adc.h"
#include "microtime.h"
#include <fastmath.h>

#define TM1637_GPIO_PORT    GPIOB
#define TM1637_CLK_PIN      GPIO_PIN_10
#define TM1637_DIO_PIN      GPIO_PIN_11

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

#define NTC_R_REF           10000.0f
#define NTC_R0              100000.0f
#define NTC_TO_KELVIN       298.15f
#define NTC_BETA            3950.0f
#define ADC_MAX_VAL         4095.0f

static ADC_HandleTypeDef hadc1;

void init_all_adcs(void) {
    __HAL_RCC_ADC1_CLK_ENABLE();

    // configure PA1 and PA2 as analog pins
    GPIO_InitTypeDef gpio_adc = {0};
    gpio_adc.Pin = GPIO_PIN_1 | GPIO_PIN_2;
    gpio_adc.Mode = GPIO_MODE_ANALOG;
    gpio_adc.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio_adc);

    // ADC1 configuration
    hadc1.Instance = ADC1;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    HAL_ADC_Init(&hadc1);

    // ADC self-calibration
    HAL_ADCEx_Calibration_Start(&hadc1);
}

uint16_t adc_read_channel(uint32_t channel) {
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = channel;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_71CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 10);
    uint16_t val = (uint16_t)HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);

    return val;
}

void rc_init_rchal(void) {
    // enable ports (we use B for now)
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // TM1637 DIO/CLK pins config
    HAL_GPIO_WritePin(TM1637_GPIO_PORT, TM1637_CLK_PIN | TM1637_DIO_PIN, GPIO_PIN_SET); // normalize first
    GPIO_InitTypeDef tm1637_pins_conf = {0}; // now can config
    tm1637_pins_conf.Mode = GPIO_MODE_OUTPUT_OD; // open drain
    tm1637_pins_conf.Pull = GPIO_NOPULL; // the TM1637 module has a pull up resistor already
    tm1637_pins_conf.Speed = GPIO_SPEED_FREQ_HIGH;
    tm1637_pins_conf.Pin = TM1637_CLK_PIN | TM1637_DIO_PIN;
    HAL_GPIO_Init(TM1637_GPIO_PORT, &tm1637_pins_conf);

    // BUZZER pín config
    HAL_GPIO_WritePin(BUZZER_GPIO_PORT, BUZZER_PIN, GPIO_PIN_RESET); // muted by default
    GPIO_InitTypeDef buzzer_conf = {0}; // now can config
    buzzer_conf.Pin = BUZZER_PIN;
    buzzer_conf.Mode = GPIO_MODE_OUTPUT_PP; // push-pull drive for transistor saturation
    buzzer_conf.Pull = GPIO_NOPULL;
    buzzer_conf.Speed = GPIO_SPEED_FREQ_LOW; // 2MHz
    HAL_GPIO_Init(BUZZER_GPIO_PORT, &buzzer_conf);

    // heating element pín config
    HAL_GPIO_WritePin(HEATER_GPIO_PORT, HEATER_PIN, GPIO_PIN_RESET); // OFF by default
    GPIO_InitTypeDef heater_cfg = {0};
    heater_cfg.Pin = HEATER_PIN;
    heater_cfg.Mode = GPIO_MODE_OUTPUT_PP;
    heater_cfg.Pull = GPIO_NOPULL;
    heater_cfg.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(HEATER_GPIO_PORT, &heater_cfg);

    // ADC1 initialization
    init_all_adcs();
}

void rc_tm1637dp_start(void) {
    HAL_GPIO_WritePin(TM1637_GPIO_PORT, TM1637_CLK_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(TM1637_GPIO_PORT, TM1637_DIO_PIN, GPIO_PIN_SET);
    mt_delay_blocking_us(2);
    HAL_GPIO_WritePin(TM1637_GPIO_PORT, TM1637_DIO_PIN, GPIO_PIN_RESET);
}

void rc_tm1637dp_write_byte(uint8_t byte) {
    // FOR OPEN DRAIN BUSES, SET is not pulling HIGH but leave it floating (release)
    for (uint8_t i = 0; i < 8; i++) {
        HAL_GPIO_WritePin(TM1637_GPIO_PORT, TM1637_CLK_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(TM1637_GPIO_PORT, TM1637_DIO_PIN,
            byte & 0b1 ? GPIO_PIN_SET : GPIO_PIN_RESET // lsb
        );
        mt_delay_blocking_us(3);
        byte >>= 1;
        HAL_GPIO_WritePin(TM1637_GPIO_PORT, TM1637_CLK_PIN, GPIO_PIN_SET);
        mt_delay_blocking_us(3);
    }
    // ignore the ACK but gives the 9th clock cycle anyway
    HAL_GPIO_WritePin(TM1637_GPIO_PORT, TM1637_CLK_PIN, GPIO_PIN_RESET);
    mt_delay_blocking_us(5);
    HAL_GPIO_WritePin(TM1637_GPIO_PORT, TM1637_DIO_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(TM1637_GPIO_PORT, TM1637_CLK_PIN, GPIO_PIN_SET);
    mt_delay_blocking_us(2);
    HAL_GPIO_WritePin(TM1637_GPIO_PORT, TM1637_CLK_PIN, GPIO_PIN_RESET);
}

void rc_tm1637dp_stop(void) {
    HAL_GPIO_WritePin(TM1637_GPIO_PORT, TM1637_CLK_PIN, GPIO_PIN_RESET);
    mt_delay_blocking_us(2);
    HAL_GPIO_WritePin(TM1637_GPIO_PORT, TM1637_DIO_PIN, GPIO_PIN_RESET);
    mt_delay_blocking_us(2);
    HAL_GPIO_WritePin(TM1637_GPIO_PORT, TM1637_CLK_PIN, GPIO_PIN_SET);
    mt_delay_blocking_us(2);
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

void rc_buzzer_set(const bool on) {
    HAL_GPIO_WritePin(BUZZER_GPIO_PORT, BUZZER_PIN, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

float rc_sensor_get_temp(const rc_temp_sensor_t sensor) {
    uint32_t channel = (sensor == RC_SENSOR_POT) ? ADC_CHANNEL_1 : ADC_CHANNEL_2;

    // take the average of 8 samples
    uint32_t adc_sum = 0;
    for (int i = 0; i < 8; i++) {
        adc_sum += adc_read_channel(channel);
    }
    float raw_adc = (float)adc_sum / 8.0f;

    if (raw_adc < 1.0f) raw_adc = 1.0f;
    if (raw_adc >= 4094.0f) raw_adc = 4094.0f;

    // R_ntc = R_ref / ((ADC_MAX / raw_adc) - 1)
    float r_ntc = NTC_R_REF / ((ADC_MAX_VAL / raw_adc) - 1.0f);

    // beta function: 1/T = 1/T0 + (1/B) * ln(R / R0)
    float steinhart = (1.0f / NTC_TO_KELVIN) + (1.0f / NTC_BETA) * logf(r_ntc / NTC_R0);
    float temp_celsius = (1.0f / steinhart) - 273.15f;

    return temp_celsius;
}

static bool heater_switched_on = false;

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
