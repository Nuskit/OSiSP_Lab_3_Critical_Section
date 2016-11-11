#include "stdafx.h"
#include "CriticalSection.h"

std::wstring CriticalSection::GenerateName(const wchar_t* prefix, const wchar_t* name)
{
	std::wstring result = std::wstring::basic_string().append(prefix).append(name);
	return result.c_str();
}

void CriticalSection::InitializeCriticalSection(const wchar_t* name)
{
	std::wstring eventName = GenerateName(L"EVENT", name);
	lockEvent = CreateEventW(NULL, FALSE, FALSE, eventName.c_str());
	status->lockCount = 0;
	status->owningThread = 0;
	status->recursionCount = 0;
	status->spinCount = 0;
	status->owningProc = 0;
}

CriticalSection::CriticalSection(const wchar_t* name, DWORD spinCount)
{
	std::wstring fileName = GenerateName(L"FILE", name);
	mappedFile = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL,
		PAGE_READWRITE, 0, sizeof(*status), fileName.c_str());
	status = static_cast<CriticalSectionState*>(MapViewOfFile(mappedFile, FILE_MAP_WRITE, 0, 0, 0));
	InitializeCriticalSection(name);
	SetSpinCount(spinCount);
}

CriticalSection::~CriticalSection()
{
	CloseHandle(lockEvent);
	UnmapViewOfFile(status);
	CloseHandle(mappedFile);
}

void CriticalSection::SetSpinCount(LONG spinCount)
{
	InterlockedExchange(&status->spinCount, spinCount);
}

void CriticalSection::TakeByThreadProc()
{
	InterlockedExchange(&status->owningProc, GetCurrentProcessId());
	InterlockedExchange(&status->owningThread, GetCurrentThreadId());
}

void CriticalSection::LeaveByThreadProc()
{
	InterlockedExchange(&status->owningThread, 0);
	InterlockedExchange(&status->owningProc, 0);
}

void CriticalSection::WaitFreeCriticalSection()
{
	while (!TryEnterCriticalSection())
	{
		InterlockedIncrement(&status->lockCount);
		WaitForSingleObject(lockEvent, INFINITE);
	}
}

void CriticalSection::EnterCriticalSection()
{
	if (!IsThreadProcInCriticalSection())
		WaitFreeCriticalSection();
	else
		InterlockedIncrement(&status->recursionCount);
}

bool CriticalSection::IsThreadProcInCriticalSection()
{
	return (status->owningProc == GetCurrentProcessId()) && (status->owningThread == GetCurrentThreadId());
}

bool CriticalSection::TryEnterCriticalSection()
{
	bool isFreeSection = false;
	LONG spinCount = status->spinCount;
	do
	{
		if (InterlockedCompareExchange(&status->lockCount, 1, 0) == 1)
		{
			TakeByThreadProc();
			isFreeSection = true;
		}
	} while ((!isFreeSection) && (--spinCount > 0));

	return isFreeSection;
}

void CriticalSection::LeaveCriticalSection()
{
	if (status->owningThread == GetCurrentThreadId())
	{
		if (status->recursionCount == 0)
		{
			LeaveByThreadProc();
			InterlockedDecrement(&status->lockCount);
			SetEvent(lockEvent);
		}
		else
			InterlockedDecrement(&status->recursionCount);
	}
}
