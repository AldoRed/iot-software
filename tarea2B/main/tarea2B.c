#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "ESP_B";

// UART1 on GPIO17/16
#define UARTx UART_NUM_1
#define TX_PIN GPIO_NUM_17
#define RX_PIN GPIO_NUM_16
#define BAUD_RATE 115200
#define BUF_SIZE 1024

// I2C for HT16K33 on GPIO21/22
#define I2C_PORT I2C_NUM_0
#define SDA_PIN GPIO_NUM_21
#define SCL_PIN GPIO_NUM_22
#define HT16K33_ADDR 0x70

// Voltage threshold
#define THRESHOLD_V 1.65f

// 8×8 bitmaps for 'L' and 'R'
static const uint8_t bmp_L[8] = {0x10, 0x10, 0x10, 0x10,
                                 0x10, 0x10, 0x10, 0x1F};
static const uint8_t bmp_R[8] = {0x1F, 0x11, 0x11, 0x1F,
                                 0x14, 0x12, 0x11, 0x10};

static void ht16k33_init(void) {
  // oscillator on
  uint8_t on = 0x21, disp = 0x81, bright = 0xE7;
  i2c_master_write_to_device(I2C_PORT, HT16K33_ADDR, &on, 1, pdMS_TO_TICKS(50));
  i2c_master_write_to_device(I2C_PORT, HT16K33_ADDR, &disp, 1,
                             pdMS_TO_TICKS(50));
  i2c_master_write_to_device(I2C_PORT, HT16K33_ADDR, &bright, 1,
                             pdMS_TO_TICKS(50));
}

static void ht16k33_draw(const uint8_t *bmp) {
  uint8_t data[17];
  data[0] = 0x00;
  for (int i = 0; i < 8; i++) {
    data[1 + 2 * i] = bmp[i];
    data[1 + 2 * i + 1] = 0x00;
  }
  i2c_master_write_to_device(I2C_PORT, HT16K33_ADDR, data, sizeof(data),
                             pdMS_TO_TICKS(100));
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

  // I2C setup
  i2c_config_t ic = {.mode = I2C_MODE_MASTER,
                     .sda_io_num = SDA_PIN,
                     .scl_io_num = SCL_PIN,
                     .sda_pullup_en = GPIO_PULLUP_ENABLE,
                     .scl_pullup_en = GPIO_PULLUP_ENABLE,
                     .master.clk_speed = 100000};
  ESP_ERROR_CHECK(i2c_param_config(I2C_PORT, &ic));
  ESP_ERROR_CHECK(i2c_driver_install(I2C_PORT, ic.mode, 0, 0, 0));

  // Init display
  ht16k33_init();
  ht16k33_draw((uint8_t[8]){0}); // clear

  // Tell ESP-A to start
  const char *start = "START\n";
  uart_write_bytes(UARTx, start, strlen(start));
  ESP_LOGI(TAG, "TX cmd → START");

  // Main loop: read voltages and show L/R
  uint8_t buf[BUF_SIZE];
  while (1) {
    int len = uart_read_bytes(UARTx, buf, BUF_SIZE - 1, pdMS_TO_TICKS(1000));
    if (len > 0) {
      buf[len] = '\0';
      float v;
      if (sscanf((char *)buf, "VOLT:%f", &v) == 1) {
        ESP_LOGI(TAG, "Got VOLT=%.3f V", v);
        ht16k33_draw(v < THRESHOLD_V ? bmp_L : bmp_R);
      } else {
        ESP_LOGW(TAG, "Malformed: %s", buf);
      }
    }
  }
}