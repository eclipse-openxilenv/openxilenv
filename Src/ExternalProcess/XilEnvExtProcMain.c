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


#ifdef _WIN32
#ifdef _MSC_VER
    #pragma warning(push, 3)
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <Windows.h>

#ifdef _MSC_VER
    #pragma warning(pop)
#endif

#define PATH_SEPARTOR "\\"
#else
#include <dlfcn.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#define MAX_PATH 1024
#define PATH_SEPARTOR "/"
#define _stricmp strcasecmp
#define _strnicmp strncasecmp

static int GetEnvironmentVariableA(const char *var, char *value, int maxc)
{
    char *v = getenv(var);
    if (v == (char*)0) return 0;
    int len = strlen(v);
    if (len < maxc) {
        strncpy(value, v, maxc);
        return len;
    } else {
        strncpy(value, v, maxc - 1);
        value[maxc - 1] = 0;
        return maxc - 1;
    }
}

static char *GetCommandLineA(void)
{
    static char *ret;
    int CharCounter = 1;
    int CharPos = 0;
    int BufferSize = 100;
    if (ret == NULL) {
        ret = (char*)malloc(BufferSize);
        int fd = open("/proc/self/cmdline", O_RDONLY);
        if (fd > 0) {
            char c;
            while (read(fd, &c, 1) == 1) {
                if (c == 0) {
                    CharCounter++;
                    if (CharCounter > BufferSize) {
                        BufferSize += 100;
                    }
                    ret = (char*)realloc(ret, BufferSize);
                    ret[CharPos] = ' ';
                    CharPos++;
                } else {
                    CharCounter++;
                    if (CharCounter > BufferSize) {
                        BufferSize += 100;
                    }
                    ret = (char*)realloc(ret, BufferSize);
                    ret[CharPos] = c;
                    CharPos++;
                }
            }
            ret[CharPos] = 0;
        }
    }
    return ret;
}
#endif

#include "XilEnvExtProc.h"

#include "XilEnvExtProcInterfaceMethods.c"

/*
 *  The path where the XilEnv external process library will be searched:
 *  1. The environment variable XILENV_DLL_PATH, XilEnv will set this variable to his location for all child processes
 *  2. The parameter -dllpath <path>
 *  3. If the extern process will start XilEnv with the parameter -q or -w the path where XilEnv started from
 *  4. normal DLL search
 */

static XilEnvFunctionCallPointer XilEnvInternal_GetStubFuncPtr(const char *par_FunctionName)
{
    (void)par_FunctionName;
    return NULL;
}

static void XilEnvInternal_Strcat(char *par_Dst, int par_MaxLen, const char *par_Src)
{
    char *d = par_Dst;
    int MaxM1 = par_MaxLen - 1;
    while ((*d != 0) && ((d - par_Dst) < MaxM1)) d++;
    if ((*d == 0) && ((d - par_Dst) < MaxM1)) {
        const char *s = par_Src;
        while ((*s != 0) && ((d - par_Dst) < MaxM1)) {
            *d++ = *s++;
        }
    }
    *d = 0; // be sure it is always terminated
}

// to avoid compiler warning use own strnicmp function
static int XilEnvInternal_upper(const char par_Char)
{
    if ((par_Char >= 'a') && (par_Char <= 'z')) {
        return (int)((par_Char - 'a') + 'A');
    } else {
        return (int)par_Char;
    }
}

static int XilEnvInternal_strnicmp(const char *par_String1, const char *par_String2, int par_MaxChars)
{
    int c = 0;
    while ((par_String1[c] != 0) && (c < par_MaxChars)) {
        if (XilEnvInternal_upper(par_String1[c]) != XilEnvInternal_upper(par_String2[c])) {
            return 1;
        }
        c++;
    }
    return !((par_String2[c] == 0) || (c == par_MaxChars));
}

// This function give back the pointer to a XilEnv function. It will be searched by par_FunctionName
XilEnvFunctionCallPointer GetXilEnvFunctionCallPointer(int par_FunctionNr, const char *par_FunctionName)
{
#if (XILENV_INTERFACE_TYPE == XILENV_STUB_INTERFACE_TYPE)
     (void)par_FunctionNr;
     return XilEnvInternal_GetStubFuncPtr(par_FunctionName);
#else
    static struct {
        XilEnvFunctionCallPointer fkt_ptrs[0x400];
#ifdef _WIN32
        HMODULE hModulDll;
#else
        void *SharedLibHandle;
#endif
        int StubFlag;
        char DllNameWithPath[MAX_PATH];
    } DllFunctionCache;

#ifdef _WIN32
    if (DllFunctionCache.hModulDll == NULL) {
#else
    if (DllFunctionCache.SharedLibHandle == NULL) {
#endif
        if (DllFunctionCache.StubFlag) {
STUB_OUT:
            return XilEnvInternal_GetStubFuncPtr(par_FunctionName);
        } else {
            DllFunctionCache.DllNameWithPath[0] = 0;
            if (((par_FunctionNr < 0) && (par_FunctionName == NULL)) ||
                GetEnvironmentVariableA("XILENV_FUNC_STUB", DllFunctionCache.DllNameWithPath, sizeof(DllFunctionCache.DllNameWithPath)) > 0) {
                DllFunctionCache.StubFlag = 1;
                goto STUB_OUT;
            }
            if (GetEnvironmentVariableA("XILENV_DLL_PATH", DllFunctionCache.DllNameWithPath, sizeof(DllFunctionCache.DllNameWithPath) - 25) <= 0) {
                char* p, *CommandLine;
                int QuoteSign = 0;
                p = CommandLine = GetCommandLineA();
                if (p != NULL) {
                    // first parameter are the process itself
                    while ((*p != 0) && ((*p != ' ') || QuoteSign)) {
                        if (*p == '"') QuoteSign = !QuoteSign;
                        p++;
                    }
                    while (*p != 0) {
                        if (p[0] == '-') {
                            char *h = NULL;
                            if (!XilEnvInternal_strnicmp("-stub", p, 6)) {
                                DllFunctionCache.StubFlag = 1;
                                goto STUB_OUT;
                            } else if (!XilEnvInternal_strnicmp("-dllpath ", p, 9)) {
                                // path where to find the DLL are defined with parameter (can be include " chars)
                                //const char *s = p + 2;
                                char *d = DllFunctionCache.DllNameWithPath;
                                QuoteSign = 0;
                                p += 9; // -dllpath<whitespace>
                                while (isascii(*p) && isspace(*p)) p++;
                                while ((*p != 0) && ((*p != ' ') || QuoteSign)) {
                                    if (*p == '"') QuoteSign = !QuoteSign;
                                    else if ((d - DllFunctionCache.DllNameWithPath) < (sizeof(DllFunctionCache.DllNameWithPath) - 25)) *d++ = *p;
                                    p++;
                                }
                                *d = 0;
                                break;  // while (*p != 0)
                            } else if (((p[1] == 'w') || (p[1] == 'q')) && (strtoul(p + 2, &h, 0) > 0) && (h != NULL) && (*h == ' ')) {
                                const char *s = h;
                                const char *e;
                                while (*s == ' ') s++;
                                if (*s == '"') s++;
                                e = s;
                                while ((*e != 0) && (*e != ' ') && (*e != '"')) e++;
                                while ((e > s) && (*e != '\\') && (*e != '/')) e--;
                                // path of XilEnv executable
                                if ((e - s) < (sizeof(DllFunctionCache.DllNameWithPath) - 25)) {
                                    int Len = e - s;
                                    memcpy(DllFunctionCache.DllNameWithPath, s, Len);
                                    DllFunctionCache.DllNameWithPath[Len] = 0;
                                }
                                break;  // while (*p != 0)
                            }
                        }
                        p++;
                    }
                }
            }
            if (DllFunctionCache.DllNameWithPath[0]) {
                size_t len = strlen(DllFunctionCache.DllNameWithPath);
                if (DllFunctionCache.DllNameWithPath[len-1] == '\\')  ;
                else if (DllFunctionCache.DllNameWithPath[len-1] == '/') ;
                else XilEnvInternal_Strcat (DllFunctionCache.DllNameWithPath, sizeof(DllFunctionCache.DllNameWithPath), PATH_SEPARTOR);
            }
#ifdef _WIN32
#ifdef _MSC_VER
    #pragma warning(push)
    /* warning C4127: conditional expression is constant */
    #pragma warning(disable: 4127)
#endif
            if (sizeof(void*) == 8) {
                XilEnvInternal_Strcat (DllFunctionCache.DllNameWithPath, sizeof(DllFunctionCache.DllNameWithPath), "XilEnvExtProc64.dll");
            } else {
                XilEnvInternal_Strcat (DllFunctionCache.DllNameWithPath, sizeof(DllFunctionCache.DllNameWithPath), "Bin32\\XilEnvExtProc32.dll");
            }
#ifdef _MSC_VER
    #pragma warning(pop)
#endif
            if (sizeof(void*) == 8) {
                DllFunctionCache.hModulDll = LoadLibraryA (DllFunctionCache.DllNameWithPath);
            } else {
                DllFunctionCache.hModulDll = LoadLibraryExA (DllFunctionCache.DllNameWithPath, NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR);
            }

            if (DllFunctionCache.hModulDll == NULL) {
                char *lpMsgBuf = NULL;
                DWORD dw = GetLastError ();
                FormatMessageA (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                FORMAT_MESSAGE_FROM_SYSTEM |
                                FORMAT_MESSAGE_IGNORE_INSERTS,
                                NULL,
                                dw,
                                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                (LPSTR) &lpMsgBuf,
                                0, NULL);
#else
            if (sizeof(void*) == 8) {
                XilEnvInternal_Strcat(DllFunctionCache.DllNameWithPath, sizeof(DllFunctionCache.DllNameWithPath), "libXilEnvExtProc64.so");
            }  else {
                XilEnvInternal_Strcat(DllFunctionCache.DllNameWithPath, sizeof(DllFunctionCache.DllNameWithPath), "Bin32/libXilEnvExtProc32.so");
            }
            DllFunctionCache.SharedLibHandle = dlopen(DllFunctionCache.DllNameWithPath, RTLD_LAZY); // RTLD_NOW | RTLD_GLOBAL);
            if (DllFunctionCache.SharedLibHandle == NULL) {
                char *lpMsgBuf = dlerror();
#endif
                if (par_FunctionNr == 0x13) { //  no recursive call if ThrowErrorNoVariableArguments
                    printf ("cannot load DLL \"%s\": %s", DllFunctionCache.DllNameWithPath, lpMsgBuf);
                } else {
                    ThrowError (1, "cannot load DLL \"%s\": %s", DllFunctionCache.DllNameWithPath, lpMsgBuf);
                }
#ifdef _WIN32
                LocalFree (lpMsgBuf);
#endif
                return XilEnvInternal_GetStubFuncPtr(par_FunctionName);
            }
        }
    }
    if (DllFunctionCache.fkt_ptrs[par_FunctionNr] == NULL) {
#ifdef _WIN32
        DllFunctionCache.fkt_ptrs[par_FunctionNr] = (XilEnvFunctionCallPointer)GetProcAddress(DllFunctionCache.hModulDll, par_FunctionName);
#else
        DllFunctionCache.fkt_ptrs[par_FunctionNr] = (XilEnvFunctionCallPointer)dlsym(DllFunctionCache.SharedLibHandle, par_FunctionName);
#endif
        if (DllFunctionCache.fkt_ptrs[par_FunctionNr] == NULL) {
            if (par_FunctionNr == 0x13) { //  no recursive call if ThrowErrorNoVariableArguments
                printf ("cannot find function \"%s\" inside DLL \"%s\"", par_FunctionName, DllFunctionCache.DllNameWithPath);
            } else {
                ThrowError (1, "cannot find function \"%s\" inside DLL \"%s\"", par_FunctionName, DllFunctionCache.DllNameWithPath);
            }
            return XilEnvInternal_GetStubFuncPtr(par_FunctionName);
        }
    }
    return DllFunctionCache.fkt_ptrs[par_FunctionNr];
#endif
}

#ifndef XILENV_NO_MAIN
int main(int argc, char* argv[])
{
    #ifndef XILENV_USE_OWN_PROCESS_TASKS_LIST
    EXTERN_PROCESS_TASKS_LIST ExternProcessTasksList[1] = {{"", reference_varis, init_test_object, cyclic_test_object, terminate_test_object}};
    #endif

    (void)argc;
    (void)argv;

    return ExternalProcessMain (0, ExternProcessTasksList, sizeof(ExternProcessTasksList) / sizeof(ExternProcessTasksList[0]));
}

#ifdef _WIN32
int WINAPI WinMain (HINSTANCE h_instance,
                    HINSTANCE h_prev_instance,
                    LPSTR lpsz_cmdline,
                    int i_cmd_show)
{
    #ifndef XILENV_USE_OWN_PROCESS_TASKS_LIST
    EXTERN_PROCESS_TASKS_LIST ExternProcessTasksList[1] = {{"", reference_varis, init_test_object, cyclic_test_object, terminate_test_object}};
    #endif

    (void)h_instance;
    (void)h_prev_instance;
    (void)lpsz_cmdline;
    (void)i_cmd_show;

    return ExternalProcessMain (1, ExternProcessTasksList, sizeof(ExternProcessTasksList) / sizeof(ExternProcessTasksList[0]));
}
#endif
#endif /* XILENV_NO_MAIN */
