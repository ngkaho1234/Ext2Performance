#include <windows.h>
#include <stdio.h>
#include "common.h"

CHAR *
PerfStatStrings[] = {
    "IRP_CONTEXT",
    "VCB",
    "FCB",
    "CCB",
    "MCB",
    "EXTENT",
    "RW_CONTEXT",
    "VPB",
    "FCB_NAME",
    "MCB_NAME",
    "FILE_NAME",
    "DIR_ENTRY",
    "DIR_PATTERN",
    "DISK_EVENT",
    "DISK_BUFFER",
    "BLOCK_DATA",
    "inode",
    "dentry",
    "buffer head",
    "extents buffer",
    NULL
};

void cls( HANDLE hConsole )
{
   COORD coordScreen = { 0, 0 };    // home for the cursor 
   DWORD cCharsWritten;
   CONSOLE_SCREEN_BUFFER_INFO csbi; 
   DWORD dwConSize;

// Get the number of character cells in the current buffer. 

   if( !GetConsoleScreenBufferInfo( hConsole, &csbi ))
   {
      return;
   }

   dwConSize = csbi.dwSize.X * csbi.dwSize.Y;

   // Fill the entire screen with blanks.

   if( !FillConsoleOutputCharacter( hConsole,        // Handle to console screen buffer 
                                    (TCHAR) ' ',     // Character to write to the buffer
                                    dwConSize,       // Number of cells to write 
                                    coordScreen,     // Coordinates of first cell 
                                    &cCharsWritten ))// Receive number of characters written
   {
      return;
   }

   // Get the current text attribute.

   if( !GetConsoleScreenBufferInfo( hConsole, &csbi ))
   {
      return;
   }

   // Set the buffer's attributes accordingly.

   if( !FillConsoleOutputAttribute( hConsole,         // Handle to console screen buffer 
                                    csbi.wAttributes, // Character attributes to use
                                    dwConSize,        // Number of cells to set attribute 
                                    coordScreen,      // Coordinates of first cell 
                                    &cCharsWritten )) // Receive number of characters written
   {
      return;
   }

   // Put the cursor at its home coordinates.

   SetConsoleCursorPosition( hConsole, coordScreen );
}

BOOL
Ext2QueryPerfStat (
    HANDLE                      Handle,
    PEXT2_QUERY_PERFSTAT        Stat,
    PEXT2_PERF_STATISTICS_V1   *PerfV1,
    PEXT2_PERF_STATISTICS_V2   *PerfV2
)
{
    BOOL                status;
    DWORD lpBytesReturned;

    memset(Stat, 0, sizeof(EXT2_QUERY_PERFSTAT));
    Stat->Magic = EXT2_VOLUME_PROPERTY_MAGIC;
    Stat->Command = IOCTL_APP_QUERY_PERFSTAT;

    *PerfV1 = NULL;
    *PerfV2 = NULL;

    status = DeviceIoControl(
                 Handle,
                 IOCTL_APP_QUERY_PERFSTAT,
                 Stat, sizeof(EXT2_QUERY_PERFSTAT),
                 Stat, sizeof(EXT2_QUERY_PERFSTAT),
                 &lpBytesReturned,
                 NULL
             );
    if (!status)
        return FALSE;

    if (lpBytesReturned == EXT2_QUERY_PERFSTAT_SZV2 &&
            (Stat->Flags & EXT2_QUERY_PERFSTAT_VER2) != 0) {

        if (Stat->PerfStatV2.Magic == EXT2_PERF_STAT_MAGIC &&
                Stat->PerfStatV2.Length == sizeof(EXT2_PERF_STATISTICS_V2) &&
                Stat->PerfStatV2.Version == EXT2_PERF_STAT_VER2) {
            *PerfV2 = &Stat->PerfStatV2;
        }

    } else if (lpBytesReturned >= EXT2_QUERY_PERFSTAT_SZV1)  {

        *PerfV1 = &Stat->PerfStatV1;
    }

    if (PerfV1 || PerfV2)
        return TRUE;

    return FALSE;
}

HANDLE OpenExt2(void)
{
    return CreateFile("\\\\.\\Ext2Fsd", GENERIC_READ, FILE_SHARE_READ,
                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}

int main(void)
{
    HANDLE Ext2Drive;
    HANDLE hStdout;
    static EXT2_QUERY_PERFSTAT Stat;
    static PEXT2_PERF_STATISTICS_V1 Statv1;
    static PEXT2_PERF_STATISTICS_V2 Statv2;
    int StringTableNumber = sizeof(PerfStatStrings) / sizeof(char *);

    Ext2Drive = OpenExt2();
    hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    
    if (!Ext2Drive || !hStdout) {
        printf("Failed!\n");
        ExitProcess(1);
    }
	while (1) {
		int i;
		cls(hStdout);
		Ext2QueryPerfStat(Ext2Drive, &Stat, &Statv1, &Statv2);
		for (i = 0; i < StringTableNumber; i++) {
			if (PerfStatStrings[i] != NULL)
				printf("%s: Size: %u, Count = %u, Accumulated: %u\n", PerfStatStrings[i],
						Statv2->Unit.Slot[i],
						Statv2->Current.Slot[i],
						Statv2->Total.Slot[i]);
		}
		Sleep(2000);
	}
}