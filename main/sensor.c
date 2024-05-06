#include <sht3x.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <esp_err.h>

#ifndef APP_CPU_NUM
#define APP_CPU_NUM PRO_CPU_NUM
#endif

static sht3x_t dev;
static const char *SENSOR_TAG = "SHT31 Sensor";

float temperature;
float humidity;

/*
 * User task that fetches latest measurement results of sensor every 2
 * seconds. It starts the SHT3x in periodic mode with 1 measurements per
 * second (*SHT3X_PERIODIC_1MPS*).
 */
void measure(void *pvParameters)
{
    esp_err_t res;

    // Start periodic measurements with 1 measurement per second.
    ESP_ERROR_CHECK(sht3x_start_measurement(&dev, SHT3X_PERIODIC_1MPS, SHT3X_HIGH));

    // Wait until first measurement is ready (constant time of at least 30 ms
    // or the duration returned from *sht3x_get_measurement_duration*).
    vTaskDelay(sht3x_get_measurement_duration(SHT3X_HIGH));

    TickType_t last_wakeup = xTaskGetTickCount();

    while (1)
    {
        // Get the values and do something with them.
        if ((res = sht3x_get_results(&dev, &temperature, &humidity)) == ESP_OK) {
            ESP_LOGI(SENSOR_TAG, "SHT3x Sensor: %.2f °C, %.2f %%\n", temperature, humidity);
            // printf("SHT3x Sensor: %.2f °C, %.2f %%\n", temperature, humidity);
        } else {
            // printf("Could not get results: %d (%s)", res, esp_err_to_name(res));
        }

        // Wait until 2 seconds (cycle time) are over.
        vTaskDelayUntil(&last_wakeup, pdMS_TO_TICKS(2000));
    }
}

void sensor_main()
{
    ESP_ERROR_CHECK(i2cdev_init());
    memset(&dev, 0, sizeof(sht3x_t));

    ESP_ERROR_CHECK(sht3x_init_desc(&dev, CONFIG_EXAMPLE_SHT3X_ADDR, 0, CONFIG_EXAMPLE_I2C_MASTER_SDA, CONFIG_EXAMPLE_I2C_MASTER_SCL));
    ESP_ERROR_CHECK(sht3x_init(&dev));

    xTaskCreatePinnedToCore(measure, "sh301x_measure", configMINIMAL_STACK_SIZE * 8, NULL, 5, NULL, APP_CPU_NUM);
}