// Importar bibliotecas
#include "soc/gpio_num.h"
#include <freertos/FreeRTOS.h>
#include <esp_log.h>
#include <driver/gpio.h>

// Definir constantes
#define TAG "TAREA1"

#define BUTTON_GPIO GPIO_NUM_35
#define YELLOW_LED GPIO_NUM_5
#define GREEN_LED GPIO_NUM_18
#define RED_LED GPIO_NUM_19

// Tiempo máximo para considerar pulsaciones consecutivas (en ticks)
#define PRESS_INTERVAL (500 / portTICK_PERIOD_MS)

// Estados del semaforo
typedef enum {
    STATE_RED,
    STATE_YELLOW,
    STATE_GREEN
} TrafficState;

// Variables globales
static TrafficState current_state = STATE_RED;
static int press_count = 0;
static TickType_t last_press_time = 0;

// Estados del semaforo
void update_lights() {
    switch (current_state) {
        case STATE_RED:
            gpio_set_level(YELLOW_LED, 0);
            gpio_set_level(GREEN_LED, 0);
            gpio_set_level(RED_LED, 1);
            ESP_LOGI(TAG, "Luz roja encendida");
            break;
        case STATE_YELLOW:
            gpio_set_level(YELLOW_LED, 1);
            gpio_set_level(GREEN_LED, 0);
            gpio_set_level(RED_LED, 0);
            ESP_LOGI(TAG, "Luz amarilla encendida");
            break;
        case STATE_GREEN:
            gpio_set_level(YELLOW_LED, 0);
            gpio_set_level(GREEN_LED, 1);
            gpio_set_level(RED_LED, 0);
            ESP_LOGI(TAG, "Luz verde encendida");
            break;
    }
}

// Función para avanzar el estado del semáforo
void advance_state() {
    switch (current_state) {
        case STATE_RED:
            current_state = STATE_YELLOW;
            break;
        case STATE_YELLOW:
            current_state = STATE_GREEN;
            break;
        case STATE_GREEN:
            current_state = STATE_RED;
            break;
    }
    ESP_LOGI(TAG, "Cambio de estado hacia delante");
    update_lights();
}

// Función para retroceder el estado del semáforo
void retreat_state() {
    switch (current_state) {
        case STATE_RED:
            current_state = STATE_GREEN;
            break;
        case STATE_YELLOW:
            current_state = STATE_RED;
            break;
        case STATE_GREEN:
            current_state = STATE_YELLOW;
            break;
    }
    ESP_LOGI(TAG, "Cambio de estado hacia atrás");
    update_lights();
}

// Funcion principal
void app_main(void)
{
    // Inicializar GPIOs
    gpio_set_direction(YELLOW_LED, GPIO_MODE_OUTPUT);
    gpio_set_direction(GREEN_LED, GPIO_MODE_OUTPUT);
    gpio_set_direction(RED_LED,   GPIO_MODE_OUTPUT);
    gpio_set_direction(BUTTON_GPIO, GPIO_MODE_INPUT);

    update_lights();

    int last_button_state = 0;

    while (1)
    {
        // Escribimos en el monitor el estado actual
        ESP_LOGI(TAG, "Estado actual: %d", current_state);

        // Leer el estado del botón
        int current_button_state = gpio_get_level(BUTTON_GPIO);
        TickType_t current_time = xTaskGetTickCount();

        // Detectar flanco de bajada a subida
        if (current_button_state == 1 && last_button_state == 0) {
            // Se detectó una nueva pulsación
            // Si la pulsación es dentro del intervalo establecido, se acumula el conteo,
            // de lo contrario se reinicia el contador.
            if (current_time - last_press_time < PRESS_INTERVAL) {
                press_count++;
            } else {
                press_count = 1;
            }
            last_press_time = current_time;
        }
        last_button_state = current_button_state;

        // Verificar si pasó el intervalo sin nueva pulsación para ejecutar la acción correspondiente
        if ( (xTaskGetTickCount() - last_press_time) >= PRESS_INTERVAL && press_count > 0 ) {
            if (press_count == 2) {
                advance_state();
            } else if (press_count >= 3) {
                retreat_state();
            }
            // Reiniciar el contador
            press_count = 0;
        }

        // Pequeña demora para evitar consumir demasiados recursos
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}
