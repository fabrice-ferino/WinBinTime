#include <windows.h>
#include <psapi.h>

#include <stdio.h>
#include <tchar.h>
#include <stdint.h>

#include <vector>

void _tmain(int argc, TCHAR *argv[])
{
  STARTUPINFO si;
  PROCESS_INFORMATION pi;

  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  ZeroMemory(&pi, sizeof(pi));

  if (argc < 2)
  {
    _ftprintf(stderr, _T("Usage: %s [cmdline]\n"), argv[0]);
    return;
  }

  size_t totalLen = 0;
  std::vector<size_t> lengths(argc - 1);

  // concatenate all arguments to the command line
  for (int i = 1; i < argc; ++i)
  {
    lengths[i - 1] = _tcslen(argv[i]);
    totalLen += lengths[i-1] + 3; // +3 -> quotes and space or null 
  }
  LPTSTR commandLine = (LPTSTR) malloc(totalLen * sizeof(TCHAR));
  if (!commandLine)
  {
    _ftprintf(stderr, _T("Out of memory!\n"));
    return;
  }
  LPTSTR tmp = commandLine;
  for (int i = 1; i < argc; ++i)
  {
    *tmp++ = _T('"');
    memcpy(tmp, argv[i], lengths[i - 1]*sizeof(TCHAR));
    tmp += lengths[i - 1];
    *tmp++ = _T('"');
    *tmp++ = _T(' ');
  }
  *(tmp - 1) = 0;
  
  // Start the child process. 
  if (!CreateProcess(NULL,   // No module name (use command line)
    commandLine,          // Command line
    NULL,           // Process handle not inheritable
    NULL,           // Thread handle not inheritable
    TRUE,          // Set handle inheritance to TRUE
    0,              // No creation flags
    NULL,           // Use parent's environment block
    NULL,           // Use parent's starting directory 
    &si,            // Pointer to STARTUPINFO structure
    &pi)           // Pointer to PROCESS_INFORMATION structure
    )
  {
    _ftprintf(stderr, _T("CreateProcess failed (%d).\n"), GetLastError());
    goto exit;
  }
 
  // Wait until child process exits.
  WaitForSingleObject(pi.hProcess, INFINITE);

  // Query the information 
  FILETIME startTime, endTime, kernelTime, userTime;
  if (GetProcessTimes(pi.hProcess, &startTime, &endTime, &kernelTime, &userTime))
  {
    // convert to 64 bit values
    endTime.dwHighDateTime -= startTime.dwHighDateTime;
    endTime.dwLowDateTime -= startTime.dwLowDateTime;

    uint64_t hundredNs[3];
    hundredNs[0] = (((uint64_t)endTime.dwHighDateTime) << 32) | endTime.dwLowDateTime;
    hundredNs[1] = (((uint64_t)kernelTime.dwHighDateTime) << 32) | kernelTime.dwLowDateTime;
    hundredNs[2] = (((uint64_t)userTime.dwHighDateTime) << 32) | userTime.dwLowDateTime;

    _ftprintf(stderr, _T("Real: %g ms  User: %g ms Kernel: %g ms\n"),
      hundredNs[0] / 10000.0, hundredNs[1] / 10000.0, hundredNs[2] / 10000.0);
  }

  PROCESS_MEMORY_COUNTERS pmc;
  if (GetProcessMemoryInfo(pi.hProcess, &pmc, sizeof(pmc)))
  {
    _ftprintf(stderr, _T("\tPageFaultCount: %llu\n"), 
      (unsigned long long) pmc.PageFaultCount);
    _ftprintf(stderr, _T("\tPeakWorkingSetSize: %llu bytes\n"),
      (unsigned long long) pmc.PeakWorkingSetSize);
    _ftprintf(stderr, _T("\tWorkingSetSize: %llu bytes\n"), 
      (unsigned long long) pmc.WorkingSetSize);
    _ftprintf(stderr, _T("\tQuotaPeakPagedPoolUsage: %llu bytes\n"),
      (unsigned long long) pmc.QuotaPeakPagedPoolUsage);
    _ftprintf(stderr, _T("\tQuotaPagedPoolUsage: %llu bytes\n"),
      (unsigned long long) pmc.QuotaPagedPoolUsage);
    _ftprintf(stderr, _T("\tQuotaPeakNonPagedPoolUsage: %llu bytes\n"),
      (unsigned long long) pmc.QuotaPeakNonPagedPoolUsage);
    _ftprintf(stderr, _T("\tQuotaNonPagedPoolUsage: %llu bytes\n"),
      (unsigned long long) pmc.QuotaNonPagedPoolUsage);
    _ftprintf(stderr, _T("\tPagefileUsage: %llu bytes\n"), 
      (unsigned long long) pmc.PagefileUsage);
    _ftprintf(stderr, _T("\tPeakPagefileUsage: %llu bytes\n"),
      (unsigned long long) pmc.PeakPagefileUsage);
  }

  // Close process and thread handles. 
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);

exit:

  free(commandLine);
}

