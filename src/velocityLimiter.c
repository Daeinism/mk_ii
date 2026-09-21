#include "velocityLimiter.h"

#include <stddef.h>

static int32_t roundCount(float count);

void velocityLimiterInit(
    VelocityLimiterState *state,
    int32_t initialCount,
    float maxVelocityCountsPerSecond
)
{
    if (state == NULL) {
        return;
    }

    state->activeTargetCount = (float)initialCount;
    state->maxVelocityCountsPerSecond = maxVelocityCountsPerSecond;
}

int32_t velocityLimiterUpdate(
    VelocityLimiterState *state,
    int32_t finalTargetCount,
    float deltaTime
)
{
    if (state == NULL) {
        return finalTargetCount;
    }

    float maxStep = state->maxVelocityCountsPerSecond * deltaTime;
    if (maxStep <= 0.0f) {
        return roundCount(state->activeTargetCount);
    }

    float remainingCount = (float)finalTargetCount - state->activeTargetCount;

    if (remainingCount > maxStep) {
        state->activeTargetCount += maxStep;
    } else if (remainingCount < -maxStep) {
        state->activeTargetCount -= maxStep;
    } else {
        state->activeTargetCount = (float)finalTargetCount;
    }

    return roundCount(state->activeTargetCount);
}

void velocityLimiterReset(VelocityLimiterState *state, int32_t currentCount)
{
    if (state == NULL) {
        return;
    }

    state->activeTargetCount = (float)currentCount;
}

static int32_t roundCount(float count)
{
    if (count >= 0.0f) {
        return (int32_t)(count + 0.5f);
    }

    return (int32_t)(count - 0.5f);
}
