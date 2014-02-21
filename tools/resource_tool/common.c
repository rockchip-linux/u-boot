#include "common.h"

bool g_debug = 
#ifdef DEBUG
    true;
#else
    false;
#endif //DEBUG
