#ifndef DUTY_SCHEDULER_H
#define DUTY_SCHEDULER_H

#include <stdint.h>

typedef struct {
    float resistanceStartDegrees;
    float resistanceFullDegrees;
    int baseMinimumDuty;
    int resistanceMinimumDuty;
} DutySchedulerConfig;

int dutySchedulerGetMinimumDuty(
    const DutySchedulerConfig *config,
    float jointAngleDegrees,
    int32_t jointPositionErrorCount
);

#endif
