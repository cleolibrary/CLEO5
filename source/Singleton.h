#pragma once
#include <Windows.h>
#include <tlhelp32.h>

class _CleoSingleton
{
public:
    _CleoSingleton()
    {
        MODULEENTRY32 module;
        module.dwSize = sizeof(MODULEENTRY32);
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());

        Module32First(snapshot, &module);
        if (snapshot != INVALID_HANDLE_VALUE)
        {
            size_t count = 0;
            do
            {
                if (_strcmpi(module.szModule, "CLEO.asi") == 0)
                {
                    count++;

                    if(count > 1)
                    {
                        CloseHandle(snapshot);
                        MessageBox(NULL, "Another copy of CLEO.asi is already loaded!\nPlease remove duplicated files.", "CLEO error", MB_SYSTEMMODAL | MB_TOPMOST | MB_ICONERROR | MB_OK);
                        exit(1);
                        break;
                    }
                }
            } while (Module32Next(snapshot, &module));

            CloseHandle(snapshot);
        }
    }
} CleoSingleton;

