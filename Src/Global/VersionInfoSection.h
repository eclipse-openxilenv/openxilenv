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


#ifndef VERSIONINFOSECTION_H 
#define VERSIONINFOSECTION_H

struct OwnVersionInfosStruct {
    char SignString[64];
    int Version;
    int MinorVersion;
    int RemoteMasterLibraryAPIVersion;
    int Fill2;
    char BuildDateString[32];
    char BuildTimeString[32];
    char VersionSignString[128];
    char Fill3[1024];
};

#define OWN_VERSION_SIGN_STRING "OwnVersionSignKeywordString"

const struct OwnVersionInfosStruct *GetOwnVersionInfos(void);

#endif
