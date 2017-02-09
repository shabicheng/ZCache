#pragma once
#include "ZRlibrary.h"

inline void ZCheckPtr(void * ptr)
{
    if (ptr == NULL)
    {
        INNER_LOG(CERROR, "ÄÚ´æÉêÇëÊ§°Ü£¬³ÌĞòÍË³ö.");
        exit(1);
    }
}

const int MAX_COMMAND_ITEM_SIZE = 16;