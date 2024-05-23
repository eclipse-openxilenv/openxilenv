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


#ifndef BASECMD_H
#define BASECMD_H

class cParser;

#include "Executor.h"

#define UNUSED(x) (void)(x)

class cBaseCmd {
private:
    const char *CommandString;
    int MinParams;
    int MaxParams;
    const char *TempFileName;  // zB: "generator.gen", "recorder.cfg", "player.cfg" oder "equations.tri" oder NULL falls keines
    int RemoteMasterOnly;
    int WaitFlagNeeded;
    int WaitFlagTimeout;   // falls -1 unendlich sonst in ms
    int ParseDuringUsing;  // 0 -> Befehl wird beim Parsen im USING-Mode ignoriert
    int InsideAtomicAllowedFlag;  
#define CMD_INSIDE_ATOMIC_ALLOWED                3
#define CMD_INSIDE_ATOMIC_NOT_ALLOWED            0
#define CMD_INSIDE_ATOMIC_ALLOWED_ONLY_WITHOUT_REMOTE_MASTER   1
#define CMD_INSIDE_ATOMIC_ALLOWED_ONLY_WITH_REMOTE_MASTER      2
//#define ENVIRONMENTVARIABLES_INSIDE_PARAMETERS_NOT_ALLOWED  4

    void AddToTokenArray (void);

public:
    virtual ~cBaseCmd()
    {
    }

    const char *GetName (void)
    {
        return CommandString;
    }

    const char *GetTempFilename (void)
    {
        return TempFileName;
    }

    int GetWaitFlagTimeout (void)
    {
        return WaitFlagTimeout;
    }

    int GetWaitFlagNeeded (void)
    {
        return WaitFlagNeeded;
    }

    int SyntaxCheckInUsingMode (void)
    {
        return ParseDuringUsing;
    }

    int GetMinParams (void)
    { 
        return MinParams;
    }

    int GetMaxParams (void)
    { 
        return MaxParams;
    }

    void SetTimeout (int par_WaitFlagNeeded, int par_WaitFlagTimeout)
    {
        WaitFlagNeeded = par_WaitFlagNeeded;
        WaitFlagTimeout = par_WaitFlagTimeout;
    }

    int IsInsideAtomicAllowed (void)
    {
        return ((InsideAtomicAllowedFlag & CMD_INSIDE_ATOMIC_ALLOWED_ONLY_WITHOUT_REMOTE_MASTER) == CMD_INSIDE_ATOMIC_ALLOWED_ONLY_WITHOUT_REMOTE_MASTER);
    }

    cBaseCmd (const char * const par_CommandString,
              int par_MinParams,
              int par_MaxParams,
              const char * const par_TempFileName,  // zB: "generator.gen", "recorder.cfg", "player.cfg" oder "equations.tri"
              int par_RemoteMasterOnly,
              int par_WaitFlagNeeded,
              int par_WaitFlagTimeout,
              int par_ParseDuringUsing,
              int par_InsideAtomicAllowedFlag)
    {
        CommandString = par_CommandString;
        MinParams = par_MinParams;
        MaxParams = par_MaxParams;
        TempFileName = par_TempFileName;  // zB: "generator.gen", "recorder.cfg", "player.cfg" oder "equations.tri"
        RemoteMasterOnly = par_RemoteMasterOnly;
        WaitFlagNeeded = par_WaitFlagNeeded;
        WaitFlagTimeout = par_WaitFlagTimeout;
        ParseDuringUsing = par_ParseDuringUsing;
        InsideAtomicAllowedFlag = par_InsideAtomicAllowedFlag;
        AddToTokenArray ();
    }

    virtual int SyntaxCheck (cParser *par_Parser) = 0;
    virtual int Execute (cParser *par_Parser, cExecutor *par_Executor) = 0;
    virtual int Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles) = 0;
};


#define BAS_CMD_CONSTRUCTOR(c) c (const char *const par_CommandString, int par_MinParams, int par_MaxParams, const char * const par_TempFileName, int par_RemoteMasterOnly, int par_WaitFlagNeeded, int par_WaitFlagTimeout, int par_ParseDuringUsing, int par_InsideAtomicAllowedFlag) : cBaseCmd (par_CommandString, par_MinParams, par_MaxParams, par_TempFileName,  par_RemoteMasterOnly, par_WaitFlagNeeded, par_WaitFlagTimeout, par_ParseDuringUsing, par_InsideAtomicAllowedFlag) {}

#define DEFINE_CMD_CLASS(name) \
class name : public cBaseCmd { \
private: \
public: \
    BAS_CMD_CONSTRUCTOR(name) \
    int SyntaxCheck (cParser *par_Parser); \
    int Execute (cParser *par_Parser, cExecutor *par_Executor); \
    int Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles); \
};

#endif
