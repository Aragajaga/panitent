#ifndef DEBUG_H
#define DEBUG_H

#include "stdafx.h"

#define EXPLAINMASK(dMask, dic)                 \
        bFirst = 1;                             \
        for (int i = 0; i < ARRAYSIZE(dic); i++)\
            if (dMask & dic[i].dwConst)         \
            {                                   \
                if (!bFirst--) printf(" | ");   \
                printf("%s", dic[i].szName);          \
            }                                   \
        printf(")\n");   

typedef struct _ASSOC
{
    DWORD dwConst;
    LPSTR szName;
} ASSOC;

void DebugVirtualMemoryInfo(void *memPtr);
void DebugPrintRect(RECT *rc);

#endif /* DEBUG_H */