/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2018 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS products only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
/* HomeKit Sensor Example with custom data/tlv8 characteristics
*/
#include <stdio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

#include <hap.h>
#include <hap_apple_servs.h>
#include <hap_apple_chars.h>

#include <iot_button.h>

#include <app_wifi.h>
#include <app_hap_setup_payload.h>

#include <sensor.c>

static const char *TAG = "HAP Sensor";

#define SENSOR_TASK_PRIORITY  1
#define SENSOR_TASK_STACKSIZE 4 * 1024
#define SENSOR_TASK_NAME      "hap_sensor"

/* Reset network credentials if button is pressed for more than 3 seconds and then released */
#define RESET_NETWORK_BUTTON_TIMEOUT        3

/* Reset to factory if button is pressed and held for more than 10 seconds */
#define RESET_TO_FACTORY_BUTTON_TIMEOUT     10

/* The button "Boot" will be used as the Reset button for the example */
#define RESET_GPIO  GPIO_NUM_0
/**
 * @brief The network reset button callback handler.
 * Useful for testing the Wi-Fi re-configuration feature of WAC2
 */
static void reset_network_handler(void* arg)
{
    hap_reset_network();
}
/**
 * @brief The factory reset button callback handler.
 */
static void reset_to_factory_handler(void* arg)
{
    hap_reset_to_factory();
}

/**
 * The Reset button  GPIO initialisation function.
 * Same button will be used for resetting Wi-Fi network as well as for reset to factory based on
 * the time for which the button is pressed.
 */
static void reset_key_init(uint32_t key_gpio_pin)
{
    button_handle_t handle = iot_button_create(key_gpio_pin, BUTTON_ACTIVE_LOW);
    iot_button_add_on_release_cb(handle, RESET_NETWORK_BUTTON_TIMEOUT, reset_network_handler, NULL);
    iot_button_add_on_press_cb(handle, RESET_TO_FACTORY_BUTTON_TIMEOUT, reset_to_factory_handler, NULL);
}


/* Mandatory identify routine for the accessory.
 * In a real accessory, something like LED blink should be implemented
 * got visual identification
 */
static int sensor_identify(hap_acc_t *ha)
{
    ESP_LOGI(TAG, "Sensor identified");
    return HAP_SUCCESS;
}

/* A dummy callback for handling a read on the "Temperature" characteristic of Sensor.
 * In an actual accessory, this should read from the hardware
 */
static int sensor_read_t(hap_char_t *hc, hap_status_t *status_code, void *serv_priv, void *read_priv)
{
    if (!strcmp(hap_char_get_type_uuid(hc), HAP_CHAR_UUID_CURRENT_TEMPERATURE)) {
        ESP_LOGI(TAG, "Received Read for Current Temperature");
        const hap_val_t *cur_val = hap_char_get_val(hc);
        ESP_LOGI(TAG, "Current Value: %f", cur_val->f);
        /* We are setting a fummy value here, which is 1 more than the current value.
         * If the value overflows max limit of 100, we reset it to 0
         * TODO: Read from actual hardware
         */
        hap_val_t new_val;
        new_val.f = temperature;
        ESP_LOGI(TAG, "Updated Value: %f", new_val.f);
        hap_char_update_val(hc, &new_val);
        *status_code = HAP_STATUS_SUCCESS;
        return HAP_SUCCESS;
    } else {
        *status_code = HAP_STATUS_RES_ABSENT;
    }
    return HAP_FAIL;
}

/* A dummy callback for handling a read on the "Humidity" characteristic of Sensor.
 * In an actual accessory, this should read from the hardware
 */
static int sensor_read_h(hap_char_t *hc, hap_status_t *status_code, void *serv_priv, void *read_priv)
{
    if (!strcmp(hap_char_get_type_uuid(hc), HAP_CHAR_UUID_CURRENT_RELATIVE_HUMIDITY)) {
        ESP_LOGI(TAG, "Received Read for Current Humidity");
        const hap_val_t *cur_val = hap_char_get_val(hc);
        ESP_LOGI(TAG, "Current Value: %f", cur_val->f);
        /* We are setting a fummy value here, which is 1 more than the current value.
         * If the value overflows max limit of 100, we reset it to 0
         * TODO: Read from actual hardware
         */
        hap_val_t new_val;
        new_val.f = humidity;
        ESP_LOGI(TAG, "Updated Value: %f", new_val.f);
        hap_char_update_val(hc, &new_val);
        *status_code = HAP_STATUS_SUCCESS;
        return HAP_SUCCESS;
    } else {
        *status_code = HAP_STATUS_RES_ABSENT;
    }
    return HAP_FAIL;
}

/*The main thread for handling the Sensor Accessory */
static void sensor_thread_entry(void *p)
{
    hap_acc_t *accessory;
    hap_serv_t *service;

    sensor_main();

    /* Initialize the HAP core */
    hap_init(HAP_TRANSPORT_WIFI);

    /* Initialise the mandatory parameters for Accessory which will be added as
     * the mandatory services internally
     */
    hap_acc_cfg_t cfg = {
        .name = "Esp-Temperature-Sensor",
        .manufacturer = "Espressif",
        .model = "EspSensor-T01",
        .serial_num = "001122334466",
        .fw_rev = "0.9.0",
        .hw_rev = NULL,
        .pv = "1.1.0",
        .identify_routine = sensor_identify,
        .cid = HAP_CID_SENSOR,
    };
    /* Create accessory object */
    accessory = hap_acc_create(&cfg);

    /* Add a dummy Product Data */
    uint8_t product_data[] = {'E','S','P','3','2','H','A','P'};
    hap_acc_add_product_data(accessory, product_data, sizeof(product_data));

    /* Add Wi-Fi Transport service required for HAP Spec R16 */
    hap_acc_add_wifi_transport_service(accessory, 0);

    /* Create the Sensor Service. Include the "name" since this is a user visible service  */
    service = hap_serv_temperature_sensor_create(10);
    hap_serv_add_char(service, hap_char_name_create("My Temperature Sensor"));

    /* Set the read callback for the service */
    hap_serv_set_read_cb(service, sensor_read_t);

    /* Add the Sensor Service to the Accessory Object */
    hap_acc_add_serv(accessory, service);

    /* Create the Sensor Service. Include the "name" since this is a user visible service  */
    service = hap_serv_humidity_sensor_create(10);
    hap_serv_add_char(service, hap_char_name_create("My Humidity Sensor"));

    /* Set the read callback for the service */
    hap_serv_set_read_cb(service, sensor_read_h);

    /* Add the Sensor Service to the Accessory Object */
    hap_acc_add_serv(accessory, service);

    /* Add the Accessory to the HomeKit Database */
    hap_add_accessory(accessory);

    /* Use the setup_payload_gen tool to get the QR code for Accessory Setup.
     * The payload below is for a Sensor with setup code 111-22-333 and setup id ES32
     */
    ESP_LOGI(TAG, "Use setup payload: \"X-HM://00AHJA6JHES32\" for Accessory Setup");

    /* Register a common button for reset Wi-Fi network and reset to factory.
     */
    reset_key_init(RESET_GPIO);

    /* For production accessories, the setup code shouldn't be programmed on to
     * the device. Instead, the setup info, derived from the setup code must
     * be used. Use the factory_nvs_gen utility to generate this data and then
     * flash it into the factory NVS partition.
     *
     * By default, the setup ID and setup info will be read from the factory_nvs
     * Flash partition and so, is not required to set here explicitly.
     *
     * However, for testing purpose, this can be overridden by using hap_set_setup_code()
     * and hap_set_setup_id() APIs, as has been done here.
     */
#ifdef CONFIG_EXAMPLE_USE_HARDCODED_SETUP_CODE
    /* Unique Setup code of the format xxx-xx-xxx. Default: 111-22-333 */
    hap_set_setup_code(CONFIG_EXAMPLE_SETUP_CODE);
    /* Unique four character Setup Id. Default: ES32 */
    hap_set_setup_id(CONFIG_EXAMPLE_SETUP_ID);
#ifdef CONFIG_APP_WIFI_USE_WAC_PROVISIONING
    app_hap_setup_payload(CONFIG_EXAMPLE_SETUP_CODE, CONFIG_EXAMPLE_SETUP_ID, true, cfg.cid);
#else
    app_hap_setup_payload(CONFIG_EXAMPLE_SETUP_CODE, CONFIG_EXAMPLE_SETUP_ID, false, cfg.cid);
#endif
#endif

    /* Enable Hardware MFi authentication (applicable only for MFi variant of SDK) */
    hap_enable_mfi_auth(HAP_MFI_AUTH_HW);

    /* Initialize Wi-Fi */
    app_wifi_init();

    /* After all the initializations are done, start the HAP core */
    hap_start();
    /* Start Wi-Fi */
    app_wifi_start(portMAX_DELAY);
    /* The task ends here. The read/write callbacks will be invoked by the HAP Framework */
    vTaskDelete(NULL);

}

void app_main()
{
    /* Create the application thread */
    xTaskCreate(sensor_thread_entry, SENSOR_TASK_NAME, SENSOR_TASK_STACKSIZE,
                NULL, SENSOR_TASK_PRIORITY, NULL);
}
