#include "wifiCredentialStore.h"

#include <stdio.h>
#include <string.h>

#include "esp_err.h"
#include "nvs.h"

#define WIFI_CREDENTIAL_STORE_NAMESPACE "scara_wifi"

static const char *ssidKeys[WIFI_CREDENTIAL_STORE_MAX_ENTRIES] = {
    "ssid0",
    "ssid1"
};
static const char *passwordKeys[WIFI_CREDENTIAL_STORE_MAX_ENTRIES] = {
    "pass0",
    "pass1"
};

static bool writeCredentials(
    nvs_handle_t handle,
    const WifiStoredCredential credentials[WIFI_CREDENTIAL_STORE_MAX_ENTRIES]
);

size_t wifiCredentialStoreLoad(
    WifiStoredCredential credentials[WIFI_CREDENTIAL_STORE_MAX_ENTRIES]
)
{
    if (credentials == NULL) {
        return 0;
    }

    memset(credentials,
           0,
           sizeof(WifiStoredCredential) * WIFI_CREDENTIAL_STORE_MAX_ENTRIES);

    nvs_handle_t handle;
    esp_err_t openResult = nvs_open(
        WIFI_CREDENTIAL_STORE_NAMESPACE,
        NVS_READONLY,
        &handle
    );
    if (openResult == ESP_ERR_NVS_NOT_FOUND) {
        return 0;
    }
    if (openResult != ESP_OK) {
        printf("Wi-Fi credential load failed: %s\n", esp_err_to_name(openResult));
        return 0;
    }

    size_t loadedCount = 0;
    for (size_t i = 0; i < WIFI_CREDENTIAL_STORE_MAX_ENTRIES; i++) {
        size_t ssidLength = sizeof(credentials[i].ssid);
        size_t passwordLength = sizeof(credentials[i].password);
        esp_err_t ssidResult = nvs_get_str(
            handle,
            ssidKeys[i],
            credentials[i].ssid,
            &ssidLength
        );
        esp_err_t passwordResult = nvs_get_str(
            handle,
            passwordKeys[i],
            credentials[i].password,
            &passwordLength
        );

        if (ssidResult == ESP_OK && passwordResult == ESP_OK &&
            credentials[i].ssid[0] != '\0') {
            credentials[i].valid = true;
            loadedCount++;
        }
    }

    nvs_close(handle);
    return loadedCount;
}

bool wifiCredentialStoreSave(const char *ssid, const char *password)
{
    if (ssid == NULL || password == NULL || ssid[0] == '\0' ||
        strlen(ssid) >= WIFI_CREDENTIAL_STORE_SSID_LENGTH ||
        strlen(password) >= WIFI_CREDENTIAL_STORE_PASSWORD_LENGTH) {
        return false;
    }

    WifiStoredCredential stored[WIFI_CREDENTIAL_STORE_MAX_ENTRIES];
    wifiCredentialStoreLoad(stored);

    if (stored[0].valid && strcmp(stored[0].ssid, ssid) == 0 &&
        strcmp(stored[0].password, password) == 0) {
        return true;
    }

    WifiStoredCredential updated[WIFI_CREDENTIAL_STORE_MAX_ENTRIES] = {0};
    snprintf(updated[0].ssid, sizeof(updated[0].ssid), "%s", ssid);
    snprintf(updated[0].password, sizeof(updated[0].password), "%s", password);
    updated[0].valid = true;

    size_t destinationIndex = 1;
    for (size_t i = 0;
         i < WIFI_CREDENTIAL_STORE_MAX_ENTRIES &&
         destinationIndex < WIFI_CREDENTIAL_STORE_MAX_ENTRIES;
         i++) {
        if (stored[i].valid && strcmp(stored[i].ssid, ssid) != 0) {
            updated[destinationIndex] = stored[i];
            destinationIndex++;
        }
    }

    nvs_handle_t handle;
    esp_err_t openResult = nvs_open(
        WIFI_CREDENTIAL_STORE_NAMESPACE,
        NVS_READWRITE,
        &handle
    );
    if (openResult != ESP_OK) {
        printf("Wi-Fi credential save failed: %s\n", esp_err_to_name(openResult));
        return false;
    }

    bool saved = writeCredentials(handle, updated);
    nvs_close(handle);
    return saved;
}

static bool writeCredentials(
    nvs_handle_t handle,
    const WifiStoredCredential credentials[WIFI_CREDENTIAL_STORE_MAX_ENTRIES]
)
{
    for (size_t i = 0; i < WIFI_CREDENTIAL_STORE_MAX_ENTRIES; i++) {
        esp_err_t ssidResult;
        esp_err_t passwordResult;

        if (credentials[i].valid) {
            ssidResult = nvs_set_str(handle, ssidKeys[i], credentials[i].ssid);
            passwordResult = nvs_set_str(
                handle,
                passwordKeys[i],
                credentials[i].password
            );
        } else {
            ssidResult = nvs_erase_key(handle, ssidKeys[i]);
            passwordResult = nvs_erase_key(handle, passwordKeys[i]);

            if (ssidResult == ESP_ERR_NVS_NOT_FOUND) {
                ssidResult = ESP_OK;
            }
            if (passwordResult == ESP_ERR_NVS_NOT_FOUND) {
                passwordResult = ESP_OK;
            }
        }

        if (ssidResult != ESP_OK || passwordResult != ESP_OK) {
            printf("Wi-Fi credential save failed\n");
            return false;
        }
    }

    esp_err_t commitResult = nvs_commit(handle);
    if (commitResult != ESP_OK) {
        printf("Wi-Fi credential commit failed: %s\n",
               esp_err_to_name(commitResult));
        return false;
    }

    return true;
}
