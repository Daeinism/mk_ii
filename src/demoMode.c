#include "demoMode.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "limitSwitch.h"
#include "motor.h"
#include "scaraMotion.h"
#include "servo.h"

#define DEMO_TASK_STACK_SIZE 3072
#define DEMO_TASK_PRIORITY 1
#define DEMO_WAIT_SLICE_MS 20

typedef enum {
    DEMO_STEP_MOVE,
    DEMO_STEP_PEN_UP,
    DEMO_STEP_PEN_DOWN,
    DEMO_STEP_WAIT
} DemoStepType;

typedef struct {
    DemoStepType type;
    double theta1Degrees;
    double theta2Degrees;
    uint32_t waitMs;
} DemoStep;

static void demoTask(void *arg);
static bool executeDemoStep(const DemoStep *step);
static bool waitWhileRunning(uint32_t waitMs);
static void finishAfterFailure(void);

static const DemoStep demoSteps[] = {
    {DEMO_STEP_PEN_UP, 0.0, 0.0, 0},
    {DEMO_STEP_MOVE, -40.0, 40.0, 0},
    {DEMO_STEP_PEN_DOWN, 0.0, 0.0, 0},
    {DEMO_STEP_WAIT, 0.0, 0.0, 300},
    {DEMO_STEP_PEN_UP, 0.0, 0.0, 0},
    {DEMO_STEP_WAIT, 0.0, 0.0, 200},
    {DEMO_STEP_MOVE, 0.0, 70.0, 0},
    {DEMO_STEP_PEN_DOWN, 0.0, 0.0, 0},
    {DEMO_STEP_WAIT, 0.0, 0.0, 300},
    {DEMO_STEP_PEN_UP, 0.0, 0.0, 0},
    {DEMO_STEP_WAIT, 0.0, 0.0, 200},
    {DEMO_STEP_MOVE, 40.0, 40.0, 0},
    {DEMO_STEP_PEN_DOWN, 0.0, 0.0, 0},
    {DEMO_STEP_WAIT, 0.0, 0.0, 300},
    {DEMO_STEP_PEN_UP, 0.0, 0.0, 0},
    {DEMO_STEP_WAIT, 0.0, 0.0, 200},
    {DEMO_STEP_MOVE, 60.0, -60.0, 0},
    {DEMO_STEP_PEN_DOWN, 0.0, 0.0, 0},
    {DEMO_STEP_WAIT, 0.0, 0.0, 300},
    {DEMO_STEP_PEN_UP, 0.0, 0.0, 0},
    {DEMO_STEP_WAIT, 0.0, 0.0, 200},
    {DEMO_STEP_MOVE, 20.0, -140.0, 0},
    {DEMO_STEP_PEN_DOWN, 0.0, 0.0, 0},
    {DEMO_STEP_WAIT, 0.0, 0.0, 300},
    {DEMO_STEP_PEN_UP, 0.0, 0.0, 0},
    {DEMO_STEP_WAIT, 0.0, 0.0, 200},
    {DEMO_STEP_MOVE, -30.0, -90.0, 0},
    {DEMO_STEP_PEN_DOWN, 0.0, 0.0, 0},
    {DEMO_STEP_WAIT, 0.0, 0.0, 300},
    {DEMO_STEP_PEN_UP, 0.0, 0.0, 0},
    {DEMO_STEP_WAIT, 0.0, 0.0, 200},
    {DEMO_STEP_MOVE, -60.0, 80.0, 0},
    {DEMO_STEP_PEN_DOWN, 0.0, 0.0, 0},
    {DEMO_STEP_WAIT, 0.0, 0.0, 300},
    {DEMO_STEP_PEN_UP, 0.0, 0.0, 0},
    {DEMO_STEP_WAIT, 0.0, 0.0, 200},
    {DEMO_STEP_MOVE, 30.0, 140.0, 0},
    {DEMO_STEP_PEN_DOWN, 0.0, 0.0, 0},
    {DEMO_STEP_WAIT, 0.0, 0.0, 300},
    {DEMO_STEP_PEN_UP, 0.0, 0.0, 0},
    {DEMO_STEP_WAIT, 0.0, 0.0, 200},
    {DEMO_STEP_MOVE, 0.0, 0.0, 0},
    {DEMO_STEP_WAIT, 0.0, 0.0, 500}
};

static TaskHandle_t demoTaskHandle = NULL;
static volatile bool demoRunning = false;

bool demoModeInit(void)
{
    if (demoTaskHandle != NULL) {
        return true;
    }

    return xTaskCreate(
        demoTask,
        "demoTask",
        DEMO_TASK_STACK_SIZE,
        NULL,
        DEMO_TASK_PRIORITY,
        &demoTaskHandle
    ) == pdPASS;
}

bool demoModeStart(void)
{
    if (demoTaskHandle == NULL || demoRunning || !motorIsControlEnabled() ||
        limitSwitchAnyIsPressed()) {
        return false;
    }

    demoRunning = true;
    xTaskNotifyGive(demoTaskHandle);
    return true;
}

void demoModeStop(void)
{
    demoRunning = false;
    servoPenUp();

    if (motorIsControlEnabled() && !limitSwitchAnyIsPressed()) {
        motorHold();
    }
}

bool demoModeIsRunning(void)
{
    return demoRunning;
}

static void demoTask(void *arg)
{
    (void)arg;

    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        printf("Demo mode started\n");
        while (demoRunning) {
            for (size_t i = 0; i < sizeof(demoSteps) / sizeof(demoSteps[0]); i++) {
                if (!demoRunning) {
                    break;
                }

                if (!executeDemoStep(&demoSteps[i])) {
                    if (demoRunning) {
                        printf("Demo mode stopped: a step did not complete\n");
                        finishAfterFailure();
                    }
                    break;
                }
            }
        }

        servoPenUp();
        printf("Demo mode stopped\n");
    }
}

static bool executeDemoStep(const DemoStep *step)
{
    if (!demoRunning || !motorIsControlEnabled() || limitSwitchAnyIsPressed()) {
        return false;
    }

    switch (step->type) {
        case DEMO_STEP_MOVE:
            return setScaraAngles(step->theta1Degrees, step->theta2Degrees);

        case DEMO_STEP_PEN_UP:
            servoPenUp();
            return true;

        case DEMO_STEP_PEN_DOWN:
            servoPenDown();
            return true;

        case DEMO_STEP_WAIT:
            return waitWhileRunning(step->waitMs);
    }

    return false;
}

static bool waitWhileRunning(uint32_t waitMs)
{
    uint32_t elapsedMs = 0;

    while (demoRunning && motorIsControlEnabled() &&
           !limitSwitchAnyIsPressed() && elapsedMs < waitMs) {
        uint32_t remainingMs = waitMs - elapsedMs;
        uint32_t delayMs = remainingMs < DEMO_WAIT_SLICE_MS
            ? remainingMs
            : DEMO_WAIT_SLICE_MS;

        vTaskDelay(pdMS_TO_TICKS(delayMs));
        elapsedMs += delayMs;
    }

    return demoRunning && motorIsControlEnabled() && !limitSwitchAnyIsPressed();
}

static void finishAfterFailure(void)
{
    demoRunning = false;
    servoPenUp();

    if (motorIsControlEnabled() && !limitSwitchAnyIsPressed()) {
        motorHold();
    }
}
