// PriorityBoosterClient.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <winioctl.h>

#include "..\PriorityBooster\PriorityBooster.h"
#include <cstdio>
#include <cstdlib>

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        std::printf("PriorityBoosterClient <ThreadId> <Priority>\n");
        return 1;
    }

    int threadId = std::atoi(argv[1]);
    int priority = std::atoi(argv[2]);

    HANDLE thread = OpenThread(THREAD_SET_INFORMATION, FALSE, threadId);
    if (thread == nullptr)
    {
        std::printf("Failed to open thread %d (%08x)\n", threadId, GetLastError());
        return 1;
    }
    
    HANDLE device = CreateFile(LR"(\\.\PriorityBooster)", GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
    if (device == nullptr)
    {
        std::printf("Failed to open the device (%08x)\n", GetLastError());
        return 1;
    }

    SetThreadPriorityData stpd{};
    stpd.Thread = thread;
    stpd.Priority = priority;

    DWORD bytesReturned;
    if (DeviceIoControl(
        device,
        PRIORITY_BOOSTER_IOCTL_SET_THREAD_PRIORITY,
        &stpd,
        sizeof(stpd),
        nullptr,
        0,
        &bytesReturned,
        nullptr))
    {
        std::printf("Success!\n");
    }
    else
    {
        std::printf("Failed... (%08x)\n", GetLastError());
    }

    CloseHandle(device);
    CloseHandle(thread);
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started:
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
