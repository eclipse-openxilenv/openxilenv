#pragma once
#include <QString>
#define QStringToConstChar(s) ((s).toUtf8().constData())
#define QStringToChar(s) ((s).toUtf8().data())
#define CharToQString(s) (QString().fromUtf8(s))

// The returned string have to be give free with my_free() function (if it is none zero)
char *MallocCopyString(QString &par_String);
char *ReallocCopyString(char *par_Dst, QString &par_String);


