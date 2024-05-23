/*
 * Copyright 2023 ZF Friedrichshafen AG
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include "Platform.h"

extern "C" {
#include "MyMemory.h"
#include "IniDataBase.h"
#include "MainValues.h"
#include "InterfaceToScript.h"
}

#include "Parser.h"
#include "BaseCmd.h"

DEFINE_CMD_CLASS(cImportControlLampStencils)


int cImportControlLampStencils::SyntaxCheck (cParser *par_Parser)
{
    UNUSED(par_Parser);
    return 0;
}

int cImportControlLampStencils::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    UNUSED(par_Executor);
    int Ret = -1;
    int Fd = IniFileDataBaseOpen(par_Parser->GetParameter(0));

    if (Fd <= 0) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "IMPORT_CONTROL_LAMP_STENCIL \"%s\" cannot open file \"%s\") ", par_Parser->GetParameter(0));
    } else {
        IniFileDataBaseWriteString("GUI/AllUserDefinedStancilForControlLamps", nullptr, nullptr, GetMainFileDescriptor());
        IniFileDataBaseCopySection(GetMainFileDescriptor(), Fd, "GUI/AllUserDefinedStancilForControlLamps", "GUI/AllUserDefinedStancilForControlLamps");
        Ret = 0;  // OK
        IniFileDataBaseClose(Fd);
    }
    return Ret;
}

int cImportControlLampStencils::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
}

static cImportControlLampStencils ImportControlLampStencils ("IMPORT_CONTROL_LAMP_STENCIL",
                             1,
                             1,
                             nullptr,
                             FALSE,
                             FALSE,
                             0,
                             0,
                             CMD_INSIDE_ATOMIC_ALLOWED);


DEFINE_CMD_CLASS(cExportControlLampStencils)


int cExportControlLampStencils::SyntaxCheck (cParser *par_Parser)
{
    UNUSED(par_Parser);
    return 0;
}

int cExportControlLampStencils::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    UNUSED(par_Executor);
    char Filename[MAX_PATH];
    strncpy (Filename, par_Parser->GetParameter (0), sizeof (Filename));
    Filename[sizeof(Filename)-1] = 0;
    int Ret = -1;
    int Fd;

    Fd = IniFileDataBaseOpen(par_Parser->GetParameter(0));
    if (Fd <= 0) {
        Fd = IniFileDataBaseCreateAndOpenNewIniFile(par_Parser->GetParameter(0));
    }
    if (Fd <= 0) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "IMPORT_CONTROL_LAMP_STENCIL \"%s\" cannot open file \"%s\") ", par_Parser->GetParameter(0));
    } else {
        IniFileDataBaseWriteString("GUI/AllUserDefinedStancilForControlLamps", nullptr, nullptr, Fd);
        IniFileDataBaseCopySection(Fd, GetMainFileDescriptor(), "GUI/AllUserDefinedStancilForControlLamps", "GUI/AllUserDefinedStancilForControlLamps");
        if (IniFileDataBaseSave(Fd, nullptr, 2)) {
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "IMPORT_CONTROL_LAMP_STENCIL \"%s\" cannot write file \"%s\") ", par_Parser->GetParameter(0));
        } else {
            Ret = 0;  // alles OK
        }
    }
    return Ret;
}

int cExportControlLampStencils::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
}

static cExportControlLampStencils ExportControlLampStencils ("EXPORT_CONTROL_LAMP_STENCIL",
                             1,
                             1,
                             nullptr,
                             FALSE,
                             FALSE,
                             0,
                             0,
                             CMD_INSIDE_ATOMIC_ALLOWED);

