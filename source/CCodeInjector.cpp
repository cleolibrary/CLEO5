#include "stdafx.h"
#include "CleoBase.h"
#include "CDebug.h"
#include "CCodeInjector.h"
#include <windows.h> 

namespace CLEO
{
    void CCodeInjector::OpenReadWriteAccess()
    {
        if (bAccessOpen) return;

        auto dwLoadOffset = reinterpret_cast<memory_pointer>(GetModuleHandle(nullptr));

        // Unprotect image - make .text and .rdata section writable
        auto pImageBase = reinterpret_cast<BYTE *>(dwLoadOffset);
        auto pDosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(dwLoadOffset);
        auto pNtHeader = reinterpret_cast<PIMAGE_NT_HEADERS>(pImageBase + pDosHeader->e_lfanew);
        auto pSection = IMAGE_FIRST_SECTION(pNtHeader);

        for (int i = 0; i < pNtHeader->FileHeader.NumberOfSections; ++i, ++pSection)
        {
            std::string sectionName(reinterpret_cast<char*>(pSection->Name));
            if (sectionName == ".text" || sectionName == ".rdata")
            {
                DWORD dwPhysSize = (pSection->Misc.VirtualSize + 4095) & ~4095;
                TRACE("Unprotecting memory region '%s': 0x%08X (size: 0x%08X)",
                    sectionName.c_str(),
                    reinterpret_cast<DWORD>(pSection->VirtualAddress),
                    dwPhysSize
                );
                DWORD oldProtect;
                DWORD newProtect = (pSection->Characteristics & IMAGE_SCN_MEM_EXECUTE) ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE;
                if (!VirtualProtect(pImageBase + pSection->VirtualAddress, dwPhysSize, newProtect, &oldProtect))
                {
                    SHOW_ERROR("Virtual protect error");
                }
            }
        }

        bAccessOpen = true;
    }

    void CCodeInjector::CloseReadWriteAccess()
    {
        if (!bAccessOpen) return;

        auto dwLoadOffset = static_cast<memory_pointer>(GetModuleHandle(nullptr));

        // Unprotect image - make .text and .rdata section writeable
        auto pImageBase = (BYTE *)dwLoadOffset;
        auto pDosHeader = (PIMAGE_DOS_HEADER)dwLoadOffset;
        auto pNtHeader = (PIMAGE_NT_HEADERS)(pImageBase + pDosHeader->e_lfanew);
        auto pSection = IMAGE_FIRST_SECTION(pNtHeader);

        for (int i = pNtHeader->FileHeader.NumberOfSections; i; i--, pSection++)
        {
            if (!strcmp((char*)pSection->Name, ".text") || !strcmp((char*)pSection->Name, ".rdata"))
            {
                DWORD dwPhysSize = (pSection->Misc.VirtualSize + 4095) & ~4095;
                TRACE("Reprotecting memory region '%s': 0x%08X (size: 0x%08X)",
                    pSection->Name,
                    (DWORD)pSection->VirtualAddress,
                    (DWORD)dwPhysSize
                );
                DWORD oldProtect, newProtect = (pSection->Characteristics & IMAGE_SCN_MEM_EXECUTE) ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE;
                if (!VirtualProtect(pImageBase + pSection->VirtualAddress, dwPhysSize, newProtect, &oldProtect))
                    SHOW_ERROR("Virtual protect error");
            }
        }

        bAccessOpen = false;
    }
}
