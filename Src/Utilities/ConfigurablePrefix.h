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


#ifndef CONFIGURABLEPREFIX_H
#define CONFIGURABLEPREFIX_H

enum ConfigurablePrefixType {CONFIGURABLE_PREFIX_TYPE_PROGRAM_NAME,
                             CONFIGURABLE_PREFIX_TYPE_SHORT_BLACKBOARD,
                             CONFIGURABLE_PREFIX_TYPE_LONG_BLACKBOARD,
                             CONFIGURABLE_PREFIX_TYPE_LONG2_BLACKBOARD,
                             CONFIGURABLE_PREFIX_TYPE_ERROR_FILE,
                             CONFIGURABLE_PREFIX_TYPE_CAN_NAMES,
                             CONFIGURABLE_PREFIX_TYPE_FLEXRAY_NAMES,

                             CONFIGURABLE_PREFIX_TYPE_CYCLE_COUNTER,
                             CONFIGURABLE_PREFIX_TYPE_SAMPLE_TIME,
                             CONFIGURABLE_PREFIX_TYPE_SAMPLE_FREQUENCY,
                             CONFIGURABLE_PREFIX_TYPE_OWN_EXIT_CODE,

                             CONFIGURABLE_PREFIX_TYPE_SCRIPT,
                             CONFIGURABLE_PREFIX_TYPE_GENERATOR,
                             CONFIGURABLE_PREFIX_TYPE_TRACE_RECORDER,
                             CONFIGURABLE_PREFIX_TYPE_STIMULUS_PLAYER,
                             CONFIGURABLE_PREFIX_TYPE_EQUATION_CALCULATOR};

char *ExtendsWithConfigurablePrefix(enum ConfigurablePrefixType par_PrefixType, const char *par_Src,
                                    char *par_Buffer, int par_BufferSize);

const char *GetConfigurablePrefix(enum ConfigurablePrefixType par_PrefixType);

#ifdef REMOTE_MASTER
void RemoteMasterInitConfigurablePrefix(const char *par_BaseAddress, uint32_t *par_OffsetConfigurablePrefix);
#endif

#endif // CONFIGURABLEPREFIX_H
