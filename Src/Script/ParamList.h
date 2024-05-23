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


#ifndef PARAMLIST_H
#define PARAMLIST_H

class cParser;

class cParamList {
private:

    typedef struct {
        char *Name;
        char *Params;
        int NameLen;
        int ParamsLen;
    } SCRIPT_PARAM_LIST;

    SCRIPT_PARAM_LIST *ScriptParamList;
    int ScriptParamListCount;
    int ScriptParamListSize;

    int ScriptSearchParamList (char *Name);

public:

    cParamList (void)
    {
        ScriptParamList = NULL;
        ScriptParamListCount = 0;
        ScriptParamListSize = 0;
    }
    ~cParamList (void)
    {
        if (ScriptParamList != NULL) my_free (ScriptParamList);
    }

    int AddParamsToParamList (cParser *par_Parser);

    int DelParamsFromParamList (cParser *par_Parser);

    int AddNewParamList (cParser *par_Parser);
            
    int ScriptDeleteParamList (char *Name);

    int ScriptInsertParameterList (char *par_ParamListName, cTokenizer *par_Tokenizer, cParser *par_Parser);

    void ScriptDeleteAllParamLists (void);

    void ScriptPrintAllParamLists (char *Txt, int Maxc);
    
    void Reset (void)
    {
        ScriptDeleteAllParamLists ();
    }

    int GetParamListString (char **Buffer, int *SizeOfBuffer, char **RefBuffer, int *SizeOfRefBuffer);

};

#endif
