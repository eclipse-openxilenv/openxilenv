#include "Platform.h"
#include <stdio.h>

extern "C" {
#include "MyMemory.h"
}
#include "Parser.h"
#include "BaseCmd.h"

DEFINE_CMD_CLASS(cDetachAllBbvaris)


int cDetachAllBbvaris::SyntaxCheck (cParser *par_Parser)
{
    UNUSED(par_Parser);
    return 0;
}

int cDetachAllBbvaris::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    par_Executor->DetachAllBbvaris();
    return 0;
}

int cDetachAllBbvaris::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
}

static cDetachAllBbvaris DetachAllBbvaris ("DETACH_ALL_BBVARIS",
                                           0,
                                           0,
                                           nullptr,
                                           FALSE,
                                           FALSE,
                                           0,
                                           0,
                                           CMD_INSIDE_ATOMIC_ALLOWED);
