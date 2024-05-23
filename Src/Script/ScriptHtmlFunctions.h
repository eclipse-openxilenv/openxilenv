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


#ifndef _SC_SCRIPT_HTMLFUNCTIONS_H
#define _SC_SCRIPT_HTMLFUNCTIONS_H

int get_html_strings(char *st1,char *st2,char *keywordstring);

void init_html_strings(void);

FILE *GenerateHTMLReportFile (char *HTML_ReportFilename);

int CloseHTMLReportFile (FILE *HTML_ReportFile);


#endif
