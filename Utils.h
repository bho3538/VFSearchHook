#pragma once
#include "stdafx.h"

PVOID VFGetRawMethodFromInterface(PVOID interf, DWORD index) {
#if _WIN64
	return *(PVOID*)(*(DWORD_PTR*)interf + index * 8);
#else
	return *(PVOID*)(*(DWORD_PTR*)interf + index * 4);
#endif
}
