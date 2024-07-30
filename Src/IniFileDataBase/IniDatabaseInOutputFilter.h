#ifndef INIDATABASEINOUTPUTFILTER_H
#define INIDATABASEINOUTPUTFILTER_H

#include <stdio.h>

FILE *CreateInOrOutputFilterProcessPipe(const char *par_ExecName, const char *par_FileName, int par_InOrOut);
int IsInOrOutputFilterProcessPipe(FILE *par_PipeFile);
int CloseInOrOutputFilterProcessPipe(FILE *par_PipeFile);

#endif // INIDATABASEINOUTPUTFILTER_H
