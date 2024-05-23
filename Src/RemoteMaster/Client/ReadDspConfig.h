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


#pragma once

int GetMaxNumberOfDsps(void);

int GetNumberOfDaDsps (void);

int GetNumberOfAdDsps (void);

void DSP56KLoadAllConfigs (void);

void DSP56KConfigAck (int Flag);

// wird von der GUI-Message-Loop aus aufgerufen wenn dort eine
// DSP56301_INI-Message empfangen wird.
void DSP56KLoadAllConfigsAfterStartDspProcess (void);


int SaveAnalogSignalConfiguration (char *Filename);
int LoadAnalogSignalConfiguration (char *Filename);