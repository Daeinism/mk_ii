#include "servo.h"

#include <stdint.h>

#include "driver/gpio.h"
#include "driver/ledc.h"

#define SERVO_GPIO GPIO_NUM_3
#define SERVO_MIN_ANGLE 0
#define SERVO_MAX_ANGLE 180
#define SERVO_MIN_PULSE_US 600
#define SERVO_MAX_PULSE_US 2400
#define SERVO_PWM_FREQUENCY 50
#define SERVO_PWM_PERIOD_US 20000
#define SERVO_PWM_MAX_DUTY 16383

void servoInit(void)
{
    ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_2,
        .duty_resolution = LEDC_TIMER_14_BIT,
        .freq_hz = SERVO_PWM_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer);

    ledc_channel_config_t channel = {
        .gpio_num = SERVO_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_6,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_2,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&channel);
}

void servoSetAngle(int angle)
{
    if (angle < SERVO_MIN_ANGLE) {
        angle = SERVO_MIN_ANGLE;
    }
    else if (angle > SERVO_MAX_ANGLE) {
        angle = SERVO_MAX_ANGLE;
    }

    uint32_t pulseWidth = SERVO_MIN_PULSE_US +
        (uint32_t)(angle * (SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US) / SERVO_MAX_ANGLE);
    uint32_t duty = pulseWidth * SERVO_PWM_MAX_DUTY / SERVO_PWM_PERIOD_US;

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_6, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_6);
}
