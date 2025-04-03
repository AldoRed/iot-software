#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/ledc.h"
#include "driver/gpio.h"

#define TAG "servo"
#define SERVO_GPIO GPIO_NUM_19

// Configuración PWM para el servo
#define LEDC_TIMER          LEDC_TIMER_0
#define LEDC_MODE           LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL        LEDC_CHANNEL_0
#define LEDC_DUTY_RES       LEDC_TIMER_16_BIT   // Resolución de 16 bits
#define LEDC_FREQUENCY      50                  // Frecuencia 50 Hz, típica para servos

// Anchos de pulso en microsegundos
#define SERVO_MIN_PULSEWIDTH 500   // Aproximadamente 0°
#define SERVO_MAX_PULSEWIDTH 2400  // Aproximadamente 180°

// Función para configurar el ángulo del servo (0° a 180°)
static void set_servo_angle(uint32_t angle)
{
    // Se calcula el ancho de pulso correspondiente al ángulo
    uint32_t pulse_width = SERVO_MIN_PULSEWIDTH +
        ((SERVO_MAX_PULSEWIDTH - SERVO_MIN_PULSEWIDTH) * angle) / 180;

    // La señal PWM tiene un periodo de 20,000 us (50 Hz)
    // Se calcula el duty cycle según la resolución configurada:
    uint32_t duty = (pulse_width * ((1 << LEDC_DUTY_RES) - 1)) / 20000;

    ESP_LOGI(TAG, "Angulo: %d - Pulso: %u us - Duty: %u", (int)angle, (unsigned int)pulse_width, (unsigned int)duty);

    // Se actualiza el duty cycle del canal LEDC
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
}

void app_main(void)
{
    // Configuración del timer LEDC
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Configuración del canal LEDC
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = SERVO_GPIO,
        .duty           = 0, // Inicio sin pulso
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    // Bucle principal: se mueve el servo a 0°, luego 90° y finalmente 180° con 5 segundos de espera en cada posición
    while (1) {
        set_servo_angle(0);
        vTaskDelay(pdMS_TO_TICKS(5000));
        set_servo_angle(90);
        vTaskDelay(pdMS_TO_TICKS(5000));
        set_servo_angle(180);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
