#ifndef VELOCITY_LIMITER_H
#define VELOCITY_LIMITER_H

#include <stdint.h>

typedef struct {
    float activeTargetCount;
    float maxVelocityCountsPerSecond;
} VelocityLimiterState;

void velocityLimiterInit(
    VelocityLimiterState *state,
    int32_t initialCount,
    float maxVelocityCountsPerSecond
);
int32_t velocityLimiterUpdate(
    VelocityLimiterState *state,
    int32_t finalTargetCount,
    float deltaTime
);
void velocityLimiterReset(VelocityLimiterState *state, int32_t currentCount);

#endif
