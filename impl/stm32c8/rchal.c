#include "include/rchal.h"
#include "stm32f1xx_hal.h"
#include "microtime.h"

#define TM1637_GPIO_PORT    GPIOB
#define TM1637_CLK_PIN      GPIO_PIN_10
#define TM1637_DIO_PIN      GPIO_PIN_11

#define BTNS_GPIO_PORT      GPIOA
#define BTN_SELECT_PIN      GPIO_PIN_3
#define BTN_CANCEL_PIN      GPIO_PIN_4
#define BTN_OPTIONS_PIN     GPIO_PIN_5
#define BTN_INCR_PIN        GPIO_PIN_6
#define BTN_DECR_PIN        GPIO_PIN_7

void rc_init_rchal(void) {
    // enable ports (we use B for now)
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // TM1637 DIO/CLK pins config
    HAL_GPIO_WritePin(TM1637_GPIO_PORT, TM1637_CLK_PIN | TM1637_DIO_PIN, GPIO_PIN_SET); // normalize first
    GPIO_InitTypeDef tm1637_pins_conf = {0}; // now can config
    tm1637_pins_conf.Mode = GPIO_MODE_OUTPUT_OD; // open drain
    tm1637_pins_conf.Pull = GPIO_PULLUP; // the TM1637 module has a pull up resistor already
    tm1637_pins_conf.Speed = GPIO_SPEED_FREQ_HIGH;
    tm1637_pins_conf.Pin = TM1637_CLK_PIN | TM1637_DIO_PIN;
    HAL_GPIO_Init(TM1637_GPIO_PORT, &tm1637_pins_conf);
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

void rc_buzzer_play(void) {

}

float rc_sensor_get_temp(const rc_temp_sensor_t sensor) {
    return 36.0f;
}

static bool dummy;
void rc_heater_set(const bool on) {
    dummy = on;
}

bool rc_heater_is_on(void) {
    return dummy;
}

uint32_t rc_time_current_millis(void) {
    return HAL_GetTick();
}
