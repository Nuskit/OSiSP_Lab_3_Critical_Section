#pragma once

#include "stdafx.h"

class CriticalSection {
public:
	CriticalSection(const wchar_t* pszName, bool isCreateSection, DWORD dwSpinCount = 4000);
	~CriticalSection();
	void SetSpinCount(LONG dwSpinCount);
	void EnterCriticalSection();
	bool TryEnterCriticalSection();
	void LeaveCriticalSection();
private:
	typedef struct _CriticalSectionState {
		volatile DWORD owningThread;
		volatile LONG spinCount;
		volatile LONG lockCount;
		volatile LONG recursionCount;
	} CriticalSectionState;
	HANDLE mappedFile;
	CriticalSectionState* status;
	HANDLE lockEvent;
private:
	static std::wstring GenerateName(const wchar_t* prefix, const wchar_t* name);
	void TakeByThread();
	void LeaveByThread();
	void WaitFreeCriticalSection();
	void InitializeCriticalSection(const wchar_t* name);
	bool IsThreadInCriticalSection();
	HANDLE CreateSectionMap(const std::wstring& fileName);
	HANDLE OpenSectionMap(const std::wstring& fileName);
	void CreateSection(const wchar_t* name, DWORD spinCount);
	void OpenSection(const wchar_t* name, DWORD spinCount);
};
