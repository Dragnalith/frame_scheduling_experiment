#include <windows.h>

void AssertHandler(bool test)
{
    if (!test)
    {
        DebugBreak();
    }
}