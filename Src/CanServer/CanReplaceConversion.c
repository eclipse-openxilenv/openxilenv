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

#include <string.h>

#include "MyMemory.h"
#include "ThrowError.h"
#include "Message.h"
#include "ExecutionStack.h"
#include "ReadCanCfg.h"

#include "CanReplaceConversion.h"


typedef struct {
   struct EXEC_STACK_ELEM *ExecStack;
   int ConvTypeSave;
   int ConvIdxSave;
} REPLACE_CONV_ELEM;

static REPLACE_CONV_ELEM *ReplaceConvList;
static int ReplaceConvListSize;

void CleanAllReplaceConvs (void)
{
    int i;
    for (i = 0; i < ReplaceConvListSize; i++) {
        if (ReplaceConvList[i].ExecStack != NULL) {
            remove_exec_stack (ReplaceConvList[i].ExecStack);
            ReplaceConvList[i].ExecStack  = NULL;
        }
    }
}

static int RemoveReplaceConvFromSig (NEW_CAN_SERVER_SIGNAL *ps)
{
    int Idx;

    Idx = ps->ConvIdx;
    if (Idx >= ReplaceConvListSize) {
        ThrowError (1, "Internal error in RemoveReplaceConvFromSig() %s (%i)", __FILE__, __LINE__);
        return -1;
    }
    if (ReplaceConvList[Idx].ExecStack != NULL) {
        remove_exec_stack (ReplaceConvList[Idx].ExecStack);
        ReplaceConvList[Idx].ExecStack = NULL;
    }
    ps->ConvIdx = ReplaceConvList[Idx].ConvIdxSave;
    ps->ConvType = ReplaceConvList[Idx].ConvTypeSave;

    return 0;
}

// wenn par_ExecStack == NULL setze die Umrechnung wieder zurueck
// wenn par_ExecStack == NULL und par_SignalName == NULL setze alle Umrechnungen wieder zurueck

static int AddOrRemoveReplaceCanSigConv (NEW_CAN_SERVER_CONFIG *par_CanConfig, 
                                         struct EXEC_STACK_ELEM *par_ExecStack, 
                                         int par_Channel, int par_Id, char *par_SignalName)
{
    int x, c;
    int ChannelFrom, ChannelTo;
    int ReplaceCounter = 0;

    if (par_CanConfig == NULL) return -1;
    if (par_Channel < 0) {
        ChannelFrom = 0;
        ChannelTo = par_CanConfig->channel_count;
    } else {
        if (par_Channel >= par_CanConfig->channel_count) return -1;
        ChannelFrom = par_Channel;
        ChannelTo = ChannelFrom + 1;
    }
    for (c = ChannelFrom; c < ChannelTo; c++) {
        int o, o_pos;
        NEW_CAN_SERVER_OBJECT *po;
        for (o = 0; o < par_CanConfig->channels[c].tx_object_count; o++) {          
            o_pos = par_CanConfig->channels[c].hash_tx[o].pos;
            po = &par_CanConfig->objects[o_pos];
            if ((par_Id < 0) || ((int)po->id == par_Id)) {
                int s, s_pos;
                NEW_CAN_SERVER_SIGNAL *ps;
                for (s = 0; s < po->signal_count; s++) {
                    s_pos = po->signals_ptr[s];
                    ps = &(par_CanConfig->bb_signals[s_pos]);
                    if ((par_SignalName == NULL) ||
                        !strcmp (par_SignalName, GET_CAN_SIG_NAME (par_CanConfig, s_pos))) {
                        if (par_ExecStack != NULL) {
                            // Umrechnung aendern
                            if (ps->ConvType == CAN_CONV_REPLACED) {
                                RemoveReplaceConvFromSig (ps);
                            }
                            __REPEAT:
                            for (x = 0; x < ReplaceConvListSize; x++) {
                                if (ReplaceConvList[x].ExecStack == NULL) { // leeres Element?
                                    ReplaceConvList[x].ConvIdxSave = ps->ConvIdx;
                                    ReplaceConvList[x].ConvTypeSave = ps->ConvType;

                                    ReplaceConvList[x].ExecStack = 
                                        (struct EXEC_STACK_ELEM*)my_malloc (sizeof_exec_stack (par_ExecStack));
                                    if (ReplaceConvList[x].ExecStack == NULL) {
                                        ThrowError (1, "out of memory");
                                        return -1;
                                    }

                                    copy_exec_stack (ReplaceConvList[x].ExecStack,
                                                     par_ExecStack);
                                    attach_exec_stack (ReplaceConvList[x].ExecStack);
                                    //copy_exec_stack_with_bbvaris (ReplaceConvList[ReplaceConvListPos].ExecStack,
                                    //                              par_ExecStack);

                                    // Index der neuen Umrechnung setzen
                                    ps->ConvIdx = x;
                                    ps->ConvType = CAN_CONV_REPLACED;
                                    ReplaceCounter++;
                                    break;
                                }
                            }
                            if (x == ReplaceConvListSize) {
                                // Groese anpassen falls noetig
                                int OldSize = ReplaceConvListSize;
                                ReplaceConvListSize += (ReplaceConvListSize >> 2) + 8;
                                ReplaceConvList = (REPLACE_CONV_ELEM*)my_realloc (ReplaceConvList, (size_t)ReplaceConvListSize* sizeof (REPLACE_CONV_ELEM));
                                if (ReplaceConvList == NULL) {
                                    ReplaceConvListSize = 0;
                                    return -1;
                                }
                                for (x = OldSize; x < ReplaceConvListSize; x++) {
                                    ReplaceConvList[x].ExecStack = NULL;
                                }
                                goto __REPEAT;
                            }
                        } else {
                            // Umrechnung wieder loeschen
                            if (ps->ConvType == CAN_CONV_REPLACED) {
                                RemoveReplaceConvFromSig (ps);
                            }
                        }
                    }
                }
            }
        }
    }
    if ((ReplaceCounter == 0) && (par_ExecStack != NULL) && (par_SignalName != NULL)) {
        return -1;
    }
    return 0;                     
}


int AddOrRemoveReplaceCanSigConvMsg (NEW_CAN_SERVER_CONFIG *par_CanConfig,
                                     MESSAGE_HEAD *mhead)
{
    static CAN_SET_SIG_CONV *cssc;
    static int SizeOfCssc;

    struct EXEC_STACK_ELEM *ExecStack;
    char *SignalName;
    int Ret;

    if ((cssc == NULL) || (mhead->size > SizeOfCssc)) {
        SizeOfCssc = mhead->size;
        cssc = (CAN_SET_SIG_CONV*)my_realloc (cssc, SizeOfCssc);
        if (cssc == NULL) {
            Ret = -1;
            write_message (mhead->pid, NEW_CANSERVER_SET_SIG_CONV, sizeof (int), (char*)&Ret);
            return -1;
        }
    }
    read_message (mhead, (char*)cssc, SizeOfCssc);
    if (cssc->IdxOfSignalName == -1) {
        SignalName = NULL;
    } else {
        SignalName = &cssc->Data[cssc->IdxOfSignalName];
    }
    if (cssc->IdxOfExecStack == -1) {
        ExecStack = NULL;
    } else {
        ExecStack = (struct EXEC_STACK_ELEM *)&cssc->Data[cssc->IdxOfExecStack];
    }

    Ret = AddOrRemoveReplaceCanSigConv (par_CanConfig, 
                                        ExecStack, 
                                        cssc->Channel, cssc->Id, SignalName);
    write_message (mhead->pid, NEW_CANSERVER_SET_SIG_CONV, sizeof (int), (char*)&Ret);
    return 0;
}


int CalcReplaceCanSigConv (int par_Idx, union FloatOrInt64 par_Parameter, int par_ParameterType, NEW_CAN_SERVER_OBJECT *par_CanObject, union FloatOrInt64 *ret_Value)
{
    if ((par_Idx >= ReplaceConvListSize) || (ReplaceConvList[par_Idx].ExecStack == NULL)) {
        ThrowError (1, "Internal error in CalcReplaceCanSigConv() %s (%i)", __FILE__, __LINE__);
        return 0;
    }
    return execute_stack_whith_can_parameter (ReplaceConvList[par_Idx].ExecStack, par_Parameter, par_ParameterType, par_CanObject,ret_Value);
}
