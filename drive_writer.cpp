#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#endif

#include <windows.h>
#include <cstdio>
#include <crtdbg.h>

#define PAGES_TO_ALLOCATE 1

typedef unsigned __int64 QWORD;

BOOL CreateUSBHandle(HANDLE* handle);
BOOL DetermineSectorSize(HANDLE handle, UINT64* sectorSize);

#pragma pack(show)
#pragma pack(push, 1)
#pragma pack(show)
typedef struct BiosParameterBlock {

	BYTE jumpOpcode[3];
	BYTE oemIdentifier[8];
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

typedef struct MasterBootRecord {

	DWORD diskID;
	WORD reserved;
	PARTITION partitions[4];

} MBR;
#pragma pack(pop)

int wmain(int argc, wchar_t** argv, wchar_t** envp) { 

	HANDLE consoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	HANDLE consoleInput = GetStdHandle(STD_INPUT_HANDLE);
	WCHAR* buffer = new WCHAR[512];
	WCHAR* inputBuffer = new WCHAR[512];
	INT charactersWritten = 0;

	HANDLE usbHandle = nullptr;
	BOOL createUSBHandleError = CreateUSBHandle(&usbHandle);

	if (createUSBHandleError == FALSE) {

		charactersWritten = swprintf_s(buffer, 512, L"\nFailed to create physical disk handle for removable media.\n");
		WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
		OutputDebugString(buffer);

		delete[] inputBuffer;
		delete[] buffer;
		return -1;

	}

	UINT64 sectorSize = 0;
	BOOL determineSectorSizeError = DetermineSectorSize(usbHandle, &sectorSize);

	if (determineSectorSizeError == FALSE) {

		charactersWritten = swprintf_s(buffer, 512, L"\nFailed to ascertain sector size of removable media.\n");
		WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
		OutputDebugString(buffer);

		CloseHandle(usbHandle);
		delete[] inputBuffer;
		delete[] buffer;
		return -1;

	}

	SYSTEM_INFO sysInfo = {};
	GetSystemInfo(&sysInfo);

	if (sysInfo.dwPageSize % sectorSize != 0) {

		charactersWritten = swprintf_s(buffer, 500, L"\nSystem page size not divisible by sector size.\n");
		WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
		OutputDebugString(buffer);

		CloseHandle(usbHandle);
		delete[] inputBuffer;
		delete[] buffer;
		return -1;

	}

	DWORD charactersRead = 0;
	CONSOLE_READCONSOLE_CONTROL consoleControl = {};
	consoleControl.nLength = sizeof(CONSOLE_READCONSOLE_CONTROL);
	consoleControl.nInitialChars = 0;
	consoleControl.dwCtrlWakeupMask = 0x01 << 0x0A;
	consoleControl.dwControlKeyState = NULL;

	charactersWritten = swprintf_s(buffer, 512, L"\nFile path of new code section of MBR (default - \"./bootloader\"): ");
	WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
	ReadConsole(consoleInput, inputBuffer, 512, &charactersRead, &consoleControl);

	if (charactersRead == 2) {

		charactersWritten = swprintf_s(buffer, 512, L"\nOpening file ./bootloader\n");
		swprintf_s(inputBuffer, 512, L"./bootloader");
		WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);

	}
	else {

		*(inputBuffer + charactersRead - 2) = L'\0';
		charactersWritten = swprintf_s(buffer, 512, L"\nOpening file: %s\n", inputBuffer);
		WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);

	}

	HANDLE mbrCodeFile = nullptr;
	mbrCodeFile = CreateFile(inputBuffer, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (mbrCodeFile == INVALID_HANDLE_VALUE) {

		charactersWritten = swprintf_s(buffer, 512, L"\nFailed to open mbr code file.\n");
		WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
		OutputDebugString(buffer);

		CloseHandle(usbHandle);
		delete[] inputBuffer;
		delete[] buffer;
		return -1;

	}

	DWORD fileSizeHighBytes = 0;
	DWORD fileSizeLowBytes = GetFileSize(mbrCodeFile, &fileSizeHighBytes);

	if (fileSizeLowBytes == 0) {

		charactersWritten = swprintf_s(buffer, 512, L"\nFailed to ascertain size of mbr code file.\n");
		WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
		OutputDebugString(buffer);

		CloseHandle(mbrCodeFile);
		CloseHandle(usbHandle);
		delete[] inputBuffer;
		delete[] buffer;
		return -1;

	}

	QWORD mbrCodeSize = (QWORD)fileSizeLowBytes + ((QWORD)fileSizeHighBytes << 32);
	if ((mbrCodeSize != 512) && (mbrCodeSize != 420) && (mbrCodeSize != 350)) {

		charactersWritten = swprintf_s(buffer, 512, L"\nSize of mbr code file is not 350, 420 or 512 bytes: %I64d.\n", mbrCodeSize);
		WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
		OutputDebugString(buffer);

		CloseHandle(mbrCodeFile);
		CloseHandle(usbHandle);
		delete[] inputBuffer;
		delete[] buffer;
		return -1;

	}

	charactersWritten = swprintf_s(buffer, 512, L"\nSize of mbr code file: %I64d.\n", mbrCodeSize);
	WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);

	BYTE* mbrCode = new BYTE[512];
	DWORD bytesRead = 0;
	BOOL readFileError = ReadFile(mbrCodeFile, mbrCode, mbrCodeSize, &bytesRead, NULL);

	if (readFileError == FALSE) {

		charactersWritten = swprintf_s(buffer, 512, L"\nFailed to read mbr code from file.\n");
		WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
		OutputDebugString(buffer);

		
		CloseHandle(mbrCodeFile);
		CloseHandle(usbHandle);
		delete[] mbrCode;
		delete[] inputBuffer;
		delete[] buffer;
		return -1;

	}

	WORD bootSignature = 0xAA55;
	BIOS_PARAMETER_BLOCK bpb = {};
	FAT32_EXT_BOOT_RECORD ebr = {};
	memset(&bpb, 0, sizeof(BIOS_PARAMETER_BLOCK));
	memset(&ebr, 0, sizeof(FAT32_EXT_BOOT_RECORD));

	bpb.jumpOpcode[0] = 0xEB;
	bpb.jumpOpcode[1] = 0x5A;
	bpb.jumpOpcode[2] = 0x90;
	CONST CHAR* oemIdentifier = "MSDOS5.1";
	memcpy(&bpb.oemIdentifier, oemIdentifier, 8);
	bpb.bytesPerSector = 512;
	CONST CHAR* volumeLabel = "NO NAME    ";
	memcpy(&ebr.volumeLabel, volumeLabel, 11);
	CONST CHAR* systemIdentifier = "FAT32   ";
	memcpy(&ebr.systemIdentifer, systemIdentifier, 8);

	LPVOID alignedBuffer = NULL;
	alignedBuffer = VirtualAlloc(NULL, PAGES_TO_ALLOCATE * sysInfo.dwPageSize, MEM_COMMIT, PAGE_READWRITE);

	if (alignedBuffer == NULL) {

		charactersWritten = swprintf_s(buffer, 512, L"\nFailed to allocate memory using VirtualAlloc.\n");
		WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
		OutputDebugString(buffer);

		CloseHandle(mbrCodeFile);
		CloseHandle(usbHandle);
		delete[] mbrCode;
		delete[] inputBuffer;
		delete[] buffer;
		return -1;

	}

	SetFilePointer(usbHandle, 0, NULL, FILE_BEGIN);
	readFileError = ReadFile(usbHandle, alignedBuffer, sectorSize, &bytesRead, NULL);

	if (readFileError == FALSE) {

		charactersWritten = swprintf_s(buffer, 512, L"\nFailed to read first sector of removable media.\n");
		WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
		OutputDebugString(buffer);

		DWORD lastError = GetLastError();

		VirtualFree(alignedBuffer, 0, MEM_RELEASE);
		CloseHandle(mbrCodeFile);
		CloseHandle(usbHandle);
		delete[] mbrCode;
		delete[] inputBuffer;
		delete[] buffer;
		return -1;

	}

	DWORD bytesWritten = 0;
	HANDLE originalDumpFile = nullptr;
	originalDumpFile = CreateFile(L"./original_first_sector_dump", GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	WriteFile(originalDumpFile, alignedBuffer, sectorSize, &bytesWritten, NULL);
	CloseHandle(originalDumpFile);

	BIOS_PARAMETER_BLOCK originalBpb = {};
	FAT32_EXT_BOOT_RECORD originalEbr = {};
	MBR originalMbr = {};
	memcpy(&originalBpb, alignedBuffer, sizeof(BIOS_PARAMETER_BLOCK));
	memcpy(&originalEbr, (BYTE*)alignedBuffer + sizeof(BIOS_PARAMETER_BLOCK), sizeof(FAT32_EXT_BOOT_RECORD));
	memcpy(&originalMbr, (BYTE*)alignedBuffer + 0x1B8, sizeof(MBR));

	if (mbrCodeSize == 420) {

		memcpy((BYTE*)alignedBuffer, &bpb, sizeof(BIOS_PARAMETER_BLOCK));
		memcpy((BYTE*)alignedBuffer + sizeof(BIOS_PARAMETER_BLOCK), &ebr, sizeof(FAT32_EXT_BOOT_RECORD));
		memcpy((BYTE*)alignedBuffer + sizeof(BIOS_PARAMETER_BLOCK) + sizeof(FAT32_EXT_BOOT_RECORD), mbrCode, mbrCodeSize);
		memcpy((BYTE*)alignedBuffer + sizeof(BIOS_PARAMETER_BLOCK) + sizeof(FAT32_EXT_BOOT_RECORD) + mbrCodeSize, &bootSignature, 2);
	
	}
	else if (mbrCodeSize == 350) {

		memcpy((BYTE*)alignedBuffer, &bpb, sizeof(BIOS_PARAMETER_BLOCK));
		memcpy((BYTE*)alignedBuffer + sizeof(BIOS_PARAMETER_BLOCK), &ebr, sizeof(FAT32_EXT_BOOT_RECORD));
		memcpy((BYTE*)alignedBuffer + sizeof(BIOS_PARAMETER_BLOCK) + sizeof(FAT32_EXT_BOOT_RECORD), mbrCode, mbrCodeSize);
		memcpy((BYTE*)alignedBuffer + 0x1B8, &originalMbr, sizeof(MBR));
		memcpy((BYTE*)alignedBuffer + 0x1FE, &bootSignature, 2);

	}
	else if (mbrCodeSize == 512) {

		memcpy((BYTE*)alignedBuffer, mbrCode, mbrCodeSize);

	}
	else {

		charactersWritten = swprintf_s(buffer, 512, L"\nFilesize error.\n");
		WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
		OutputDebugString(buffer);

		VirtualFree(alignedBuffer, 0, MEM_RELEASE);
		CloseHandle(mbrCodeFile);
		CloseHandle(usbHandle);
		delete[] mbrCode;
		delete[] inputBuffer;
		delete[] buffer;
		return -1;

	}

	bytesWritten = 0;
	BOOL writeFileError = FALSE;
	SetFilePointer(usbHandle, 0, NULL, FILE_BEGIN);
	writeFileError = WriteFile(usbHandle, alignedBuffer, sectorSize, &bytesWritten, NULL);

	if (writeFileError == FALSE) {

		charactersWritten = swprintf_s(buffer, 512, L"\nFailed to write mbr code to first sector.\n");
		WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
		OutputDebugString(buffer);

		DWORD lastError = GetLastError();

		VirtualFree(alignedBuffer, 0, MEM_RELEASE);
		CloseHandle(mbrCodeFile);
		CloseHandle(usbHandle);
		delete[] mbrCode;
		delete[] inputBuffer;
		delete[] buffer;
		return -1;

	}

	charactersWritten = swprintf_s(buffer, 512, L"\nFirst sector overwritten successfully.\n");
	WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);

	charactersWritten = swprintf_s(buffer, 512, L"\nFile path of additional file to write (default - \"./secondstage\"): ");
	WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
	ReadConsole(consoleInput, inputBuffer, 512, &charactersRead, &consoleControl);

	if (charactersRead == 2) {

		charactersWritten = swprintf_s(buffer, 512, L"\nOpening additional file ./secondstage\n");
		swprintf_s(inputBuffer, 512, L"./secondstage");
		WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);

	}
	else {

		*(inputBuffer + charactersRead - 2) = L'\0';
		charactersWritten = swprintf_s(buffer, 512, L"\nOpening additional file: %s\n", inputBuffer);
		WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);

	}

	HANDLE additionalFile = nullptr;
	additionalFile = CreateFile(inputBuffer, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (additionalFile == INVALID_HANDLE_VALUE) {

		charactersWritten = swprintf_s(buffer, 512, L"\nFailed to open additional file.\n");
		WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
		OutputDebugString(buffer);

		VirtualFree(alignedBuffer, 0, MEM_RELEASE);
		CloseHandle(mbrCodeFile);
		CloseHandle(usbHandle);
		delete[] mbrCode;
		delete[] inputBuffer;
		delete[] buffer;
		return -1;

	}

	fileSizeHighBytes = 0;
	fileSizeLowBytes = GetFileSize(additionalFile, &fileSizeHighBytes);

	if (fileSizeLowBytes == 0) {

		charactersWritten = swprintf_s(buffer, 512, L"\nFailed to ascertain size of additional file.\n");
		WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
		OutputDebugString(buffer);

		VirtualFree(alignedBuffer, 0, MEM_RELEASE);
		CloseHandle(additionalFile);
		CloseHandle(mbrCodeFile);
		CloseHandle(usbHandle);
		delete[] mbrCode;
		delete[] inputBuffer;
		delete[] buffer;
		return -1;

	}

	QWORD additionalFileSize = (QWORD)fileSizeLowBytes + ((QWORD)fileSizeHighBytes << 32);
	if (additionalFileSize == 0) {

		charactersWritten = swprintf_s(buffer, 512, L"\nSize of additional file is 0.\n");
		WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
		OutputDebugString(buffer);

		VirtualFree(alignedBuffer, 0, MEM_RELEASE);
		CloseHandle(additionalFile);
		CloseHandle(mbrCodeFile);
		CloseHandle(usbHandle);
		delete[] mbrCode;
		delete[] inputBuffer;
		delete[] buffer;
		return -1;

	}

	charactersWritten = swprintf_s(buffer, 512, L"\nSize of additional file: %I64d.\n", additionalFileSize);
	WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);

	if (PAGES_TO_ALLOCATE * sysInfo.dwPageSize < additionalFileSize) {

		charactersWritten = swprintf_s(buffer, 512, L"\nSize of additional file exceeds allocated memory.\n");
		WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
		OutputDebugString(buffer);

		VirtualFree(alignedBuffer, 0, MEM_RELEASE);
		CloseHandle(additionalFile);
		CloseHandle(mbrCodeFile);
		CloseHandle(usbHandle);
		delete[] mbrCode;
		delete[] inputBuffer;
		delete[] buffer;
		return -1;

	}

	readFileError = ReadFile(additionalFile, alignedBuffer, additionalFileSize, &bytesRead, NULL);

	if (readFileError == FALSE) {

		charactersWritten = swprintf_s(buffer, 512, L"\nFailed to read bytes from additional file.\n");
		WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
		OutputDebugString(buffer);

		VirtualFree(alignedBuffer, 0, MEM_RELEASE);
		CloseHandle(additionalFile);
		CloseHandle(mbrCodeFile);
		CloseHandle(usbHandle);
		delete[] mbrCode;
		delete[] inputBuffer;
		delete[] buffer;
		return -1;

	}

	DWORD bytesToWriteAdditional = 0;
	if ((additionalFileSize % sectorSize) == 0) bytesToWriteAdditional = additionalFileSize;
	else bytesToWriteAdditional = ((additionalFileSize / sectorSize) + 1) * sectorSize;

	writeFileError = WriteFile(usbHandle, alignedBuffer, bytesToWriteAdditional, &bytesWritten, NULL);

	if (writeFileError == FALSE) {

		charactersWritten = swprintf_s(buffer, 512, L"\nFailed to write additional file to second sector and onwards.\n");
		WriteConsole(consoleOutput, buffer, charactersWritten, NULL, NULL);
		OutputDebugString(buffer);

		DWORD lastError = GetLastError();

		VirtualFree(alignedBuffer, 0, MEM_RELEASE);
		CloseHandle(additionalFile);
		CloseHandle(mbrCodeFile);
		CloseHandle(usbHandle);
		delete[] mbrCode;
		delete[] inputBuffer;
		delete[] buffer;
		return -1;

	}

	SetFilePointer(usbHandle, 0, NULL, FILE_BEGIN);
	ReadFile(usbHandle, alignedBuffer, sectorSize * 2, &bytesRead, NULL);
	HANDLE dumpFile = nullptr;
	dumpFile = CreateFile(L"./first_sector_dump", GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	WriteFile(dumpFile, alignedBuffer, sectorSize * 2, &bytesWritten, NULL);
	CloseHandle(dumpFile);

	VirtualFree(alignedBuffer, 0, MEM_RELEASE);
	CloseHandle(additionalFile);
	CloseHandle(mbrCodeFile);
	CloseHandle(usbHandle);
	delete[] mbrCode;
	delete[] inputBuffer;
	delete[] buffer;

#ifdef _DEBUG
	::_CrtDumpMemoryLeaks();
#endif

	return 0;

}

//Return a physical device handle for removable media
BOOL CreateUSBHandle(HANDLE * handle) {

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

