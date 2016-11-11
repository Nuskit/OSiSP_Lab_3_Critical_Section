#include "stdafx.h"
#include "CriticalSection.h"

std::wstring CriticalSection::GenerateName(const wchar_t* prefix, const wchar_t* name)
{
	std::wstring result = std::wstring::basic_string().append(prefix).append(name);
	return result.c_str();
}

void CriticalSection::InitializeCriticalSection(const wchar_t* name)
{
	status->lockCount = 0;
	status->owningThread = 0;
	status->recursionCount = 0;
	status->spinCount = 0;
}

HANDLE CriticalSection::CreateSectionMap(const std::wstring& fileName)
{
	return CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(*status), fileName.c_str());
}

HANDLE CriticalSection::OpenSectionMap(const std::wstring& fileName)
{
	return OpenFileMappingW(FILE_MAP_ALL_ACCESS, false, fileName.c_str());
}

void CriticalSection::CreateSection(const wchar_t* name, DWORD spinCount)
{
	std::wstring fileName = GenerateName(L"Global\\FILE_", name);
	mappedFile = CreateSectionMap(fileName);
	std::wstring eventName = GenerateName(L"Global\\EVENT_", name);
	lockEvent = CreateEventW(NULL, FALSE, FALSE, eventName.c_str());
	status = static_cast<CriticalSectionState*>(MapViewOfFile(mappedFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(*status)));
	InitializeCriticalSection(name);
	SetSpinCount(spinCount);
}

void CriticalSection::OpenSection(const wchar_t* name, DWORD spinCount)
{
	std::wstring fileName = GenerateName(L"Global\\FILE_", name);
	mappedFile = OpenSectionMap(fileName);
	std::wstring eventName = GenerateName(L"Global\\EVENT_", name);
	lockEvent = OpenEventW(EVENT_ALL_ACCESS, false, eventName.c_str());
	status = static_cast<CriticalSectionState*>(MapViewOfFile(mappedFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(*status)));
}

CriticalSection::CriticalSection(const wchar_t* name, bool isCreateSection, DWORD spinCount)
{
	if (isCreateSection)
		CreateSection(name, spinCount);
	else
		OpenSection(name, spinCount);
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

void CriticalSection::TakeByThread()
{
	InterlockedIncrement(&status->lockCount);
	InterlockedExchange(&status->owningThread, GetCurrentThreadId());
}

void CriticalSection::LeaveByThread()
{
	InterlockedDecrement(&status->lockCount);
	InterlockedExchange(&status->owningThread, 0);
}

void CriticalSection::WaitFreeCriticalSection()
{
	InterlockedIncrement(&status->lockCount);
	while (!TryEnterCriticalSection())
	{
		WaitForSingleObject(lockEvent, INFINITE);
	}
	InterlockedDecrement(&status->lockCount);
}

void CriticalSection::EnterCriticalSection()
{
	if (!IsThreadInCriticalSection())
		WaitFreeCriticalSection();
	else
		InterlockedIncrement(&status->recursionCount);
}

bool CriticalSection::IsThreadInCriticalSection()
{
	return status->owningThread == GetCurrentThreadId();
}

bool CriticalSection::TryEnterCriticalSection()
{
	bool isFreeSection = false;
	LONG spinCount = status->spinCount;
	do
	{
		if (InterlockedCompareExchange(&status->recursionCount, 1, 0) == 0)
		{
			TakeByThread();
			isFreeSection = true;
		}
	} while ((!isFreeSection) && (--spinCount > 0));

	return isFreeSection;
}

void CriticalSection::LeaveCriticalSection()
{
	if (IsThreadInCriticalSection())
	{
		if (status->recursionCount == 1)
			LeaveByThread();
		InterlockedDecrement(&status->recursionCount);
		if (status->lockCount > 0)
			SetEvent(lockEvent);
	}
}
