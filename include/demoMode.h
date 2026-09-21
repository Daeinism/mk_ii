#ifndef DEMO_MODE_H
#define DEMO_MODE_H

#include <stdbool.h>

bool demoModeInit(void);
bool demoModeStart(void);
void demoModeStop(void);
bool demoModeIsRunning(void);

#endif
