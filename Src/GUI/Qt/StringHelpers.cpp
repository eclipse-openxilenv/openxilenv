#include "StringHelpers.h"

extern "C" {
#include "StringMaxChar.h"
}

char *MallocCopyString(QString &par_String)
{
    char * Ret = StringMalloc(QStringToConstChar(par_String));
    return Ret;
}

char *ReallocCopyString(char *par_Dst, QString &par_String)
{
    char * Ret = StringRealloc(par_Dst, QStringToConstChar(par_String));
    return Ret;
}
