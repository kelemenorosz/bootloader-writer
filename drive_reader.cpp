#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#endif

#include <windows.h>
#include <cstdio>
#include <crtdbg.h>

typedef unsigned __int64 QWORD;
constexpr auto PAGES_TO_ALLOCATE = 1;

typedef struct Partition {

	BYTE bootFlag;
	BYTE startHead;
	WORD startSectorCylinder;
	BYTE systemID;
	BYTE endHead;
	WORD endSectorCylinder;
	DWORD startLBA;
	DWORD totalSectors;

} PARTITION;

#pragma pack(push, 1)
typedef struct BiosParameterBlock {

	BYTE jumpOpcode[3];
	QWORD oemIdentifier;
	WORD bytesPerSector;
	BYTE sectorsPerCluster;
	WORD reservedSectors;
	BYTE numberOfFATs;
	WORD numberOfRootDirEntries;
	WORD smallSectorCount;
	BYTE mediaDescriptor;
	WORD sectorsPerFAT;
	WORD sectorsPerTrack;
	WORD numberOfHeads;
	DWORD hiddenSectors;
	DWORD largeSectorCount;


} BIOS_PARAMETER_BLOCK;

typedef struct ExtendedBootRecord {

	DWORD sectorsPerFAT;
	WORD flags;
	WORD FATVersion;
	DWORD rootDirCluster;
	WORD FSInfoSector;
	WORD backupBootSectorSector;
	BYTE reserved[12];
	BYTE driveNumber;
	BYTE flagsWinNT;
	BYTE signature;
	DWORD volumeIDSerialNumber;
	BYTE volumeLabel[11];
	BYTE systemIdentifer[8];

} FAT32_EXT_BOOT_RECORD;

typedef struct FSInfo {

	DWORD leadSignature;
	BYTE reserved[480];
	DWORD signature;
	DWORD freeClusterCount;
	DWORD availableCluster;
	BYTE reserved2[12];
	DWORD trailSignature;

} FAT32_FSINFO;

typedef struct FileMetadata {

	BYTE filename[11];
	BYTE attributes;
	BYTE reserved;
	BYTE creationTimeHundredthSeconds;
	WORD creationTime;
	WORD creationDate;
	WORD lastAccessedDate;
	WORD firstClusterHighBits;
	WORD lastModifyTime;
	WORD lastModifyDate;
	WORD firstClusterLowBits;
	DWORD fileSizeInBytes;

} FAT32_DIR_ENTRY;

typedef struct FileMetadataExtension {

	BYTE entryOrder;
	WORD filenameFirstFive[5];
	BYTE attributes;
	BYTE type;
	BYTE checksum;
	WORD filenameSecondSix[6];
	WORD fileSizeInBytes;
	WORD filenameLastTwo[2];

} FAT32_DIR_ENTRY_EXT;
#pragma pack(pop)

typedef struct FileMetadata_simplified {

	WORD filename[256];
	DWORD firstCluster;

} FAT32_DIR_ENTRY_S;

typedef struct FileMetadata_simplifiedList {

	FileMetadata_simplifiedList* nextEntry;
	BYTE fileType;
	DWORD fileSizeInBytes;
	WORD filename[256];
	DWORD firstCluster;
	BYTE ext;

} FAT32_DIR_ENTRY_S_LIST;

typedef struct DirectoryEntries {

	DWORD count;
	FAT32_DIR_ENTRY_S entries[4096];

} FAT32_DIR;

typedef struct DirectoryEntriesList {

	DWORD count;
	FAT32_DIR_ENTRY_S_LIST* head;
	FAT32_DIR_ENTRY_S_LIST entries[4096];

} FAT32_DIR_LIST;

BOOL CreateUSBHandle(HANDLE* handle);
BOOL DetermineSectorSize(HANDLE handle, UINT64* sectorSize);
BOOL ReadDisk(HANDLE handle, LPVOID alignedBuffer, SIZE_T bufferSize, UINT64 sectorSize);
FAT32_DIR_LIST* ReadDirectory(LPVOID alignedBuffer, SIZE_T clusterSize, SIZE_T sectorSize, BYTE* fat, SIZE_T fatSize);
VOID ParseDirectory(HANDLE handle, LPVOID alignedBuffer, SIZE_T clusterSize, SIZE_T sectorSize, BYTE* fat, SIZE_T fatSize, DWORD clusterOffset, DWORD firstCluster, WCHAR* prefix);
BOOL RewriteMBR(HANDLE handle, LPVOID alignedBuffer);

int wmain(int argc, wchar_t** argv, wchar_t** envp) {

	HANDLE consoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	HANDLE consoleInput = GetStdHandle(STD_INPUT_HANDLE);
	WCHAR* buffer = new WCHAR[500];
	INT charactersWritten = 0;

	HANDLE usbHandle = NULL;
	BOOL createHandleError = CreateUSBHandle(&usbHandle);

	if (createHandleError == FALSE) {
	
		charactersWritten = swprintf_s(buffer, 500, L"\nFailed to create physical drive handle.\n");
		WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
		OutputDebugString(buffer);

	}
	else {
	
		charactersWritten = swprintf_s(buffer, 500, L"\nPhysical drive handle created successfully.\n");
		WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
		OutputDebugString(buffer);

		UINT64 sectorSize = 0;
		BOOL determineSectorSizeError = DetermineSectorSize(usbHandle, &sectorSize);

		if (determineSectorSizeError == FALSE) {
		
			charactersWritten = swprintf_s(buffer, 500, L"\nFailed to determine disk sector size.\n");
			WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
			OutputDebugString(buffer);

		}
		else {
		
			charactersWritten = swprintf_s(buffer, 500, L"\nDisk sector size: %I64d.\n", sectorSize);
			WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
			OutputDebugString(buffer);

			SYSTEM_INFO sysInfo = {};
			GetSystemInfo(&sysInfo);
			charactersWritten = swprintf_s(buffer, 500, L"\nSystem page size: %d.\n", sysInfo.dwPageSize);
			WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
			OutputDebugString(buffer);

			if (sysInfo.dwPageSize % sectorSize != 0) {
			
				charactersWritten = swprintf_s(buffer, 500, L"\nSystem page size not divisible by sector size.\n");
				WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
				OutputDebugString(buffer);
			
			}
			else {

				DWORD charactersRead = 0;
				CONSOLE_READCONSOLE_CONTROL consoleControl = {};
				consoleControl.nLength = sizeof(CONSOLE_READCONSOLE_CONTROL);
				consoleControl.nInitialChars = 0;
				consoleControl.dwCtrlWakeupMask = 0x01 << 0x0A;
				consoleControl.dwControlKeyState = NULL;

				charactersWritten = swprintf_s(buffer, 500, L"\nSystem pages to allocate (default - %d): ", PAGES_TO_ALLOCATE);
				WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
				ReadConsole(consoleInput, buffer, 500, &charactersRead, &consoleControl);
				
				SIZE_T pagesToAllocate = PAGES_TO_ALLOCATE;

				if (charactersRead > 2) {

					*(buffer + charactersRead - 2) = 0x00;
					INT convertedString = _wtoi(buffer);
					if (convertedString > 0) pagesToAllocate = convertedString;
				
				}

				LPVOID alignedBuffer = NULL;
				alignedBuffer = VirtualAlloc(NULL, pagesToAllocate * sysInfo.dwPageSize, MEM_COMMIT, PAGE_READWRITE);

				if (alignedBuffer == NULL) {
				
					charactersWritten = swprintf_s(buffer, 500, L"\nFailed to allocate memory using VirtualAlloc.\n");
					WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
					OutputDebugString(buffer);
				
				}
				else {
								
					charactersWritten = swprintf_s(buffer, 500, L"\nAllocated %zd pages of memory.\n", pagesToAllocate);
					WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
					OutputDebugString(buffer);

					BOOL readDiskError = ReadDisk(usbHandle, alignedBuffer, pagesToAllocate * sysInfo.dwPageSize, sectorSize);
					
					if (readDiskError == FALSE) {

						charactersWritten = swprintf_s(buffer, 500, L"\nFailed to read disk.\n");
						WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
						OutputDebugString(buffer);

					}

					VirtualFree(alignedBuffer, 0, MEM_RELEASE);
				
				}

			}

		}

		CloseHandle(usbHandle);
	
	}

	delete[] buffer;

#ifdef _DEBUG
	::_CrtDumpMemoryLeaks();
#endif

	return 0;

}

//Return a physical device handle for removable media
BOOL CreateUSBHandle(HANDLE* handle) {

	WCHAR* buffer = new WCHAR[500];

	DWORD logicalDrivesBitmask = 0x00;
	logicalDrivesBitmask = ::GetLogicalDrives();

	if (logicalDrivesBitmask == 0) {
	
		swprintf_s(buffer, 500, L"Failed to retrieve logical drives bitmask.\n");
		OutputDebugString(buffer);

		delete[] buffer;
		return FALSE;

	}

	HANDLE consoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	HANDLE consoleInput = GetStdHandle(STD_INPUT_HANDLE);
	
	WCHAR* nameBuffer = new WCHAR[500];
	BYTE bitmask = 0x01;
	BYTE driveExists = 0x00;
	UINT driveType = 0;
	BOOL usbFound = 0;
	INT charactersWritten = 0;

	charactersWritten = swprintf_s(buffer, 500, L"Available removable media: \n\n");
	WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);

	for (UINT8 i = 0; i < 32; i++) {

		driveExists = logicalDrivesBitmask & bitmask;
		if (driveExists) {

			swprintf_s(nameBuffer, 500, L"%c:\\\\", i + 0x41);
			driveType = GetDriveType(nameBuffer);
			
			if (driveType == 2) {

				usbFound = 1;
				charactersWritten = swprintf_s(buffer, 500, L"\t - Drive %c:", i + 0x41);
				WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);

			}

			swprintf_s(buffer, 500, L"Drive %c: %d. Type: %d.\n", i + 0x41, driveExists, driveType);

		}
		else {

			swprintf_s(buffer, 500, L"Drive %c: %d\n", i + 0x41, driveExists);

		}

		OutputDebugString(buffer);
		logicalDrivesBitmask = logicalDrivesBitmask >> 1;

	}

	if (usbFound == 0) {
	
		charactersWritten = swprintf_s(buffer, 500, L"\tNo removable media found.\n");
		WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
		OutputDebugString(buffer);

		delete[] buffer;
		delete[] nameBuffer;

		return FALSE;

	}

	DWORD charactersRead = 0;
	CONSOLE_READCONSOLE_CONTROL consoleControl = {};
	consoleControl.nLength = sizeof(CONSOLE_READCONSOLE_CONTROL);
	consoleControl.nInitialChars = 0;
	consoleControl.dwCtrlWakeupMask = 0x01 << 0x0A;
	consoleControl.dwControlKeyState = NULL;

	charactersWritten = swprintf_s(buffer, 500, L"\n\nSelect removable media: ");
	WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
	ReadConsole(consoleInput, buffer, 500, &charactersRead, &consoleControl);

	WCHAR driveLetter = *buffer;
	swprintf_s(nameBuffer, 500, L"%.1s:\\\\", &driveLetter);
	driveType = GetDriveType(nameBuffer);
	
	if (driveType != DRIVE_REMOVABLE) {

		charactersWritten = swprintf_s(buffer, 500, L"\nSelected drive is not removable media.\n");
		WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
		OutputDebugString(buffer);

		delete[] buffer;
		delete[] nameBuffer;
		
		return FALSE;

	}

	HANDLE volumeHandle = NULL;
	
	swprintf_s(nameBuffer, 500, L"\\\\.\\%.1s:", &driveLetter);
	swprintf_s(buffer, 500, L"Creating volume handle for removable media: \\\\.\\%.1s:\ndwDesiredAccess: GENERIC_READ \
		\ndwShareMode: FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE \
		\nlpSecurityAttributes: NULL\ndwCreationDisposition: OPEN_EXISTING \
		\ndwFlagsAndAttributes: FILE_FLAG_NO_BUFFERING \
		\nhTemplateFile: NULL \n", &driveLetter);
	OutputDebugString(buffer);

	volumeHandle = CreateFile(nameBuffer, GENERIC_READ, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
		OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);

	if (volumeHandle == INVALID_HANDLE_VALUE) {
		
		charactersWritten = swprintf_s(buffer, 500, L"\nFailure to create volume handle for removable media.\n");
		WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
		OutputDebugString(buffer);

		delete[] buffer;
		delete[] nameBuffer;

		return FALSE;

	}

	BOOL deviceIOControlReturn = FALSE;
	DISK_GEOMETRY usbDiskGeometry = {};
	DWORD bytesReturned = 0;
	OVERLAPPED usbOverlapped = {};
	VOLUME_DISK_EXTENTS usbVolumeDiskExtents = {};

	deviceIOControlReturn = DeviceIoControl(volumeHandle, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0,
		&usbVolumeDiskExtents, sizeof(VOLUME_DISK_EXTENTS), &bytesReturned, &usbOverlapped);
	
	if (deviceIOControlReturn == FALSE) {

		charactersWritten = swprintf_s(buffer, 500, L"\nDeviceIoControl IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS failed.\n");
		WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
		OutputDebugString(buffer);

		CloseHandle(volumeHandle);
		delete[] buffer;
		delete[] nameBuffer;

		return FALSE;

	}

	if (usbVolumeDiskExtents.NumberOfDiskExtents != 1) {
	
		charactersWritten = swprintf_s(buffer, 500, L"\nDrive letter disk extents exceeds 1.\n");
		WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
		OutputDebugString(buffer);

		CloseHandle(volumeHandle);
		delete[] buffer;
		delete[] nameBuffer;

		return FALSE;

	}

	swprintf_s(nameBuffer, 500, L"\\\\.\\PhysicalDrive%d", usbVolumeDiskExtents.Extents[0].DiskNumber);
	swprintf_s(buffer, 500, L"Creating physical drive handle for removable media: \\\\.\\PhysicalDrive%d \
		\ndwDesiredAccess: GENERIC_READ \
		\ndwShareMode: FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE \
		\nlpSecurityAttributes: NULL\ndwCreationDisposition: OPEN_EXISTING \
		\ndwFlagsAndAttributes: FILE_FLAG_NO_BUFFERING \
		\nhTemplateFile: NULL \n", usbVolumeDiskExtents.Extents[0].DiskNumber);
	OutputDebugString(buffer);

	*handle = CreateFile(nameBuffer, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
		OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);

	if (*handle == INVALID_HANDLE_VALUE) {

		charactersWritten = swprintf_s(buffer, 500, L"\nFailure to create physical drive handle for removable media.\n");
		WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
		OutputDebugString(buffer);

		CloseHandle(volumeHandle);
		delete[] buffer;
		delete[] nameBuffer;

		return FALSE;

	}

	CloseHandle(volumeHandle);
	delete[] buffer;
	delete[] nameBuffer;

	return TRUE;

}

//Determine drive sector size
BOOL DetermineSectorSize(HANDLE handle, UINT64* sectorSize) {

	WCHAR* buffer = new WCHAR[500];
	HANDLE consoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	INT charactersWritten = 0;

	BOOL diskGeometryFailed = FALSE;
	BOOL storageAccessAlignmentFailed = FALSE;

	BOOL deviceIOControlReturn = FALSE;
	DWORD bytesReturned = 0;
	DISK_GEOMETRY diskGeometry = {};
	deviceIOControlReturn = DeviceIoControl(handle, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0,
		&diskGeometry, sizeof(DISK_GEOMETRY), &bytesReturned, NULL);

	if (deviceIOControlReturn == FALSE) {

		charactersWritten = swprintf_s(buffer, 500, L"\nDeviceIoControl IOCTL_DISK_GET_DRIVE_GEOMETRY failed.\n");
		WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
		OutputDebugString(buffer);

		diskGeometryFailed = TRUE;

	}

	STORAGE_PROPERTY_QUERY storagePropertyQuery = {};
	storagePropertyQuery.PropertyId = StorageAccessAlignmentProperty;
	storagePropertyQuery.QueryType = PropertyStandardQuery;
	STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR accessAlignmentDescriptor = {};
					
	deviceIOControlReturn = DeviceIoControl(handle, IOCTL_STORAGE_QUERY_PROPERTY, 
		&storagePropertyQuery, sizeof(STORAGE_PROPERTY_QUERY), &accessAlignmentDescriptor,
		sizeof(STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR), &bytesReturned, NULL);

	if (deviceIOControlReturn == FALSE) {

		charactersWritten = swprintf_s(buffer, 500, L"\nDeviceIoControl IOCTL_STORAGE_QUERY_PROPERTY for StorageAccessAlignmentProperty failed.\n");
		WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
		OutputDebugString(buffer);

		storageAccessAlignmentFailed = TRUE;

	}

	if (storageAccessAlignmentFailed == FALSE) {
	
		*sectorSize = static_cast<UINT64>(accessAlignmentDescriptor.BytesPerPhysicalSector);

		delete[] buffer;
		return TRUE;

	}
	else if (diskGeometryFailed == FALSE) {
	
		*sectorSize = static_cast<UINT64>(diskGeometry.BytesPerSector);

		delete[] buffer;
		return TRUE;

	}
	else {
	
		charactersWritten = swprintf_s(buffer, 500, L"\nUnable to determine sector size.\n");
		WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
		OutputDebugString(buffer);

		delete[] buffer;
		return FALSE;

	}

}

//Read disk
BOOL ReadDisk(HANDLE handle, LPVOID alignedBuffer, SIZE_T bufferSize, UINT64 sectorSize) {

	HANDLE consoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	WCHAR* buffer = new WCHAR[500];
	INT charactersWritten = 0;
	DWORD bytesRead = 0;

	BOOL readFileError = ReadFile(handle, alignedBuffer, static_cast<DWORD>(sectorSize), &bytesRead, NULL);
	
	if (readFileError == FALSE) {
						
		charactersWritten = swprintf_s(buffer, 500, L"\nFailed to read MBR.\n");
		WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
		OutputDebugString(buffer);

		delete[] buffer;
		return FALSE;

	}

	PARTITION partitions[4] = {};
	memcpy(partitions, (BYTE*)alignedBuffer + 0x01BE, sizeof(PARTITION) * 4);

	charactersWritten = swprintf_s(buffer, 500, L"\nMBR: \n \
		\nUnique Disk ID - 0x%.8X \
		\nReserved - 0x%.4X \
		\n", *(DWORD*)((BYTE*)alignedBuffer + 0x1B8), *(WORD*)((BYTE*)alignedBuffer + 0x1BC));
	WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);

	for (INT i = 0; i < 4; ++i) {

		if (partitions[i].systemID == 0x00) {

			charactersWritten = swprintf_s(buffer, 500, L"\nPartition %d - Empty\n", i);

		}
		else {

			charactersWritten = swprintf_s(buffer, 500, L"\nPartition %d - \n \
				\n\tBoot flag - 0x%.2X \
				\n\tStarting head - %d \
				\n\tStarting cylinder - %d \
				\n\tStarting sector - %d \
				\n\tSystem ID - 0x%.2X \
				\n\tEnding head - %d \
				\n\tEnding cylinder - %d \
				\n\tEnding sector - %d \
				\n\tStarting LBA - %d \
				\n\tTotal sectors - %d \
				\n", i, partitions[i].bootFlag, partitions[i].startHead, (partitions[i].startSectorCylinder & 0xFFC0) >> 6,
				partitions[i].startSectorCylinder & 0x3F, partitions[i].systemID, partitions[i].endHead,
				(partitions[i].endSectorCylinder & 0xFFC0) >> 6, partitions[i].endSectorCylinder & 0x3F,
				partitions[i].startLBA, partitions[i].totalSectors);

		}
		WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);

	}

	charactersWritten = swprintf_s(buffer, 500, L"\nBootsector signature - 0x%.4X\n", *(WORD*)((BYTE*)alignedBuffer + 0x1FE));
	WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);

	for (INT i = 0; i < 4; ++i) {

		//FAT32 LBA-mapped
		if (partitions[i].systemID == 0x0C && partitions[i].startLBA > 0) {

			LONG distanceToMove = partitions[i].startLBA * static_cast<LONG>(sectorSize);
			DWORD setFilePointerError = SetFilePointer(handle, distanceToMove, NULL, FILE_BEGIN);

			if (setFilePointerError == INVALID_SET_FILE_POINTER) {
									
				charactersWritten = swprintf_s(buffer, 500, L"\nFailed to set file pointer to first sector of partition %d.\n", i);
				WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
				OutputDebugString(buffer);
				continue;
			}
			
			BOOL readFileError = ReadFile(handle, alignedBuffer, static_cast<DWORD>(sectorSize), &bytesRead, NULL);

			if (readFileError == FALSE) {

				charactersWritten = swprintf_s(buffer, 500, L"\nFailed to read Boot Record of partition %d.\n", i);
				WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
				OutputDebugString(buffer);

				continue;
				
			}

			BIOS_PARAMETER_BLOCK bpb = {};
			memcpy(&bpb, alignedBuffer, sizeof(BIOS_PARAMETER_BLOCK));

			charactersWritten = swprintf_s(buffer, 500, L"\nPartition %d: \n", i);
			WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);

			if (bpb.jumpOpcode[0] != 0xEB && bpb.jumpOpcode[2] != 0x90) {

				charactersWritten = swprintf_s(buffer, 500, L"\nJump opcode bytes 0 - 0xEB, 2 - 0x90 not found.\n");
				WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
				OutputDebugString(buffer);

				continue;

			}

			size_t charsConverted = 0;

			charactersWritten = swprintf_s(buffer, 500, L"\n\tBios Parameter Block (BPB): \n");
			WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
			WCHAR oemIdentifier[9] = {};
			mbstowcs_s(&charsConverted, oemIdentifier, 9, (CHAR*)(&bpb.oemIdentifier), 8);
			charactersWritten = swprintf_s(buffer, 500, L"\
				\n\tJump opcode - 0x%.6X \
				\n\tOEM identifier - %.8s \
				\n\tBytes per sector - %d \
				\n\tSectors per cluster - %d \
				\n\tReserved sectors - %d \
				\n\tNr. of FATs - %d \
				\n\tNr. of root directory entries - %d \
				\n\tSmall sector count - %d \
				\n\tMedia descriptor - 0x%.2X \
				\n\tSectors per FAT - %d \
				\n\tSectors per track - %d \
				\n\tNr. of heads - %d \
				\n\tHidden sectors - %d \
				\n\tLarge sector count - %d \
				\n", *(DWORD*)bpb.jumpOpcode & 0xFFFFFF, oemIdentifier, bpb.bytesPerSector,
				bpb.sectorsPerCluster, bpb.reservedSectors, bpb.numberOfFATs,
				bpb.numberOfRootDirEntries, bpb.smallSectorCount,
				bpb.mediaDescriptor, bpb.sectorsPerFAT, bpb.sectorsPerTrack,
				bpb.numberOfHeads, bpb.hiddenSectors, bpb.largeSectorCount);
			WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);

			FAT32_EXT_BOOT_RECORD ebr = {};
			memcpy(&ebr, (BYTE*)alignedBuffer + 0x24, sizeof(FAT32_EXT_BOOT_RECORD));

			charactersWritten = swprintf_s(buffer, 500, L"\n\tFAT32 Exended Boot Record: \n");
			WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
			WCHAR volumeLabelString[12] = {};
			mbstowcs_s(&charsConverted, volumeLabelString, 12, (CHAR*)(&ebr.volumeLabel), 11);
			WCHAR systemIdentifier[9] = {};
			mbstowcs_s(&charsConverted, systemIdentifier, 9, (CHAR*)(&ebr.systemIdentifer), 8);
			charactersWritten = swprintf_s(buffer, 500, L"\
				\n\tSectors per FAT - %d \
				\n\tFlags - 0x%.2X \
				\n\tFAT version - %d.%d \
				\n\tRoot directory (cluster) - %d \
				\n\tFSInfo (sector) - %d \
				\n\tBackup boot sector (sector) - %d \
				\n\tDrive nr. - 0x%.2X \
				\n\tFlags (Windows NT) - 0x%.2X \
				\n\tSignature - 0x%.2X \
				\n\tVolume ID - 0x%.8X \
				\n\tVolume label - %.11s \
				\n\tSystem identifier - %.8s \n",
				ebr.sectorsPerFAT, ebr.flags, ebr.FATVersion >> 8, ebr.FATVersion & 0xFF,
				ebr.rootDirCluster, ebr.FSInfoSector, ebr.backupBootSectorSector,
				ebr.driveNumber, ebr.flagsWinNT,
				ebr.signature, ebr.volumeIDSerialNumber, volumeLabelString,
				systemIdentifier);
			WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);

			distanceToMove = static_cast<LONG>(sectorSize) * (ebr.FSInfoSector - 1);
			if (distanceToMove > 0) setFilePointerError = SetFilePointer(handle, distanceToMove, NULL, FILE_BEGIN);
			if (setFilePointerError == INVALID_SET_FILE_POINTER) {

				charactersWritten = swprintf_s(buffer, 500, L"\nFailed to set file pointer to sector of FSInfo structure; partition %d.\n", i);
				WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
				OutputDebugString(buffer);
				continue;
			}

			readFileError = ReadFile(handle, alignedBuffer, static_cast<DWORD>(sectorSize), &bytesRead, NULL);

			if (readFileError == FALSE) {

				charactersWritten = swprintf_s(buffer, 500, L"\nFailed to read FSInfo sector of partition %d.\n", i);
				WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
				OutputDebugString(buffer);

				continue;

			}

			FAT32_FSINFO fsinfo = {};
			memcpy(&fsinfo, alignedBuffer, sizeof(FAT32_FSINFO));

			charactersWritten = swprintf_s(buffer, 500, L"\n\tFSInfo: \n");
			WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
			charactersWritten = swprintf_s(buffer, 500, L"\
				\n\tLead signature - 0x%.8X \
				\n\tSignature - 0x%.8X \
				\n\tFree cluster count - %d \
				\n\tAvailable cluster - %d \
				\n\tTrail signature - 0x%.8X\n",
				fsinfo.leadSignature, fsinfo.signature, fsinfo.freeClusterCount, fsinfo.availableCluster, fsinfo.trailSignature);
			WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);

			if (bufferSize < sectorSize * ebr.sectorsPerFAT) {

				charactersWritten = swprintf_s(buffer, 500, L"\n\tAllocated buffer smaller than FAT.\n");
				WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
				OutputDebugString(buffer);

				continue;

			}

			distanceToMove = static_cast<LONG>(sectorSize) * (partitions[i].startLBA + bpb.reservedSectors);
			if (distanceToMove > 0) setFilePointerError = SetFilePointer(handle, distanceToMove, NULL, FILE_BEGIN);
			if (setFilePointerError == INVALID_SET_FILE_POINTER) {

				charactersWritten = swprintf_s(buffer, 500, L"\n\tFailed to set file pointer to FAT.\n");
				WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
				OutputDebugString(buffer);
			
				continue;
			
			}

			readFileError = ReadFile(handle, alignedBuffer, static_cast<DWORD>(sectorSize) * ebr.sectorsPerFAT, &bytesRead, NULL);

			if (readFileError == FALSE) {

				charactersWritten = swprintf_s(buffer, 500, L"\n\tFailed to read FAT.\n");
				WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
				OutputDebugString(buffer);

				continue;

			}

			BYTE* fat = new BYTE[sectorSize * ebr.sectorsPerFAT];
			memcpy(fat, alignedBuffer, sectorSize * ebr.sectorsPerFAT);

			if (ebr.rootDirCluster < 2) {

				charactersWritten = swprintf_s(buffer, 500, L"\n\tRoot directory cluster less than 2.\n");
				WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
				OutputDebugString(buffer);

				delete[] fat;
				continue;

			}

			distanceToMove = static_cast<LONG>(sectorSize) * (partitions[i].startLBA + bpb.reservedSectors + bpb.numberOfFATs * ebr.sectorsPerFAT + (ebr.rootDirCluster - 2) * bpb.sectorsPerCluster);
			if (distanceToMove > 0) setFilePointerError = SetFilePointer(handle, distanceToMove, NULL, FILE_BEGIN);
			if (setFilePointerError == INVALID_SET_FILE_POINTER) {

				charactersWritten = swprintf_s(buffer, 500, L"\n\tFailed to set file pointer to root directory cluster.\n");
				WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
				OutputDebugString(buffer);
				
				delete[] fat;
				continue;
		
			}

			readFileError = ReadFile(handle, alignedBuffer, static_cast<DWORD>(sectorSize) * bpb.sectorsPerCluster, &bytesRead, NULL);

			if (readFileError == FALSE) {

				charactersWritten = swprintf_s(buffer, 500, L"\n\tFailed to read root directory cluster.\n");
				WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
				OutputDebugString(buffer);

				continue;

			}

			HANDLE dumpFile = nullptr;
			DWORD bytesWritten = 0;
			dumpFile = ::CreateFile(L"./fat_dump", GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			::WriteFile(dumpFile, fat, static_cast<DWORD>(sectorSize) * ebr.sectorsPerFAT, &bytesWritten, nullptr);
			CloseHandle(dumpFile);

			charactersWritten = swprintf_s(buffer, 500, L"\n");
			WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);

			WCHAR* prefixBuffer = new WCHAR[4096];
			wcscpy_s(prefixBuffer, 4096, L"/");
			DWORD clusterOffset = static_cast<LONG>(sectorSize) * (partitions[i].startLBA + bpb.reservedSectors + bpb.numberOfFATs * ebr.sectorsPerFAT);
			//ParseDirectory(handle, alignedBuffer, sectorSize * bpb.sectorsPerCluster, sectorSize, fat, sectorSize * ebr.sectorsPerFAT, clusterOffset, 4837, prefixBuffer);
			ParseDirectory(handle, alignedBuffer, sectorSize * bpb.sectorsPerCluster, sectorSize, fat, sectorSize * ebr.sectorsPerFAT, clusterOffset, ebr.rootDirCluster, prefixBuffer);
			delete[] prefixBuffer;

			/*FAT32_DIR_LIST* dirList = nullptr;
			dirList = ReadDirectory(alignedBuffer, sectorSize * bpb.sectorsPerCluster, sectorSize, fat, sectorSize * ebr.sectorsPerFAT);

			FAT32_DIR_ENTRY_S_LIST* entryPointer = dirList->head;
			while (entryPointer != nullptr) {

				charactersWritten = swprintf_s(buffer, 500, L"\tSize: %d\t\t Type: 0x%.2X\t/", entryPointer->fileSizeInBytes, entryPointer->fileType);
				WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
				memcpy(buffer, &entryPointer->filename[0], 128);
				charactersWritten = wcslen(buffer);
				WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
				charactersWritten = swprintf_s(buffer, 500, L"\n");
				WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);

				entryPointer = entryPointer->nextEntry;

			}*/

			distanceToMove = static_cast<LONG>(sectorSize) * 
				(partitions[i].startLBA + bpb.reservedSectors + bpb.numberOfFATs * ebr.sectorsPerFAT 
					+ (908 - 2) * bpb.sectorsPerCluster);
			if (distanceToMove > 0) setFilePointerError = SetFilePointer(handle, distanceToMove, NULL, FILE_BEGIN);
			if (setFilePointerError == INVALID_SET_FILE_POINTER) {

				charactersWritten = swprintf_s(buffer, 500, L"\n\tFailed to set file pointer to dump cluster.\n");
				WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
				OutputDebugString(buffer);

				continue;

			}

			readFileError = ReadFile(handle, alignedBuffer, static_cast<DWORD>(sectorSize) * bpb.sectorsPerCluster, &bytesRead, NULL);

			if (readFileError == FALSE) {

				charactersWritten = swprintf_s(buffer, 500, L"\n\tFailed to read dump cluster.\n");
				WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
				OutputDebugString(buffer);

				continue;

			}

			HANDLE clusterDumpFile = nullptr;
			bytesWritten = 0;
			clusterDumpFile = ::CreateFile(L"./cluster_dump", GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			::WriteFile(clusterDumpFile, alignedBuffer, static_cast<DWORD>(sectorSize) * bpb.sectorsPerCluster, &bytesWritten, nullptr);
			CloseHandle(clusterDumpFile);

			delete[] fat;

		}
		else {

			charactersWritten = swprintf_s(buffer, 500, L"\nPartition %d not FAT32 LBA-mapped.\n", i);
			WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);

		}

	}

	delete[] buffer;
	return TRUE;

}

//Read directory
FAT32_DIR_LIST* ReadDirectory(LPVOID alignedBuffer, SIZE_T clusterSize, SIZE_T sectorSize, BYTE* fat, SIZE_T fatSize) {

	HANDLE consoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	INT charactersWritten = 0;
	BYTE* parsePointer = (BYTE*)alignedBuffer;
	FAT32_DIR_ENTRY* dirEntry = nullptr;
	FAT32_DIR_ENTRY_EXT* dirEntryExt = nullptr;
	WCHAR* buffer = new WCHAR[512];
	WCHAR* filename = new WCHAR[512];
	WCHAR* tempBuffer = new WCHAR[512];
	CHAR* tempAsciiBuffer = new CHAR[512];
	size_t charsConverted = 0;
	*filename = L'\0';
	BOOL previousExtended = FALSE;
	WCHAR* strtokPointer = nullptr;
	WCHAR* strtokContext = nullptr;
	INT strcmpResult = 0;

	FAT32_DIR_LIST* dir = new FAT32_DIR_LIST;
	dir->count = 0;
	dir->head = nullptr;

	while ((parsePointer - (BYTE*)alignedBuffer < clusterSize) && (*(parsePointer + 0x0B) != 0x00)) {

		if (*(parsePointer + 0x0B) == 0x0F) {

			*buffer = '\0';
			dirEntryExt = (FAT32_DIR_ENTRY_EXT*)parsePointer;
			//memcpy(tempBuffer, dirEntryExt->filenameFirstFive, 10);
			wcscpy_s(tempBuffer, 512, (WCHAR*)dirEntryExt->filenameFirstFive);
			*(tempBuffer + 5) = L'\0';
			wcscat_s(buffer, 512, tempBuffer);
			//memcpy(tempBuffer, dirEntryExt->filenameSecondSix, 12);
			if (*(WCHAR*)dirEntryExt->filenameSecondSix != 0xFFFF) {
				wcscpy_s(tempBuffer, 512, (WCHAR*)dirEntryExt->filenameSecondSix);
				*(tempBuffer + 6) = L'\0';
				wcscat_s(buffer, 512, tempBuffer);
			}
			//memcpy(tempBuffer, dirEntryExt->filenameLastTwo, 4);
			if (*(WCHAR*)dirEntryExt->filenameLastTwo!= 0xFFFF) {
				wcscpy_s(tempBuffer, 512, (WCHAR*)dirEntryExt->filenameLastTwo);
				*(tempBuffer + 2) = L'\0';
				wcscat_s(buffer, 512, tempBuffer);
			}
			previousExtended = TRUE;

			wcscat_s(buffer, 512, filename);
			wcscpy_s(filename, 512, buffer);

		}
		else {

			dirEntry = (FAT32_DIR_ENTRY*)parsePointer;

			/*charactersWritten = swprintf_s(buffer, 500, L"Starting cluster: %d\
				\nStarting cluster low bits: 0x%.4X.\
				\nStarting cluster high bits: 0x%.4X.\
				\nFile size: %d\
				\nAttributes: 0x%.2X\n",
				(DWORD)dirEntry->firstClusterLowBits + ((DWORD)dirEntry->firstClusterHighBits << 16),
				dirEntry->firstClusterLowBits, dirEntry->firstClusterHighBits, dirEntry->fileSizeInBytes, dirEntry->attributes);
			WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);*/

			if (previousExtended == FALSE) {

				mbstowcs_s(&charsConverted, tempBuffer, 512, (CHAR*)dirEntry->filename, 8);
				*(tempBuffer + 8) = L'\0';
				strtokPointer = wcstok_s(tempBuffer, L" ", &strtokContext);
				wcscat_s(filename, 512, tempBuffer);
				wcscat_s(filename, 512, L".");
				strtokContext = nullptr;
				mbstowcs_s(&charsConverted, tempBuffer, 512, (CHAR*)dirEntry->filename + 8, 3);
				*(tempBuffer + 3) = L'\0';
				strtokPointer = wcstok_s(tempBuffer, L" ", &strtokContext);
				if (*tempBuffer == L' ') *tempBuffer = L'\0';
				wcscat_s(filename, 512, tempBuffer);
				_wcslwr_s(filename, 512);

				INT j = 0;

			}
			
			//Check if FAT contains starting cluster
			DWORD firstClusterIndex = (DWORD)dirEntry->firstClusterLowBits + ((DWORD)dirEntry->firstClusterHighBits << 16);
			if ((firstClusterIndex != 0) && (*((DWORD*)fat + firstClusterIndex) != 0) && (dirEntry->filename[0] != 0xE5) && (dirEntry->filename[0] != 0x2E)) {
				
				/*charactersWritten = swprintf_s(buffer, 500, L"File on disk.\n");
				WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);*/
				
				FAT32_DIR_ENTRY_S_LIST* entryPointer = dir->entries + dir->count;
				memcpy(entryPointer->filename, filename, 512);
				entryPointer->firstCluster = firstClusterIndex;
				entryPointer->fileType = dirEntry->attributes;
				entryPointer->fileSizeInBytes = dirEntry->fileSizeInBytes;
				entryPointer->ext = previousExtended;

				if (dir->head == nullptr) {

					entryPointer->nextEntry = nullptr;
					dir->head = entryPointer;

				}
				else {

					FAT32_DIR_ENTRY_S_LIST* prevEntry = nullptr;
					FAT32_DIR_ENTRY_S_LIST* currentEntry = dir->head;
					BOOL isInList = FALSE;
					while (currentEntry != nullptr) {

						strcmpResult = wcscmp((WCHAR*)currentEntry->filename, (WCHAR*)entryPointer->filename);
						if (strcmpResult > 0) {

							isInList = TRUE;
							if (prevEntry == nullptr) {

								entryPointer->nextEntry = currentEntry;
								dir->head = entryPointer;
								break;

							}
							else {

								prevEntry->nextEntry = entryPointer;
								entryPointer->nextEntry = currentEntry;
								break;
								
							}

						}
						prevEntry = currentEntry;
						currentEntry = currentEntry->nextEntry;

					}
					if (isInList == FALSE) {

						currentEntry = dir->head;
						while (currentEntry->nextEntry != nullptr) currentEntry = currentEntry->nextEntry;
						currentEntry->nextEntry = entryPointer;
						entryPointer->nextEntry = nullptr;

					}

				}

				dir->count++;

			}
			else {

				/*charactersWritten = swprintf_s(buffer, 500, L"File not found on disk.\n");
				WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);*/

			}

			/*wcscat_s(filename, 500, L"\n\n");
			charactersWritten = static_cast<INT>(wcslen(filename));
			WriteConsole(consoleOutput, filename, charactersWritten, NULL, NULL);
			*/

			*filename = L'\0';

			previousExtended = FALSE;

		}

		parsePointer += 32;

	}

	/*charactersWritten = swprintf_s(buffer, 500, L"\n\n\tRoot:\n");
	WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);

	FAT32_DIR_ENTRY_S_LIST* entryPointer = dir->entries;
	for (INT i = 0; i < dir->count; ++i) {

		charactersWritten = swprintf_s(buffer, 500, L"\t\t");
		WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
		memcpy(buffer, &entryPointer->filename[0], 128);
		charactersWritten = wcslen(buffer);
		WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
		charactersWritten = swprintf_s(buffer, 500, L"\n");
		WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
	
		entryPointer++;

	}*/
	
	delete[] buffer;
	delete[] tempBuffer;
	delete[] tempAsciiBuffer;
	delete[] filename;

	return dir;

}

//Parse directory
VOID ParseDirectory(HANDLE handle, LPVOID alignedBuffer, SIZE_T clusterSize, SIZE_T sectorSize, BYTE* fat, SIZE_T fatSize, DWORD clusterOffset, DWORD firstCluster, WCHAR* prefix) {

	HANDLE consoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	INT charactersWritten = 0;
	WCHAR* buffer = new WCHAR[512];

	DWORD setFilePointerError = SetFilePointer(handle, clusterOffset + clusterSize * (firstCluster - 2), NULL, FILE_BEGIN);
	if (setFilePointerError == INVALID_SET_FILE_POINTER) {

		charactersWritten = swprintf_s(buffer, 512, L"\nFailed to set file pointer to fist cluster of directory.\n");
		WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
		OutputDebugString(buffer);

		delete[] buffer;
		return;

	}

	DWORD bytesRead = 0;
	BOOL readFileError = ReadFile(handle, alignedBuffer, clusterSize, &bytesRead, NULL);
	if (readFileError == FALSE) {

		charactersWritten = swprintf_s(buffer, 512, L"\nFailed to read directory cluster.\n");
		WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
		OutputDebugString(buffer);

		delete[] buffer;
		return;

	}

	FAT32_DIR_LIST* dirList = nullptr;
	dirList = ReadDirectory(alignedBuffer, clusterSize, sectorSize, fat, fatSize);

	INT dirSelf = 0;
	FAT32_DIR_ENTRY_S_LIST* entryPointer = dirList->head;
	while (entryPointer != nullptr) {

		//charactersWritten = swprintf_s(buffer, 500, L"\tSize: %d\t\t Type: 0x%.2X\t First cluster: %d\t\t\t\t %s", entryPointer->fileSizeInBytes, entryPointer->fileType, entryPointer->firstCluster, prefix);
		charactersWritten = swprintf_s(buffer, 512, L"%d\t%d\t%s", entryPointer->ext, entryPointer->firstCluster , prefix);
		WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
		memcpy(buffer, &entryPointer->filename[0], 512);
		charactersWritten = wcslen(buffer);
		WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
		charactersWritten = swprintf_s(buffer, 512, L"\n");
		WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);

		dirSelf = wcscmp((WCHAR*)entryPointer->filename, L"..");
		if (entryPointer->fileType == 0x10 && dirSelf != 0) {

			INT prefixLength = wcslen(prefix);
			wcscat_s(prefix, 4096, (WCHAR*)entryPointer->filename);
			wcscat_s(prefix, 4096, L"/");
			ParseDirectory(handle, alignedBuffer, clusterSize, sectorSize, fat, fatSize, clusterOffset, entryPointer->firstCluster, prefix);
			*(prefix + prefixLength) = L'\0';

		}

		entryPointer = entryPointer->nextEntry;

	}

	delete dirList;
	delete[] buffer;

}
