#include "stdafx.h"
#include "VFSearchboxHook.h"
#include <ShlObj_core.h>
#include "Utils.h"

typedef HRESULT(__stdcall *TOnSearchTextNotify) (PVOID __this, LPCWSTR test, LPCWSTR test2,DWORD dwVal1);


typedef struct _VFSEARCHHOOKINFOINT {
	DWORD dwSize;
	DWORD dwFlags;
	DWORD dwRefCount;
	DWORD dwWaitCount;
	HWND hwnd;
	CDEFAULTSEARCHCALLBACK callback;
	CRITICAL_SECTION lockRef;
	HANDLE hWaitThread;
	PVOID userData;
	WCHAR LastText[MAX_PATH];
} VFSEARCHHOOKINFOINT, *PVFSEARCHHOOKINFOINT;

typedef struct _VFSEARCHHOOKINFOLISTITEM {
	HWND id;
	PVFSEARCHHOOKINFOINT item;
	PVOID next;
} VFSEARCHHOOKINFOLISTITEM,*PVFSEARCHHOOKINFOLISTITEM;

PVFSEARCHHOOKINFOLISTITEM _itemList = NULL;
CRITICAL_SECTION _lockList = { 0, };
TOnSearchTextNotify pNextFunc = NULL;
TOnSearchTextNotify pOriginalFunc = NULL;

const IID IID_IShellSearchTarget = { 0x9a7a94f5,0xfbf1,0x49a8,{0xb0,0xd9,0x44,0x66,0x76,0x35,0xfe,0x97} };

HRESULT __stdcall _VFOnSearchTextNotify(PVOID __this, LPCWSTR text, LPCWSTR text2, DWORD dwVal1);
void _VFInsertAtSearchList(HWND id,PVFSEARCHHOOKINFOINT item);
void _VFDeleteAtSearchList(HWND id);
void FinalReleaseSearch(PVFSEARCHHOOKINFOINT item);
PVFSEARCHHOOKINFOINT _VFGetItemFromSearchList(HWND id);

DWORD WINAPI _VFSearchWatchThread(PVOID args);

__declspec(dllexport) PVFSEARCHHOOKINFO VFAddRefSearchHook(PVOID pDefView,HWND hwnd, CDEFAULTSEARCHCALLBACK callback,PVOID userData) {
	PVFSEARCHHOOKINFOINT info = NULL;
	IUnknown* pComView = NULL;
	IUnknown* pSearchTarget = NULL;
	HRESULT hr = E_FAIL;
	MH_STATUS hookStatus;
	//HWND topHwnd = hwnd;

	//for (;;) {
	//	topHwnd = GetParent(hwnd);
	//	if (topHwnd) {
	//		hwnd = topHwnd;
	//	}
	//	else {
	//		topHwnd = hwnd;
	//		break;
	//	}
	//}


	if (!pDefView) {
		goto escapeArea;
	}

	pComView = (IUnknown*)pDefView;
	
	//For hook undocumented interface (ishellsearchtarget)
	hr = pComView->lpVtbl->QueryInterface(pComView, &IID_IShellSearchTarget, (LPVOID*)&pSearchTarget);
	if (FAILED(hr)) {
		goto escapeArea;
	}

	hwnd = GetAncestor(hwnd, GA_ROOT);

	info = _VFGetItemFromSearchList(hwnd);

	if (info) {
		//add ref count
		EnterCriticalSection(&info->lockRef);
		info->dwRefCount += 1;
		//update new callback
		info->callback = callback;
		info->userData = userData;
		LeaveCriticalSection(&info->lockRef);
	}
	else {
		info = (PVFSEARCHHOOKINFOINT)malloc(sizeof(VFSEARCHHOOKINFOINT));
		if (!info) {
			goto escapeArea;
		}
		ZeroMemory(info, sizeof(VFSEARCHHOOKINFOINT));

		info->dwSize = sizeof(VFSEARCHHOOKINFOINT);
		info->callback = callback;
		info->dwRefCount = 1;
		info->hwnd = hwnd;

		InitializeCriticalSection(&info->lockRef);

		if (!pOriginalFunc) {
			//install hook
			pOriginalFunc = VFGetRawMethodFromInterface(pSearchTarget, 4);
			hookStatus = MH_CreateHook(pOriginalFunc, &_VFOnSearchTextNotify, (LPVOID*)&pNextFunc);
			if (hookStatus == MH_OK)
			{
				if (MH_EnableHook(pOriginalFunc) != MH_OK)
				{
					hr = E_FAIL;
					goto escapeArea;
				}
			}
		}
		//start watch thread (To detect user typing is end)
		info->hWaitThread = CreateThread(NULL, 0, _VFSearchWatchThread, info, 0, NULL);

		_VFInsertAtSearchList(info->hwnd, info);
	}

escapeArea:

	if (pSearchTarget) {
		pSearchTarget->lpVtbl->Release(pSearchTarget);
	}

	if (FAILED(hr)) {
		free(info);
		info = NULL;
	}


	return (PVFSEARCHHOOKINFO)info;
}
__declspec(dllexport) void VFReleaseSearchHook(PVFSEARCHHOOKINFO pInfo) {
	PVFSEARCHHOOKINFOINT info = NULL;
	DWORD refCount = 0;

	if (!pInfo) {
		return;
	}

	info = (PVFSEARCHHOOKINFOINT)pInfo;

	EnterCriticalSection(&info->lockRef);
	info->dwRefCount -= 1;
	refCount = info->dwRefCount;

	if (refCount == 0) {
		_VFDeleteAtSearchList(info->hwnd);
	}

	LeaveCriticalSection(&info->lockRef);

	if (refCount == 0) {
		FinalReleaseSearch(info);
	}
}

//Hooked undocumented interface
HRESULT __stdcall _VFOnSearchTextNotify(PVOID __this,LPCWSTR text, LPCWSTR text2, DWORD dwVal1) {
	HWND hwnd = GetForegroundWindow();
	PVFSEARCHHOOKINFOINT item = _VFGetItemFromSearchList(hwnd);
	DWORD len = 0;

	if (item && item->callback) {		
		len = (DWORD)wcslen(text);

		wcscpy_s(item->LastText, len + 1, text);

		item->dwWaitCount = 0;
		//disable original operation;
		return S_FALSE;
	}
	return pNextFunc(__this, text, text2, dwVal1);

}

void FirstInitializeSearch() {
	MH_Initialize();
	InitializeCriticalSection(&_lockList);
}
void CleanupSearch() {
	if (pOriginalFunc) {
		MH_DisableHook(pOriginalFunc);
		MH_RemoveHook(pOriginalFunc);
	}
}

void FinalReleaseSearch(PVFSEARCHHOOKINFOINT info) {
	if (!info) {
		return;
	}

	DeleteCriticalSection(&info->lockRef);

	info->dwFlags |= 2;
	if (info->hWaitThread != NULL) {
		if (WaitForSingleObject(info->hWaitThread, 10000) == WAIT_TIMEOUT) {
			TerminateThread(info->hWaitThread,0);
		}
		CloseHandle(info->hWaitThread);
	}

	free(info);
}

void _VFInsertAtSearchList(HWND id, PVFSEARCHHOOKINFOINT insertItem) {
	PVFSEARCHHOOKINFOLISTITEM newItem = NULL;
	PVFSEARCHHOOKINFOLISTITEM item = NULL;
	if (!insertItem && !_itemList) {
		return;
	}

	newItem = (PVFSEARCHHOOKINFOLISTITEM)malloc(sizeof(VFSEARCHHOOKINFOLISTITEM));

	if (!newItem) {
		return;
	}
	newItem->id = id;
	newItem->item = insertItem;
	newItem->next = 0;
	
	EnterCriticalSection(&_lockList);

	if (_itemList) {
		item = _itemList;
		for (;;) {
			if (!item->next) {
				break;
			}
			item = item->next;
		}
		item->next = newItem;
	}
	else {
		_itemList = newItem;
	}

	LeaveCriticalSection(&_lockList);


}
void _VFDeleteAtSearchList(HWND id) {
	PVFSEARCHHOOKINFOLISTITEM item = NULL;
	PVFSEARCHHOOKINFOLISTITEM beforeItem = NULL;

	EnterCriticalSection(&_lockList);

	if (_itemList) {
		item = _itemList;
		for (;;) {
			if (item->id == id) {
				if (beforeItem) {
					beforeItem->next = item->next;
				}
				else {
					_itemList = NULL;
				}

				free(item);
				break;
			}

			if (!item->next) {
				break;
			}
			beforeItem = item;
			item = item->next;
		}
	}

	LeaveCriticalSection(&_lockList);
}

PVFSEARCHHOOKINFOINT _VFGetItemFromSearchList(HWND id) {
	PVFSEARCHHOOKINFOLISTITEM item = NULL;
	BOOL bFound = FALSE;

	EnterCriticalSection(&_lockList);

	if (_itemList) {
		item = _itemList;
		for (;;) {
			if (!item) {
				break;
			}
			if (item->id == id) {
				bFound = TRUE;
				break;
			}
			item = item->next;
		}
	}

	LeaveCriticalSection(&_lockList);

	if (bFound && item) {
		return item->item;
	}
	else {
		return NULL;
	}
}

//Detect user typing and if end -> call callback or original function (not in nse)
DWORD WINAPI _VFSearchWatchThread(PVOID args) {
	PVFSEARCHHOOKINFOINT info = (PVFSEARCHHOOKINFOINT)args;
	DWORD dwVal = 0;
	DWORD dwFlags = 0;

	if (!info) {
		return 0;
	}

	for (;;) {
		dwFlags = info->dwFlags;

		if (dwFlags & 2) {
			break;
		}

		info->dwWaitCount += 1;
		if (info->dwWaitCount > 8) {
			dwVal = (DWORD)wcslen(info->LastText);
			if (dwVal > 0) {
				info->callback(info->LastText, info->userData);
				info->LastText[0] = L'\0';
			}
			info->dwWaitCount = 0;
		}

		Sleep(100);
	}

	return 0;
}