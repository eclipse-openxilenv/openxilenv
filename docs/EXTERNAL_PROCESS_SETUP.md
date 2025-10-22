## Setup an External Process

An external process must implement the following four functions:

```c
void reference_varis(void);
int init_test_object(void);
void cyclic_test_object(void);
void terminate_test_object(void);
```

These functions define initialization, cyclic behavior, and termination of your test object.

Example: `Samples/ExternalProcesses/ExtProc_Simple.c`

```c
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define XILENV_INTERFACE_TYPE XILENV_DLL_INTERFACE_TYPE
#include "XilEnvExtProc.h"
#include "XilEnvExtProcMain.c"

short Ramp;
double Sinus;
short Random;
double SampleTime;

volatile const short BOTTOM_LIMIT_RAMP = 0;
volatile const short UPPER_LIMIT_RAMP = 1000;
volatile const double SINUS_AMPLITUDE  = 1.0;
volatile const double SINUS_FREQUENCY = 0.2;

void reference_varis(void)
{
    REFERENCE_WORD_VAR(Ramp, "Ramp");
    REFERENCE_DOUBLE_VAR(Sinus, "Sinus");
    REFERENCE_WORD_VAR(Random, "Random");
    REFERENCE_DOUBLE_VAR(SampleTime, "XilEnv.SampleTime");
}

int init_test_object(void)
{
    return 0;
}

void cyclic_test_object(void)
{
    static double SinusTime;
    if (Ramp++ > UPPER_LIMIT_RAMP) Ramp = BOTTOM_LIMIT_RAMP;
    SinusTime += 2.0 * M_PI * SampleTime * SINUS_FREQUENCY;
    Sinus = SINUS_AMPLITUDE * sin(SinusTime);
    Random = rand();
}

void terminate_test_object(void) {}
```

Build example:
```bash
# Windows
gcc -g -I "<openxilenv-install-dir>\include" ExtProc_Simple.c -o ExtProc_Simple.exe

# Linux
gcc -g -I <openxilenv-install-dir>/include ExtProc_Simple.c -ldl -lpthread -lm -o ExtProc_Simple
```

Run example:
```bash
# Windows
set PATH=%PATH%;<openxilenv-install-dir>
ExtProc_Simple.exe -q2 XilEnvGui.exe -ini <openxilenv-install-dir>\Samples\Configurations\ElectricCarSample.ini

# Linux
export PATH=$PATH:<openxilenv-install-dir>
./ExtProc_Simple -q2 XilEnvGui -ini <openxilenv-install-dir>/Samples/Configurations/ElectricCarSample.ini
```
Ensure both ExtProc_Simple and XilEnvGui are available in the same folder or included in your PATH.