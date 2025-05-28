#include "driver/adc.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "ESP_A";

// UART2
#define UART_NUM UART_NUM_2
#define BUF_SIZE 1024
#define TASK_MEMORY 1024 * 2

// ADC on GPIO34 (ADC1_CHANNEL_6)
#define ADC_CHANNEL ADC1_CHANNEL_6
#define ADC_ATTEN ADC_ATTEN_DB_11
#define ADC_WIDTH ADC_WIDTH_BIT_12

static volatile bool sending = false;

static void uart_task(void *pvParameters) {
  uint8_t *data = (uint8_t *)malloc(BUF_SIZE);
  while (1) {
    int len =
        uart_read_bytes(UART_NUM, data, BUF_SIZE - 1, pdMS_TO_TICKS(1000));
    if (len > 0) {
      ESP_LOGI(TAG, "len=%d", len);
      data[len] = '\0';
      ESP_LOGI(TAG, "RX cmd: '%s'", data);
      if (strncmp((char *)data, "START", 5) == 0) {
        sending = true;
        uart_write_bytes(UART_NUM, "ACK: START\n", 11);
        ESP_LOGI(TAG, "→ START");
      } else if (strncmp((char *)data, "STOP", 4) == 0) {
        sending = false;
        ESP_LOGI(TAG, "→ STOP");
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

void app_main(void) {
  init_uart();
  char msg[64];
  while (1) {
    if (sending) {
      int raw = adc1_get_raw(ADC_CHANNEL);
      float voltage = raw * (3.3f / 4095.0f);
      ESP_LOGI(TAG, "ADC raw: %d", raw);
      int len = snprintf(msg, sizeof(msg), "VOLT:%.3f\n", voltage);
      uart_write_bytes(UART_NUM, msg, len);
      ESP_LOGI(TAG, "TX → %s", msg);
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
