#ifndef VOLTAGE_READER_H
#define VOLTAGE_READER_H

typedef enum {
    BATTERY_STATUS_UNINITIALIZED,
    BATTERY_STATUS_NORMAL,
    BATTERY_STATUS_LOW,
    BATTERY_STATUS_CRITICAL
} BATTERY_STATUS;

void voltageReaderInit(void);
float voltageReaderRead(void);
void voltageReaderPrintStatus(void);
BATTERY_STATUS voltageReaderGetStatus(void);

#endif
