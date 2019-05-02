#pragma once

void AssertHandler(bool test);
#define DRGN_ASSERT(x) do { AssertHandler((x)); } while (false)