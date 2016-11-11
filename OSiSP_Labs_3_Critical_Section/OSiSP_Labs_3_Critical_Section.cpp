#include "stdafx.h"
#include "CriticalSection.h"

#define BUF_SIZE 256
#define ARRAY_SIZE 10
#define TRIES 5
#define FILE_VALUES L"Global\\Data"
#define SECTION_NAME L"CriticalSection"
#define NUM_PROC 2
using namespace std;

CriticalSection section(SECTION_NAME, 10);
HANDLE hMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, BUF_SIZE, FILE_VALUES);

void checkRecursiveSection()
{
	section.EnterCriticalSection();
	section.EnterCriticalSection();
	section.EnterCriticalSection();
	section.LeaveCriticalSection();
	section.LeaveCriticalSection();
	section.LeaveCriticalSection();
}

void workWithProc(wchar_t *fileName)
{
	section.EnterCriticalSection();
	PROCESS_INFORMATION processInformation[NUM_PROC];
	for (int i = 0; i < NUM_PROC; ++i)
	{
		STARTUPINFO startupInfo;
		ZeroMemory(&startupInfo, sizeof(STARTUPINFO));
		startupInfo.cb = sizeof(STARTUPINFO);

		wchar_t proc_num[256];
		swprintf(proc_num, L"%s %d", fileName, i);

		CreateProcess(fileName, proc_num, NULL, NULL, FALSE, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &startupInfo, &processInformation[i]);
	}
	Sleep(500);
	section.LeaveCriticalSection();

	getchar();
	for (int i = 0; i < NUM_PROC; ++i)
		if (WaitForSingleObject(processInformation[i].hProcess, 500) != WAIT_OBJECT_0)
		{
			cout << "Problem close proc " << processInformation[i].dwProcessId << ", terminated." << endl;
			TerminateProcess(processInformation[i].hProcess, EXIT_FAILURE);
		}
}

void ProcessProcedure()
{
	int* values = static_cast<int*>(MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, BUF_SIZE));
	for (int j = 0; j < TRIES; j++)
	{
		section.EnterCriticalSection();
		cout << "Proc num " << GetCurrentProcessId() << endl;
		for (int i = 0; i < ARRAY_SIZE; i++)
		{
			cout << ++values[i] << " ";
		}
		cout << endl;
		section.LeaveCriticalSection();
		Sleep(5+rand()%20);
	}

	UnmapViewOfFile(values);
	CloseHandle(hMap);
}

int wmain(int argc, wchar_t *argv[])
{
	if (!hMap)
		cout << "Load in Administator mode" << endl;
	else
	{
		if (argc == 1)
		{
			LPVOID pBuf = MapViewOfFile(hMap,
				FILE_MAP_ALL_ACCESS,
				0,
				0,
				BUF_SIZE);

			checkRecursiveSection();

			int values[ARRAY_SIZE];
			for (int i = 0; i < ARRAY_SIZE; i++)
				values[i] = i;

			CopyMemory(pBuf, values, sizeof(int) * ARRAY_SIZE);
			cout << "Initialized values" << endl;

			workWithProc(argv[0]);

			UnmapViewOfFile(pBuf);
		}
		else
		{
			ProcessProcedure();
		}
	}

	CloseHandle(hMap);
	return 0;
}
