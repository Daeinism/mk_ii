#include "voltageReader.h"

#include <stdio.h>

#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#include "esp_adc/adc_oneshot.h" // “One-shot” means the ADC takes one measurement whenever the program asks for one.
#include "esp_err.h" // ESP Error Handling Library
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#define VOLTAGE_READER_ADC_UNIT ADC_UNIT_2
#define VOLTAGE_READER_ADC_CHANNEL ADC_CHANNEL_2 // On the ESP32-S3, GPIO 13 is connected to ADC2 channel 2.
#define VOLTAGE_DIVIDER_RATIO 4.96f // this is for calibration
#define BATTERY_FULL_VOLTAGE 12.1f // LittoKala Battery Full Voltage for 3S LiPo is 12.1V
#define BATTERY_WARNING_VOLTAGE 10.8f
#define BATTERY_CRITICAL_VOLTAGE 10.5f
#define BATTERY_PERCENTAGE_EMPTY_VOLTAGE 9.6f
#define BATTERY_LOW_RECOVERY_VOLTAGE 11.0f
#define BATTERY_CRITICAL_RECOVERY_VOLTAGE 10.7f
#define BATTERY_SAMPLE_INTERVAL_MS 200
#define BATTERY_AVERAGE_SAMPLE_COUNT 5
#define BATTERY_MONITOR_TASK_STACK_SIZE 2048
#define BATTERY_MONITOR_TASK_PRIORITY 1
    
static adc_oneshot_unit_handle_t voltageReaderAdcHandle = NULL; // Handle for the ADC unit used for voltage reading
static adc_cali_handle_t voltageReaderCalibrationHandle = NULL;
static SemaphoreHandle_t voltageReaderMutex = NULL;
static volatile BATTERY_STATUS batteryStatus = BATTERY_STATUS_UNINITIALIZED;
static float voltageReaderGetPercentage(float batteryVoltage);
static void voltageReaderMonitorTask(void *arg);
static void voltageReaderUpdateStatus(float batteryVoltage);

void voltageReaderInit(void)
{
    voltageReaderMutex = xSemaphoreCreateMutex();

    // ------------------------- ADC Unit Configuration --------------------------
    adc_oneshot_unit_init_cfg_t unitConfig = {
        .unit_id = VOLTAGE_READER_ADC_UNIT, // ADC_UNIT_2
        .ulp_mode = ADC_ULP_MODE_DISABLE // ulp = ultra-low-power
    };

    ESP_ERROR_CHECK(adc_oneshot_new_unit( // initializes the ADC unit
        &unitConfig, &voltageReaderAdcHandle // applying the configuration to the handle
    )); // ESP_ERROR_CHECK checks if the function call was successful
        

    // ------------------------- ADC Channel Configuration -----------------------
    adc_oneshot_chan_cfg_t channelConfig = {
        .atten = ADC_ATTEN_DB_12, // attenuation = lowering the input voltage internally to read higher voltages
        .bitwidth = ADC_BITWIDTH_DEFAULT // selecting the maximum supported bit width as the default bit width
    };

    ESP_ERROR_CHECK(adc_oneshot_config_channel(
        voltageReaderAdcHandle, // Handle
        VOLTAGE_READER_ADC_CHANNEL, // ADC_CHANNEL_2
        &channelConfig // Configuration
    )); // "Configure the channel of the handle with the specified channel configuration"

    // ------------------------- ADC Calibration Configuration -------------------
    adc_cali_curve_fitting_config_t calibrationConfig = {
        .unit_id = VOLTAGE_READER_ADC_UNIT,
        .chan = VOLTAGE_READER_ADC_CHANNEL,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT
    };

    ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting( // creates a calibration scheme for specific ADC unit and channel
        &calibrationConfig,
        &voltageReaderCalibrationHandle // this is not an actual read value. It's the calibraion ratio
    ));

    if (voltageReaderMutex == NULL ||
        xTaskCreate(voltageReaderMonitorTask,
                    "batteryMonitorTask",
                    BATTERY_MONITOR_TASK_STACK_SIZE,
                    NULL,
                    BATTERY_MONITOR_TASK_PRIORITY,
                    NULL) != pdPASS) {
        printf("Battery monitor task initialization failed\n");
    }
}
float voltageReaderRead(void)
{
    if (voltageReaderMutex != NULL) {
        xSemaphoreTake(voltageReaderMutex, portMAX_DELAY);
    }

    int rawValue;
    ESP_ERROR_CHECK(adc_oneshot_read(
        voltageReaderAdcHandle,
        VOLTAGE_READER_ADC_CHANNEL,
        &rawValue
    ));

    int gpioMillivolts;
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(
        voltageReaderCalibrationHandle,
        rawValue,
        &gpioMillivolts
    ));

    float batteryVoltage =
        ((float)gpioMillivolts / 1000.0f) * VOLTAGE_DIVIDER_RATIO;

    if (voltageReaderMutex != NULL) {
        xSemaphoreGive(voltageReaderMutex);
    }

    return batteryVoltage;
}

static float voltageReaderGetPercentage(float batteryVoltage)
{
    float percentage =
        ((batteryVoltage - BATTERY_PERCENTAGE_EMPTY_VOLTAGE) /
         (BATTERY_FULL_VOLTAGE - BATTERY_PERCENTAGE_EMPTY_VOLTAGE)) * 100.0f;

    if (percentage < 0.0f) {
        return 0.0f;
    }

    if (percentage > 100.0f) {
        return 100.0f;
    }

    return percentage;
}

void voltageReaderPrintStatus(void)
{
    float batteryVoltage = voltageReaderRead();
    float batteryPercentage = voltageReaderGetPercentage(batteryVoltage);

    if (batteryVoltage <= BATTERY_CRITICAL_VOLTAGE) {
        printf("Battery CRITICAL: %.0f%% (%.2f V)\n", batteryPercentage, batteryVoltage);
    }
    else if (batteryVoltage <= BATTERY_WARNING_VOLTAGE) {
        printf("Battery LOW: %.0f%% (%.2f V)\n", batteryPercentage, batteryVoltage);
    }
    else {
        printf("Battery: %.0f%% (%.2f V)\n", batteryPercentage, batteryVoltage);
    }
}

BATTERY_STATUS voltageReaderGetStatus(void)
{
    return batteryStatus;
}

static void voltageReaderMonitorTask(void *arg)
{
    (void)arg;

    float samples[BATTERY_AVERAGE_SAMPLE_COUNT] = {0};
    float sampleSum = 0.0f;
    int sampleIndex = 0;
    int sampleCount = 0;

    while (1) {
        float batteryVoltage = voltageReaderRead();
        sampleSum -= samples[sampleIndex];
        samples[sampleIndex] = batteryVoltage;
        sampleSum += batteryVoltage;
        sampleIndex = (sampleIndex + 1) % BATTERY_AVERAGE_SAMPLE_COUNT;

        if (sampleCount < BATTERY_AVERAGE_SAMPLE_COUNT) {
            sampleCount++;
        }

        if (sampleCount == BATTERY_AVERAGE_SAMPLE_COUNT) {
            voltageReaderUpdateStatus(
                sampleSum / (float)BATTERY_AVERAGE_SAMPLE_COUNT
            );
        }

        vTaskDelay(pdMS_TO_TICKS(BATTERY_SAMPLE_INTERVAL_MS));
    }
}

static void voltageReaderUpdateStatus(float batteryVoltage)
{
    switch (batteryStatus) {
        case BATTERY_STATUS_UNINITIALIZED:
        case BATTERY_STATUS_NORMAL:
            if (batteryVoltage <= BATTERY_CRITICAL_VOLTAGE) {
                batteryStatus = BATTERY_STATUS_CRITICAL;
            } else if (batteryVoltage <= BATTERY_WARNING_VOLTAGE) {
                batteryStatus = BATTERY_STATUS_LOW;
            } else {
                batteryStatus = BATTERY_STATUS_NORMAL;
            }
            break;

        case BATTERY_STATUS_LOW:
            if (batteryVoltage <= BATTERY_CRITICAL_VOLTAGE) {
                batteryStatus = BATTERY_STATUS_CRITICAL;
            } else if (batteryVoltage >= BATTERY_LOW_RECOVERY_VOLTAGE) {
                batteryStatus = BATTERY_STATUS_NORMAL;
            }
            break;

        case BATTERY_STATUS_CRITICAL:
            if (batteryVoltage >= BATTERY_LOW_RECOVERY_VOLTAGE) {
                batteryStatus = BATTERY_STATUS_NORMAL;
            } else if (batteryVoltage >= BATTERY_CRITICAL_RECOVERY_VOLTAGE) {
                batteryStatus = BATTERY_STATUS_LOW;
            }
            break;
    }
}
