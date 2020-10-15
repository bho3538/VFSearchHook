#pragma once
#include <atlbase.h>
#include <ShObjIdl_core.h>


//Undocumented Interface.
#if defined(__cplusplus) && !defined(CINTERFACE)

MIDL_INTERFACE("9A7A94F5-FBF1-49A8-B0D9-44667635FE97")
IShellSearchTarget : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE Search(LPCWSTR text1,DWORD dwVal1) = 0;
	virtual HRESULT STDMETHODCALLTYPE OnSearchTextNotify(LPCWSTR text, LPCWSTR text2,DWORD searchFlags) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetSearchText(LPWSTR buffer, DWORD bufferLen) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetPromptText(LPWSTR buffer, DWORD bufferLen) = 0; //for placeholder text
	//-----Not Included in Windows 7 (Included after windows 8?)-------
	virtual HRESULT STDMETHODCALLTYPE GetACEnumString() = 0; //Unknown
	virtual HRESULT STDMETHODCALLTYPE GetMenu() = 0;//Unknown
	virtual HRESULT STDMETHODCALLTYPE InitMenuPopup() = 0;//Unknown
	virtual HRESULT STDMETHODCALLTYPE OnMenuCommand() = 0;//Unknown
	virtual HRESULT STDMETHODCALLTYPE Enter() = 0;//Unknown
	virtual HRESULT STDMETHODCALLTYPE Exit() = 0;//Unknown
	//------------
};

#endif