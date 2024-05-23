/* ------------- ExtProc_FMULoader.cpp ----------------------------------- *\
 *                                                                         *
 *  Programm: Softcar                                                      *
 *                                                                         *
 *  Autor : Eric Bieber  Abt.:                         Datum: 13.07.2021   *
 *                                                                         *
 *  Funktion : Main Routine fuer einen externen Prozess                    *
 *                                                                         *
\* ------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#ifdef _WIN32
#include <windows.h>
#else
#define MAX_PATH 1024
#define strcmpi(a,b) strcasecmp((a),(b)) 
#endif
#include "pugixml.hpp"
extern "C" {
#define XILENV_INTERFACE_TYPE XILENV_DLL_INTERFACE_TYPE
#include "XilEnvExtProc.h"
#define XILENV_NO_MAIN
#include "XilEnvExtProcMain.c"
}
#include "Fmu3Struct.h"
#include "Fmu3Extract.h"
#include "Fmu3Xml.h"
#include "Fmu3Execute.h"
#include "Fmu3LoadDll.h"

FMU Fmu;

int GetFmuPath(int argc, char* argv[], char *ret_Fmu, char *ret_ExtractedFmuDirectrory, int par_MaxLen)
{
    int x;
    int Ret = 0;

    for (x = 0; x < argc; x++) {
        if (!strcmpi("-fmu", argv[x])) {
            if (x < argc) {
                if ((int)strlen(argv[x + 1]) >= par_MaxLen) {
                    ret_Fmu[0] = 0;
                } else {
                    x++;
                    strcpy(ret_Fmu, argv[x]);
                    Ret |= 1;
                }
            }
        } else if (!strcmpi("-fmuextracted", argv[x])) {
            if (x < argc) {
                if ((int)strlen(argv[x + 1]) >= par_MaxLen) {
                    ret_ExtractedFmuDirectrory[0] = 0;
                } else {
                    x++;
                    strcpy(ret_ExtractedFmuDirectrory, argv[x]);
                    Ret |= 2;
                }
            }
        } else if (!strcmpi("-reference_parameters", argv[x])) {
            Ret |= 4;
        }
    }
    return Ret;
}

// This flag would be set to 1 if the loader have extracted the fmu and not the ExtProc_FMUExtract.exe
// so he knows if he should remove the directory or not
int HaveSelfExtracted;

int InitAll(int argc, char* argv[])
{
    char FmuFileName[MAX_PATH];
    char ExtractedFmuDirectrory[MAX_PATH];
    char ProcessName[MAX_PATH];
    int FmuParamsFlags;
    char *Env;

    if (((Env = getenv("REFERENCE_FMU_PARAMETERS")) != nullptr) &&
        (strcmpi(Env, "yes") == 0)) {
        Fmu.SyncParametersWithBlackboard = true;
    } else {
        Fmu.SyncParametersWithBlackboard = false;
    }

    if ((FmuParamsFlags = GetFmuPath(argc, argv, FmuFileName, ExtractedFmuDirectrory, MAX_PATH)) & 0x1) {  // es muss -fmu vorhanden sein!
#ifdef _WIN32
        GetFullPathNameA(FmuFileName, sizeof(ProcessName), ProcessName, NULL);
#else
        realpath(FmuFileName, ProcessName);
#endif

#ifdef HEAPCHECK        
        HeapDump2File("c:\\temp\\a_2.txt");
#endif
        Fmu.ProcessName = (char*)malloc (strlen(ProcessName) + 1);
        strcpy(Fmu.ProcessName, ProcessName);

        if ((FmuParamsFlags & 0x4) == 0x4) {
            Fmu.SyncParametersWithBlackboard = true;
        } 

        if ((FmuParamsFlags & 0x2) == 0) {  // es wurde kein -fmuextracted uerbgeben -> selber extrahieren
#ifdef _WIN32
            ExtractFmuToUniqueTempDirectory(FmuFileName, ExtractedFmuDirectrory);
#else
            printf("TODO\n");
            return 1;
#endif 
            HaveSelfExtracted = 1;
        }
        Fmu.ExtractedFmuDirectrory = (char*)malloc (strlen(ExtractedFmuDirectrory) + 1);
        strcpy(Fmu.ExtractedFmuDirectrory, ExtractedFmuDirectrory);

        Fmu.XmlPath = (char*)malloc (strlen(ExtractedFmuDirectrory) + 32);
        strcpy(Fmu.XmlPath, ExtractedFmuDirectrory);
#ifdef _WIN32
        strcat(Fmu.XmlPath, "\\modelDescription.xml");
#else
        strcat(Fmu.XmlPath, "/modelDescription.xml");
#endif
#ifdef HEAPCHECK        
        HeapDump2File("c:\\temp\\a_3.txt");
#endif
        if (ReadFmuXml(&Fmu, Fmu.XmlPath)) {
            return -1;
        }
#ifdef HEAPCHECK        
        HeapDump2File("c:\\temp\\a_4.txt");
#endif
        Fmu.ResourceDirectory = (char*)malloc (strlen(ExtractedFmuDirectrory) + 32);
        strcpy(Fmu.ResourceDirectory, "file:///");    // must be IETF RFC3986 syntax  (start with file:///)
        strcat(Fmu.ResourceDirectory, ExtractedFmuDirectrory);
#ifdef _WIN32
        strcat(Fmu.ResourceDirectory, "\\resources");
#else
        strcat(Fmu.ResourceDirectory, "/resources");
#endif
        Fmu.XcpXmlPath = (char*)malloc (strlen(ExtractedFmuDirectrory) + 32);
        strcpy(Fmu.XcpXmlPath, ExtractedFmuDirectrory);
#ifdef _WIN32
        strcat(Fmu.XcpXmlPath, "\\resources\\xcp.xml");
#else
        strcat(Fmu.XcpXmlPath, "/resources/xcp.xml");
#endif
        Fmu.DllName = (char*)malloc (strlen(Fmu.ModelIdentifier) + 5);   // 4 -> .DLL
        strcpy(Fmu.DllName, Fmu.ModelIdentifier);
#ifdef _WIN32
        strcat(Fmu.DllName, ".DLL");
#else
        strcat(Fmu.DllName, ".so");
#endif

#ifdef HEAPCHECK        
        HeapDump2File("c:\\temp\\a_5.txt");
#endif
        Fmu.DllPath = (char*)malloc (strlen(ExtractedFmuDirectrory) + 32 + strlen(Fmu.ModelIdentifier));
        strcpy(Fmu.DllPath, ExtractedFmuDirectrory);
#ifdef _WIN32
        if (sizeof(void*) == 8) {
            strcat(Fmu.DllPath, "\\binaries\\x86_64-windows\\");
        } else {
            strcat(Fmu.DllPath, "\\binaries\\x86-windows\\");
        }
#else
        if (sizeof(void*) == 8) {
            strcat(Fmu.DllPath, "/binaries/x86_64-linux/");
        }
        else {
            strcat(Fmu.DllPath, "/binaries/x86-linux/");
        }
#endif
        // extend the PATH variable with the DLL path. This neccessary if there are other DLLs inside this path
#ifdef _WIN32
        {
            char *Buffer = NULL;
            int BufferSize = 1024;
            int AddSize = strlen(Fmu.DllPath) + 2;  // +2 because of ";"
            while(1) {
                Buffer = (char*)realloc(Buffer, BufferSize);
                if (Buffer != NULL) {
                    int NeedSize = GetEnvironmentVariable ("PATH", Buffer, BufferSize);
                    if (NeedSize <= 0) {
                        return -1;
                    }
                    NeedSize += AddSize;
                    if (NeedSize > BufferSize) {
                        BufferSize = NeedSize;
                    } else {
                        break; // while(1)
                    }
                }
            }
            strcat(Buffer, ";");
            strcat(Buffer, Fmu.DllPath);
            SetEnvironmentVariable("PATH", Buffer);
        }
#else
        {
            const char *Path = getenv("PATH");
            char *NewPath = (char*)malloc(strlen(Path) + 1 + strlen(Fmu.DllPath) + 2);
            strcpy(NewPath, Path);
            strcat(NewPath, ":");
            strcat(NewPath, Fmu.DllPath);
            setenv("PATH", NewPath, 1);
            free(NewPath);
            setenv("LD_LIBRARY_PATH", Fmu.DllPath, 1);
        }
#endif
        strcat(Fmu.DllPath, Fmu.ModelIdentifier);
#ifdef _WIN32
        strcat(Fmu.DllPath, ".DLL");
#else
        strcat(Fmu.DllPath, ".so");
#endif
#ifdef HEAPCHECK        
        HeapDump2File("c:\\temp\\a_6.txt");
#endif
        if (FmuLoadDll(&Fmu, Fmu.DllPath)) return -1;

        // Icon falls vorhanden: PNG gehen nicht unter Windows (so einfach)
        /*{
            HWND Hwnd;
            char ImagePath[512];
            strcpy(ImagePath, ExtractedFmuDirectrory);
            strcat(ImagePath, "\\model.png");
            HINSTANCE hInstance = GetModuleHandle(NULL);
            HANDLE Icon = LoadBitmap(hInstance, ImagePath); // IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
            Hwnd = GetActiveWindow();
            SendMessage(Hwnd, WM_SETICON, ICON_SMALL, (LPARAM)Icon);
            SendMessage(Hwnd, WM_SETICON, ICON_BIG, (LPARAM)Icon);
        }*/
        //ExternProcessTasksList[0].DllName = Fmu.DllPath;
        return 0;
    }
    return -1;
}

#if 0
#define MAXEVENTCHANNELS 3

XCP_EVENT_DESC ucEventChannelSettings[MAXEVENTCHANNELS] =
{
    {1, 0x00, DAQ_TIMESTAMP_UNIT_10MS, NULL},     /*Event Channel 0 => Timing = 10ms (Timebase * 1), Prio = 0x00*/
    {2, 0x00, DAQ_TIMESTAMP_UNIT_10MS, NULL},    /*Event Channel 1 => Timing = 20ms (Timebase * 2), Prio = 0x00*/
    {10, 0x00, DAQ_TIMESTAMP_UNIT_10MS, NULL}    /*Event Channel 2 => Timing = 1000ms (Timebase * 10), Prio = 0x00*/
};
#endif

void reference_varis_1 (void)
{
    ReferenceAllVariablesToBlackboard(&Fmu);
}

int init_test_object_1 (void)
{
    FmuInit(&Fmu);
    return 0;
}

void cyclic_test_object_1 (void)
{
    FmuOneCycle(&Fmu);
}

void terminate_test_object_1 (void)
{
    FmuTerminate(&Fmu);
    FmuFreeDll(&Fmu); // muss vor loeschen passieren
    if (HaveSelfExtracted) {
#ifdef _WIN32
        DeleteDirectory(Fmu.ExtractedFmuDirectrory);
#else
        printf("TODO\n");
#endif
    }
}

int Loop (EXTERN_PROCESS_TASK_HANDLE Process)
{
    while (1) {
        switch (WaitUntilFunctionCallRequest (Process)) {
        case 1: //reference:
            reference_varis_1();
            break;
        case 2: //init:
            init_test_object_1();
            break;
        case 3: //cyclic:
            cyclic_test_object_1();
            break;
        case 4: //terminate:
            terminate_test_object_1();
            break;
        default:
        case 5:   // Process was terminated by XilEnv -> no more call
            //Sleep (1000);
            return 0;
        }
        AcknowledgmentOfFunctionCallRequest(Process, 0);
    }
}

int main(int argc, char* argv[])
{
    EXTERN_PROCESS_TASK_HANDLE Process = nullptr;
    EXTERN_PROCESS_TASKS_LIST ExternProcessTasksList[1];
    int ExternProcessTasksListElementCount;
     if (InitAll(argc, argv)) {
        ThrowError (1, "cannot find fmu (wrong parameter)");
        return 1;
    }
    SetExternalProcessExecutableName(Fmu.ProcessName);
    ExternProcessTasksList[0].TaskName = "";
    ExternProcessTasksList[0].reference = reference_varis_1;
    ExternProcessTasksList[0].init = init_test_object_1;
    ExternProcessTasksList[0].cyclic = cyclic_test_object_1;
    ExternProcessTasksList[0].terminate = terminate_test_object_1;
    ExternProcessTasksList[0].DllName = Fmu.DllPath;
    ExternProcessTasksListElementCount = 1;
    if (CheckIfConnectedToEx (ExternProcessTasksList, ExternProcessTasksListElementCount, &Process)) {
        ThrowError (1, "cannot connect to softcar");
        return 1;
    }
    Loop (Process);

    return 0;
}

