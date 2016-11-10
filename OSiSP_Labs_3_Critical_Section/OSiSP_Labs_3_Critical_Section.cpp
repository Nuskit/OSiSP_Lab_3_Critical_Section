#include "stdafx.h"
#include "CriticalSection.h"
#include <vector>
#include <iostream>

using namespace std;
DWORD WINAPI ThreadProc(void *state)
{
	CriticalSection* section= (CriticalSection*)state;
	section->EnterCriticalSection();
	cout << GetCurrentThreadId() << endl;
	section->LeaveCriticalSection();
	return EXIT_SUCCESS;
}

#define COUNT_THREAD 5
int main()
{
	CriticalSection section(L"file", 4000);

	std::vector<HANDLE> threadBlock;
	for (int i = 0; i < COUNT_THREAD; ++i)
	{
		threadBlock.push_back(CreateThread(NULL, 0, &ThreadProc, &section, 0, NULL));
	}

	WaitForMultipleObjects(threadBlock.size(), threadBlock.data(), true, INFINITE);

	if (section.TryEnterCriticalSection())
		section.LeaveCriticalSection();

	getchar();
  return 0;
}

