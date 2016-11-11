#include "stdafx.h"
#include "Criticalsection.h"

#define SPIN_COUNT 10
#define BUF_SIZE 256
#define ARRAY_SIZE 10
#define TRIES 5
#define FILE_VALUES L"Global\\Data"
#define SECTION_NAME L"CriticalSection"
#define NUM_PROC 5
#define TIME_WAIT_LOAD_ALL_PROCESS 100
using namespace std;

CriticalSection *section;
HANDLE hMap = INVALID_HANDLE_VALUE;
LPVOID pBuf;
void checkRecursiveSection()
{
	section->EnterCriticalSection();
	section->EnterCriticalSection();
	section->EnterCriticalSection();
	section->LeaveCriticalSection();
	section->LeaveCriticalSection();
	section->LeaveCriticalSection();
}

void workWithProc(wchar_t *fileName)
{
	section->EnterCriticalSection();
	PROCESS_INFORMATION processInformation[NUM_PROC];
	for (int i = 0; i < NUM_PROC; ++i)
	{
		STARTUPINFO startupInfo;
		ZeroMemory(&startupInfo, sizeof(STARTUPINFO));
		startupInfo.cb = sizeof(STARTUPINFO);

		wchar_t proc_line[266];
		swprintf(proc_line, L"%s %d", fileName, i);

		CreateProcess(fileName, proc_line, NULL, NULL, FALSE, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &startupInfo, &processInformation[i]);
	}
	Sleep(TIME_WAIT_LOAD_ALL_PROCESS);
	section->LeaveCriticalSection();

	getchar();
	for (int i = 0; i < NUM_PROC; ++i)
		if (WaitForSingleObject(processInformation[i].hProcess, INFINITY) != WAIT_OBJECT_0)
		{
			cout << "Problem close proc " << processInformation[i].dwProcessId << ", terminated." << endl;
			TerminateProcess(processInformation[i].hProcess, EXIT_FAILURE);
		}
}

DWORD WINAPI ProcessProcedure(LPVOID state = NULL)
{
	int* values = static_cast<int*>(pBuf);
	for (int j = 0; j < TRIES; j++)
	{
		section->EnterCriticalSection();
		cout << "Proc id " << GetCurrentProcessId() << ", thread id " << GetCurrentThreadId() << "." << endl;
		for (int i = 0; i < ARRAY_SIZE; i++)
		{
			cout << ++values[i] << " ";
		}
		cout << endl;
		section->LeaveCriticalSection();
		Sleep(rand() % 21);
	}

	return EXIT_SUCCESS;
}

void OpenMapView()
{
	pBuf = MapViewOfFile(hMap,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		BUF_SIZE);
}

void CloseMapView()
{
	UnmapViewOfFile(pBuf);
}

int wmain(int argc, wchar_t *argv[])
{
	if (argc == 1)
	{
		hMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, BUF_SIZE, FILE_VALUES);
		if (!hMap)
		{
			cout << "Load in Administator mode" << endl;
			system("pause");
		}
		else
		{
			OpenMapView();
			section = new CriticalSection(SECTION_NAME, true, SPIN_COUNT);
			checkRecursiveSection();

			int values[ARRAY_SIZE];
			for (int i = 0; i < ARRAY_SIZE; i++)
				values[i] = i;

			CopyMemory(pBuf, values, sizeof(int) * ARRAY_SIZE);
			cout << "Initialized values" << endl;

			workWithProc(argv[0]);
			CloseMapView();
			delete section;
		}
	}
	else
	{
		hMap = OpenFileMapping(FILE_MAP_ALL_ACCESS, false, FILE_VALUES);
		if (!hMap)
			cout << "Error open mapping" << endl;
		else
		{
			OpenMapView();
			section = new CriticalSection(SECTION_NAME, false);
			HANDLE thread = CreateThread(NULL, 0, &ProcessProcedure, NULL, NULL, NULL);
			ProcessProcedure();
			WaitForSingleObject(thread, INFINITE);
			CloseMapView();
			delete section;
		}
	}

	CloseHandle(hMap);
	return 0;
}
