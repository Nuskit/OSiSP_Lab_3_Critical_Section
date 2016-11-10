#pragma once

#include "stdafx.h"

class CriticalSection {
public:
	CriticalSection(const wchar_t* pszName, DWORD dwSpinCount);
	~CriticalSection();
	void SetSpinCount(LONG dwSpinCount);
	void EnterCriticalSection();
	bool TryEnterCriticalSection();
	void LeaveCriticalSection();
private:
	typedef struct _CriticalSectionState {
		LONG spinCount;
		LONG lockCount;
		DWORD owningThread;
		DWORD owningProc;
		LONG recursionCount;
		HANDLE lockEvent;
	} CriticalSectionState;
	HANDLE mappedFile;
	CriticalSectionState* status;
private:
	std::wstring GenerateName(const wchar_t* prefix, const wchar_t* name);
	void TakeByThreadProc();
	void LeaveByThreadProc();
	void WaitFreeCriticalSection();
	void InitializeCriticalSection(const wchar_t* name);
	bool IsThreadProcInCriticalSection();
};
