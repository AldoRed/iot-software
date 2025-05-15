#include "driver/adc.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "ESP_A";

// UART1 on GPIO17/16
#define UARTx UART_NUM_1
#define TX_PIN GPIO_NUM_17
#define RX_PIN GPIO_NUM_16
#define BAUD_RATE 115200
#define BUF_SIZE 1024

// ADC on GPIO34 (ADC1_CHANNEL_6)
#define ADC_CHANNEL ADC1_CHANNEL_6
#define ADC_ATTEN ADC_ATTEN_DB_11
#define ADC_WIDTH ADC_WIDTH_BIT_12

static volatile bool sending = false;

static void uart_event_task(void *pv) {
  uint8_t buf[BUF_SIZE];
  while (1) {
    int len = uart_read_bytes(UARTx, buf, BUF_SIZE - 1, pdMS_TO_TICKS(1000));
    if (len > 0) {
      ESP_LOGI(TAG, "len=%d", len);
      buf[len] = '\0';
      ESP_LOGI(TAG, "RX cmd: '%s'", buf);
      if (strncmp((char *)buf, "START", 5) == 0) {
        sending = true;
        ESP_LOGI(TAG, "→ START");
      } else if (strncmp((char *)buf, "STOP", 4) == 0) {
        sending = false;
        ESP_LOGI(TAG, "→ STOP");
      }
    }
  }
}

void app_main(void) {
  // UART2 setup
  uart_config_t uc = {
      .baud_rate = BAUD_RATE,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS,
      .rx_flow_ctrl_thresh = 122,
  };
  ESP_ERROR_CHECK(uart_param_config(UARTx, &uc));
  ESP_ERROR_CHECK(uart_set_pin(UARTx, TX_PIN, RX_PIN, UART_PIN_NO_CHANGE,
                               UART_PIN_NO_CHANGE));
  ESP_ERROR_CHECK(uart_driver_install(UARTx, BUF_SIZE, BUF_SIZE, 0, NULL, 0));

  // ADC1 setup
  ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH));
  ESP_ERROR_CHECK(adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN));

  // Start UART event listener
  xTaskCreate(uart_event_task, "uart_evt", 4096, NULL, 10, NULL);

  // Main loop: sample & send
  char msg[64];
  while (1) {
    if (1 == 1) {
      int raw = adc1_get_raw(ADC_CHANNEL);
      float voltage = raw * (3.3f / 4095.0f);
      ESP_LOGI(TAG, "ADC raw: %d", raw);
      int len = snprintf(msg, sizeof(msg), "VOLT:%.3f\n", voltage);
      uart_write_bytes(UARTx, msg, len);
      ESP_LOGI(TAG, "TX → %s", msg);
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
