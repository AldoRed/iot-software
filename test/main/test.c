#include "soc/gpio_num.h"
#include <driver/gpio.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>

#define TAG "TEST"

enum PeatonalState { PEATONAL_STATE_ON, PEATONAL_STATE_OFF };

void app_main(void) {
  gpio_set_direction(GPIO_NUM_5, GPIO_MODE_OUTPUT);
  gpio_set_direction(GPIO_NUM_18, GPIO_MODE_OUTPUT);
  gpio_set_direction(GPIO_NUM_19, GPIO_MODE_OUTPUT);
  gpio_set_direction(GPIO_NUM_35, GPIO_MODE_INPUT);

  // Set the GPIO level to LOW (Good practice)
  gpio_set_level(GPIO_NUM_5, 0);
  gpio_set_level(GPIO_NUM_18, 0);
  gpio_set_level(GPIO_NUM_19, 0);

  while (1) {
    switch (gpio_get_level(GPIO_NUM_35)) {
    case PEATONAL_STATE_ON:
      // Encender amarillo
      gpio_set_level(GPIO_NUM_5, 1);
      gpio_set_level(GPIO_NUM_18, 0);
      gpio_set_level(GPIO_NUM_19, 0);
      vTaskDelay(2000 / portTICK_PERIOD_MS);
      gpio_set_level(GPIO_NUM_5, 0);
      gpio_set_level(GPIO_NUM_18, 1);
      gpio_set_level(GPIO_NUM_19, 0);
      ESP_LOGI(TAG, "Luz verde peatonal encendida");
      vTaskDelay(2000 / portTICK_PERIOD_MS);
      break;
    case PEATONAL_STATE_OFF:
      gpio_set_level(GPIO_NUM_5, 0);
      gpio_set_level(GPIO_NUM_18, 0);
      gpio_set_level(GPIO_NUM_19, 1);
      ESP_LOGI(TAG, "Luz Roja peatonal encendida");
      break;
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
