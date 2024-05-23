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


#ifndef IMEXPORTDSKFILE_H
#define IMEXPORTDSKFILE_H

// Close all windows and delete all window information from the INI file
int clear_desktop_ini (void);

// Imports a desktop file (*.dsk) into the loaded INI file
int load_desktop_file_ini (const char *fname);

// Exports all window configurations into a desktop file (*.dsk)
int save_desktop_file_ini (const char *fname);


// Imports a desktop file (*.dsk) into the loaded INI file from script or rpc
int script_command_load_desktop_file (char *fname);
	
// Exports all window configurations into a desktop file (*.dsk) from script or rpc
int script_command_save_desktop_file (char *fname);

// Close all windows and delete all window information from the INI file from script or rpc
int script_command_clear_desktop (void);

#endif
