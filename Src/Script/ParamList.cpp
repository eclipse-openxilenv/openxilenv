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
#include <stdio.h>
#include <malloc.h>

extern "C" {
#include "MyMemory.h"
#include "StringMaxChar.h"
}

#include "Parser.h"
#include "Tokenizer.h"
#include "ParamList.h"

#define UNUSED(x) (void)(x)

int cParamList::ScriptSearchParamList (char *Name)
{
    int x;

    for (x = 0; x < ScriptParamListCount; x++) {
        if (ScriptParamList[x].Name != nullptr) {
            if (!strcmp (ScriptParamList[x].Name, Name)) {
                return x;
            }
        }
    }
    return -1;  // not found
}

void cParamList::ScriptDeleteAllParamLists (void)
{
    int x;

    for (x = 0; x < ScriptParamListCount; x++) {
        if (ScriptParamList[x].Name != nullptr) {
            my_free (ScriptParamList[x].Name);
            ScriptParamList[x].Name = nullptr;
        }
        if (ScriptParamList[x].Params != nullptr) {
            my_free (ScriptParamList[x].Params);
            ScriptParamList[x].Params = nullptr;
        }
    }
    ScriptParamListCount = 0;
}

void cParamList::ScriptPrintAllParamLists (char *Txt, int Maxc)
{
    UNUSED(Maxc);
    int x;

    Txt[0] = 0;
    for (x = 0; x < ScriptParamListCount; x++) {
        StringAppendMaxCharTruncate (Txt, "\"", Maxc);
        StringAppendMaxCharTruncate (Txt, ScriptParamList[x].Name, Maxc);
        StringAppendMaxCharTruncate (Txt, "\"", Maxc);
        if (x < (ScriptParamListCount-1)) StringAppendMaxCharTruncate (Txt, ",", Maxc);
    }
}


int cParamList::GetParamListString (char **Buffer, int *SizeOfBuffer,
                                    char **RefBuffer, int *SizeOfRefBuffer)
{
    int x;
    int Size;
    char *p, *r;
    int Ret = 0;

    // First calculate needed buffer size
    Size = 1;   // terminating 0-char
    for (x = 0; x < ScriptParamListCount; x++) {
        Size += ScriptParamList[x].NameLen + ScriptParamList[x].ParamsLen + 2;
    }
    if (*SizeOfBuffer < Size) {
        *SizeOfBuffer = Size;
        *Buffer = static_cast<char*>(my_realloc (*Buffer, static_cast<size_t>(Size)));
        if (*Buffer == nullptr) {
            ThrowError (1, "out of memory");
            return 0;
        }
    }
    p = *Buffer;
    r = *RefBuffer;
    for (x = 0; x < ScriptParamListCount; x++) {
        char *ps = p;
        MEMCPY (p, ScriptParamList[x].Name, static_cast<size_t>(ScriptParamList[x].NameLen));
        p += ScriptParamList[x].NameLen - 1;
        MEMCPY (p, " = ", 3);
        p += 3;
        MEMCPY (p, ScriptParamList[x].Params, static_cast<size_t>(ScriptParamList[x].ParamsLen));
        p += ScriptParamList[x].ParamsLen - 1;
        *p = 0;
        p++;
        if (Ret == 0) {
            if (r == nullptr) {
                Ret = 1;
            } else if (strcmp (r, ps)) {
                Ret = 1;
            } else {
                r += p - ps;
            }
        }
    }
    *p = 0;
    if (Ret) {
        if (*SizeOfRefBuffer < *SizeOfBuffer) {
            *SizeOfRefBuffer = *SizeOfBuffer;
            *RefBuffer = static_cast<char*>(my_realloc (*RefBuffer, static_cast<size_t>(*SizeOfRefBuffer)));
            if (*RefBuffer == nullptr) {
                ThrowError (1, "out of memory");
                return 0;
            }
        }
        MEMCPY (*RefBuffer, *Buffer, static_cast<size_t>(Size));
    }
    return Ret;
}



int cParamList::AddParamsToParamList (cParser *par_Parser)
{
    int Idx;
    int Len;
    int x;
    char *Help, *Help2;
    int First;

    if ((Idx = ScriptSearchParamList (par_Parser->GetParameter (0))) >= 0) {  // The list exist
        if (ScriptParamList[Idx].Params == nullptr) {
            Len = 0;
            First = 1;
        } else {
            Len = static_cast<int>(strlen (ScriptParamList[Idx].Params) + 3);
            First = 0;
        }
        for (x = 1; x < par_Parser->GetParameterCounter (); x++) {
            Len += static_cast<int>(strlen (par_Parser->GetParameter (x))) + 1;  // +1 for ","
        }
        Help = static_cast<char*>(my_malloc (static_cast<size_t>(Len)));
        if (Help == nullptr) return 0;
        if (First) {
            Help[0] = 0;
        } else {
            StringCopyMaxCharTruncate (Help, ScriptParamList[Idx].Params, Len);
            StringAppendMaxCharTruncate (Help, ",", Len);
        }
        for (x = 1; x < par_Parser->GetParameterCounter (); x++) {
            StringAppendMaxCharTruncate (Help, par_Parser->GetParameter (x), Len);
            if (x < (par_Parser->GetParameterCounter ()-1)) StringAppendMaxCharTruncate (Help, ",", Len);
        }
        Help2 = ScriptParamList[Idx].Params;
        ScriptParamList[Idx].Params = Help;
        ScriptParamList[Idx].ParamsLen = static_cast<int>(strlen (Help) + 1);
        my_free (Help2);
        return 0;
    }
    return -1;
}


int cParamList::DelParamsFromParamList (cParser *par_Parser)
{
    int Idx;
    int x;
    char *Help, *Help2, *p, *ps, *pn, *s, *d;
    int Found;

    if ((Idx = ScriptSearchParamList (par_Parser->GetParameter (0))) >= 0) {  // The list exist
        if (ScriptParamList[Idx].Params == nullptr) return 0;
        Help = static_cast<char*>(my_malloc (strlen (ScriptParamList[Idx].Params)));
        if (Help == nullptr) return 0;
        d = Help;
        p = s = ScriptParamList[Idx].Params;

        while (*p != 0) {
            if (*p == 0) break;
            ps = p;
            while ((*p != ',') && (*p != 0)) p++;
            pn = p;
            Found = 0;
            for (x = 1; x < par_Parser->GetParameterCounter (); x++) {
                int Len1 = static_cast<int>(p - ps);
                int Len2 = static_cast<int>(strlen (par_Parser->GetParameter (x)));
                if ((Len1 == Len2) &&
                    !strncmp (ps, par_Parser->GetParameter (x), static_cast<size_t>(Len1))) {
                    Found = 1;
                    break;
                }
            }
            // If not found insert to new parameter list
            if (!Found) {
                if (d != Help) {
                    *d = ',';
                    d++;
                }
                MEMCPY (d, ps, static_cast<size_t>(p - ps));
                d += p - ps;
            }
            p = pn;
            if (*p == ',') p++;
        }
        *d = 0;
        Help2 = ScriptParamList[Idx].Params;
        ScriptParamList[Idx].Params = Help;
        ScriptParamList[Idx].ParamsLen = static_cast<int>(strlen (Help) + 1);
        my_free (Help2);
        return 0;
    }
    return -1;
}


int cParamList::AddNewParamList (cParser *par_Parser)
{
    int x, Len;

    if (ScriptSearchParamList (par_Parser->GetParameter (0)) >= 0) return -1;  // The list exist
    if (ScriptParamListCount == ScriptParamListSize) {
        ScriptParamListSize += 64;
        ScriptParamList = static_cast<SCRIPT_PARAM_LIST*>(my_realloc (ScriptParamList, sizeof (SCRIPT_PARAM_LIST) * static_cast<size_t>(ScriptParamListSize)));
        if (ScriptParamList == nullptr) {
            return -1;
        }
    }
    ScriptParamList[ScriptParamListCount].Name = StringMalloc (par_Parser->GetParameter (0));

    Len = 0;
    for (x = 1; x < par_Parser->GetParameterCounter (); x++) {
        Len += static_cast<int>(strlen (par_Parser->GetParameter (x))) + 1;  // String + comma
    }
    ScriptParamList[ScriptParamListCount].Params = static_cast<char*>(my_malloc (static_cast<size_t>(Len)));
    ScriptParamList[ScriptParamListCount].Params[0] = 0;
    for (x = 1; x < par_Parser->GetParameterCounter (); x++) {
        StringAppendMaxCharTruncate (ScriptParamList[ScriptParamListCount].Params, par_Parser->GetParameter (x), ScriptParamList[ScriptParamListCount].ParamsLen);
        if (x < (par_Parser->GetParameterCounter ()-1)) StringAppendMaxCharTruncate (ScriptParamList[ScriptParamListCount].Params, ",", ScriptParamList[ScriptParamListCount].ParamsLen);
    }
    ScriptParamList[ScriptParamListCount].NameLen = static_cast<int>(strlen (ScriptParamList[ScriptParamListCount].Name) + 1);
    ScriptParamList[ScriptParamListCount].ParamsLen = static_cast<int>(strlen (ScriptParamList[ScriptParamListCount].Params) + 1);

    ScriptParamListCount++;

    return 0;
}
        

int cParamList::ScriptDeleteParamList (char *Name)
{
    int x, Pos;

    if ((Pos = ScriptSearchParamList (Name)) >= 0) {
        my_free (ScriptParamList[Pos].Name);
        if (ScriptParamList[Pos].Params != nullptr) my_free (ScriptParamList[Pos].Params);
        for (x = Pos; x < (ScriptParamListCount-1); x++) {
            ScriptParamList[x] = ScriptParamList[x+1];
        }
        ScriptParamList[x].Name = nullptr;
        ScriptParamList[x].Params = nullptr;
        ScriptParamListCount--;
        return 0;
    } else {
        return -1;
    }
}

int cParamList::ScriptInsertParameterList (char *par_ParamListName, cTokenizer *par_Tokenizer, cParser *par_Parser)
{
    int x;
    int char_count;
    int param_count;
    char *s;

    for (x = 0; x < ScriptParamListCount; x++) {
        if (ScriptParamList[x].Name != nullptr) {
            if (!strcmp (ScriptParamList[x].Name, par_ParamListName)) {  // and same content
                s = ScriptParamList[x].Params;
                for (param_count = 0; ; param_count++) {
                    if (par_Tokenizer->StartParameterInBuffer (par_Parser)) return -1;
                    char_count = 0;
                    while (1) {
                        if (*s == ',') {
                            if (par_Tokenizer->InsertOneCharToParameterBuffer (par_Parser, 0)) return -1;
                            s++;
                            break;
                        } else if (*s == 0) {
                            if (par_Tokenizer->InsertOneCharToParameterBuffer (par_Parser, 0)) return -1;
                            goto LAST_PARAM;
                        } else {
                            if (par_Tokenizer->InsertOneCharToParameterBuffer (par_Parser, *s)) return -1;
                            s++;
                            char_count++;
                        }
                    }
                }
LAST_PARAM:
                param_count++;
                return param_count;
            }
        }
    }
    return -1;  // not found   
}
