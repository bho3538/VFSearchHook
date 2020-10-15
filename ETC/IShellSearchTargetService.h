#include "StdAfx.h"
#include <atlbase.h>

//Undocumented Interface.
#if defined(__cplusplus) && !defined(CINTERFACE)

MIDL_INTERFACE("DDA3A58A-43DA-4A43-A5F2-F7ABF6E3C026")
IShellSearchTargetService : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE GetPromptText(LPWSTR buffer, DWORD bufferLen) = 0; //for placeholder text
	virtual HRESULT STDMETHODCALLTYPE GetTargetFlags(DWORD inFlags,PDWORD outFlags) = 0; //unknown

};
#endif