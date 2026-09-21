#ifndef WIFI_CREDENTIAL_STORE_H
#define WIFI_CREDENTIAL_STORE_H

#include <stdbool.h>
#include <stddef.h>

#define WIFI_CREDENTIAL_STORE_MAX_ENTRIES 2
#define WIFI_CREDENTIAL_STORE_SSID_LENGTH 33
#define WIFI_CREDENTIAL_STORE_PASSWORD_LENGTH 64

typedef struct {
    char ssid[WIFI_CREDENTIAL_STORE_SSID_LENGTH];
    char password[WIFI_CREDENTIAL_STORE_PASSWORD_LENGTH];
    bool valid;
} WifiStoredCredential;

size_t wifiCredentialStoreLoad(
    WifiStoredCredential credentials[WIFI_CREDENTIAL_STORE_MAX_ENTRIES]
);
bool wifiCredentialStoreSave(const char *ssid, const char *password);

#endif
