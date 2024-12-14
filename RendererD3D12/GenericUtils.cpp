#include "pch.h"
#include "GenericUtils.h"

typedef bool(WINAPI* LPFN_GLPI)(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION, PDWORD);
DWORD CountSetBits(ULONG_PTR bitMask);

bool GenericUtils::GetPhysicalCoreCount(uint32* physicalCoreCount, uint32* logicalCoreCount)
{
	{
		LPFN_GLPI glpi;

		PSYSTEM_LOGICAL_PROCESSOR_INFORMATION pBuffer = nullptr;
		PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = nullptr;
		uint32 returnLength = 0;
		uint32 logicalProcessorCount = 0;
		uint32 numaNodeCount = 0;
		uint32 processorCoreCount = 0;
		uint32 processorL1CacheCount = 0;
		uint32 processorL2CacheCount = 0;
		uint32 processorL3CacheCount = 0;
		uint32 processorPackageCount = 0;
		uint32 byteOffset = 0;
		PCACHE_DESCRIPTOR Cache;



		glpi = (LPFN_GLPI)GetProcAddress(GetModuleHandle(TEXT("kernel32")), "GetLogicalProcessorInformation");
		if (nullptr == glpi)
		{
			SYSTEM_INFO systemInfo;
			GetSystemInfo(&systemInfo);
			*physicalCoreCount = systemInfo.dwNumberOfProcessors;
			*logicalCoreCount = systemInfo.dwNumberOfProcessors;

			OutputDebugStringW(L"\nGetLogicalProcessorInformation is not supported.\n");
			return false;
		}

		bool done = false;
		while (!done)
		{
			DWORD rc = glpi(pBuffer, (PDWORD)&returnLength);

			if (false == rc)
			{
				if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
				{
					if (pBuffer)
						free(pBuffer);

					pBuffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc(returnLength);
				}
				else
				{
					break;
				}
			}
			else
			{
				done = true;
			}
		}

		ptr = pBuffer;

		while (byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= returnLength)
		{
			switch (ptr->Relationship)
			{
			case RelationNumaNode:
				// Non-NUMA systems report a single record of this type.
				numaNodeCount++;
				break;

			case RelationProcessorCore:
				processorCoreCount++;

				// A hyperthreaded core supplies more than one logical processor.
				logicalProcessorCount += CountSetBits(ptr->ProcessorMask);
				break;

			case RelationCache:
				// Cache data is in ptr->Cache, one CACHE_DESCRIPTOR structure for each cache. 
				Cache = &ptr->Cache;
				if (Cache->Level == 1)
				{
					processorL1CacheCount++;
				}
				else if (Cache->Level == 2)
				{
					processorL2CacheCount++;
				}
				else if (Cache->Level == 3)
				{
					processorL3CacheCount++;
				}
				break;

			case RelationProcessorPackage:
				// Logical processors share a physical package.
				processorPackageCount++;
				break;

			default:
				OutputDebugStringW(L"\nError: Unsupported LOGICAL_PROCESSOR_RELATIONSHIP value.\n");
				break;
			}
			byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
			ptr++;
		}
		*physicalCoreCount = processorCoreCount;
		*logicalCoreCount = logicalProcessorCount;
		//numaNodeCount;
		//processorPackageCount;
		//processorCoreCount;
		//logicalProcessorCount;
		//processorL1CacheCount;
		//processorL2CacheCount;
		//processorL3CacheCount

		free(pBuffer);
	}

	return true;
}

// Helper function to count set bits in the processor mask.
DWORD CountSetBits(ULONG_PTR bitMask)
{
	DWORD LSHIFT = sizeof(ULONG_PTR) * 8 - 1;
	DWORD bitSetCount = 0;
	ULONG_PTR bitTest = (ULONG_PTR)1 << LSHIFT;
	DWORD i;

	for (i = 0; i <= LSHIFT; ++i)
	{
		bitSetCount += ((bitMask & bitTest) ? 1 : 0);
		bitTest /= 2;
	}

	return bitSetCount;
}
