#pragma once

#if __cplusplus
extern "C" {
#endif
	typedef struct _VFSEARCHHOOKINFO {
		DWORD unused;
	} VFSEARCHHOOKINFO, *PVFSEARCHHOOKINFO;

	typedef BOOL(__stdcall *CDEFAULTSEARCHCALLBACK)(LPCWSTR, PVOID);

	__declspec(dllexport) PVFSEARCHHOOKINFO VFAddRefSearchHook(PVOID pDefView, HWND hwnd, CDEFAULTSEARCHCALLBACK callback, PVOID userData);
	__declspec(dllexport) void VFReleaseSearchHook(PVFSEARCHHOOKINFO);

	void FirstInitializeSearch();
	void CleanupSearch();

#if __cplusplus
	}
#endif