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


#ifndef ISWINDOWNAMEVALID_H
#define ISWINDOWNAMEVALID_H

inline int IsWindowNameValid (char *Name)
{
    if (isspace(Name[0])) return 0;   // darf nicht mit einem Leerzeichen anfangen
    if (Name[0] == 0) return 0; // mind. 1 Zeichen

    return 1;
}

#endif // ISWINDOWNAMEVALID_H

