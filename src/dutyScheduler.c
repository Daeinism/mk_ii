#include "dutyScheduler.h"

#include <stdbool.h>
#include <stddef.h>

int dutySchedulerGetMinimumDuty(
    const DutySchedulerConfig *config,
    float jointAngleDegrees,
    int32_t jointPositionErrorCount
)
{
    if (config == NULL) {
        return 0;
    }

    bool movingAwayFromCenter =
        (jointAngleDegrees > 0.0f && jointPositionErrorCount > 0) ||
        (jointAngleDegrees < 0.0f && jointPositionErrorCount < 0);

    if (!movingAwayFromCenter) {
        return config->baseMinimumDuty;
    }

    float absoluteJointAngle = jointAngleDegrees;
    if (absoluteJointAngle < 0.0f) {
        absoluteJointAngle = -absoluteJointAngle;
    }

    if (absoluteJointAngle <= config->resistanceStartDegrees) {
        return config->baseMinimumDuty;
    }
    if (absoluteJointAngle >= config->resistanceFullDegrees) {
        return config->resistanceMinimumDuty;
    }

    float transitionRange =
        config->resistanceFullDegrees - config->resistanceStartDegrees;
    if (transitionRange <= 0.0f) {
        return config->resistanceMinimumDuty;
    }

    float transitionRatio =
        (absoluteJointAngle - config->resistanceStartDegrees) / transitionRange;
    float smoothRatio =
        transitionRatio * transitionRatio * (3.0f - (2.0f * transitionRatio));
    float scheduledDuty =
        config->baseMinimumDuty +
        ((config->resistanceMinimumDuty - config->baseMinimumDuty) * smoothRatio);

    return (int)(scheduledDuty + 0.5f);
}
