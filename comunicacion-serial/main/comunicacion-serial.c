#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "ESP_MAIN";
#define ledR 14
#define ledY 12
#define ledB 13

#define UART_NUM UART_NUM_2
#define BUF_SIZE 1024
#define TASK_MEMORY 1024 * 2

static void uart_task(void *pvParameters) {
  uint8_t *data = (uint8_t *)malloc(BUF_SIZE);

  while (1) {
    bzero(data, BUF_SIZE);
    // Write to UART
    ESP_LOGI(TAG, "Waiting for data on UART...");
    // Write randomly 'ON', 'OFF', 'YELLOW', 'BLUE', 'RESET'
    const char *commands[] = {"ON", "OFF", "YELLOW", "BLUE", "RESET"};
    int random_index = esp_random() % 5;
    uart_write_bytes(UART_NUM, commands[random_index],
                     strlen(commands[random_index]));
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Read data from UART
    int len =
        uart_read_bytes(UART_NUM, data, BUF_SIZE - 1, pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG, "Read %d bytes from UART", len);
    if (len > 0) {
      data[len] = '\0'; // Null-terminate the string
      ESP_LOGI(TAG, "Received: '%s'", data);

      if (strncmp((char *)data, "ON", 2) == 0) {
        gpio_set_level(ledR, 1);
        ESP_LOGI(TAG, "LED Red ON");
      } else if (strncmp((char *)data, "OFF", 3) == 0) {
        gpio_set_level(ledR, 0);
        ESP_LOGI(TAG, "LED Red OFF");
      } else if (strncmp((char *)data, "YELLOW", 6) == 0) {
        gpio_set_level(ledY, 1);
        ESP_LOGI(TAG, "LED Yellow ON");
      } else if (strncmp((char *)data, "BLUE", 4) == 0) {
        gpio_set_level(ledB, 1);
        ESP_LOGI(TAG, "LED Blue ON");
      } else if (strncmp((char *)data, "RESET", 5) == 0) {
        gpio_set_level(ledR, 0);
        gpio_set_level(ledY, 0);
        gpio_set_level(ledB, 0);
        ESP_LOGI(TAG, "LEDs RESET");
      }
    }
  }
}

static void init_uart(void) {
  uart_config_t uart_config = {
      .baud_rate = 115200,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .source_clk = UART_SCLK_APB,
  };
  ESP_ERROR_CHECK(uart_param_config(UART_NUM, &uart_config));
  ESP_ERROR_CHECK(
      uart_set_pin(UART_NUM, 5, 4, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
  ESP_ERROR_CHECK(
      uart_driver_install(UART_NUM, BUF_SIZE, BUF_SIZE, 0, NULL, 0));

  xTaskCreate(uart_task, "uart_task", TASK_MEMORY, NULL, 10, NULL);
  ESP_LOGI(TAG, "UART initialized");
}

static void init_led(void) {
  gpio_reset_pin(ledR);
  gpio_set_direction(ledR, GPIO_MODE_OUTPUT);
  gpio_reset_pin(ledY);
  gpio_set_direction(ledY, GPIO_MODE_OUTPUT);
  gpio_reset_pin(ledB);
  gpio_set_direction(ledB, GPIO_MODE_OUTPUT);

  ESP_LOGI(TAG, "Init led completed");
}

void app_main(void) {
  init_led();
  init_uart();
}