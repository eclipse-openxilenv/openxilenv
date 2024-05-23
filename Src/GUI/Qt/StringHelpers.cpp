#include "StringHelpers.h"

extern "C" {
#include "MyMemory.h"
}

char *MallocCopyString(QString &par_String)
{
    int Len = strlen(QStringToConstChar(par_String)) + 1;
    char * Ret = (char*)my_malloc(Len);
    if (Ret != nullptr) {
        strcpy(Ret, QStringToConstChar(par_String));
    }
    return Ret;
}

char *ReallocCopyString(char *par_Dst, QString &par_String)
{
    int Len = strlen(QStringToConstChar(par_String)) + 1;
    char * Ret = (char*)my_realloc(par_Dst, Len);
    if (Ret != nullptr) {
        strcpy(Ret, QStringToConstChar(par_String));
    }
    return Ret;
}
