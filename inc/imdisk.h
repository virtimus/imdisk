/*
    ImDisk Virtual Disk Driver for Windows NT/2000/XP.

    Copyright (C) 2005-2007 Olof Lagerkvist.

    Permission is hereby granted, free of charge, to any person
    obtaining a copy of this software and associated documentation
    files (the "Software"), to deal in the Software without
    restriction, including without limitation the rights to use,
    copy, modify, merge, publish, distribute, sublicense, and/or
    sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following
    conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
    OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef _INC_IMDISK_
#define _INC_IMDISK_

#ifndef __T
#if defined(_NTDDK_) || defined(UNICODE) || defined(_UNICODE)
#define __T(x)  L ## x
#else
#define __T(x)  x
#endif
#endif

#ifndef _T
#define _T(x)   __T(x)
#endif

#define IMDISK_VERSION                 0x0160
#define IMDISK_DRIVER_VERSION          0x0103

#ifndef ZERO_STRUCT
#define ZERO_STRUCT { 0 }
#endif

///
/// The base names for the device objects created in \Device
///
#define IMDISK_DEVICE_DIR_NAME         _T("\\Device")
#define IMDISK_DEVICE_BASE_NAME        IMDISK_DEVICE_DIR_NAME  _T("\\ImDisk")
#define IMDISK_CTL_DEVICE_NAME         IMDISK_DEVICE_BASE_NAME _T("Ctl")

///
/// The symlinks to the device objects created in \DosDevices
///
#define IMDISK_CTL_DOSDEV              _T("ImDiskCtl")
#define IMDISK_CTL_DOSDEV_NAME         _T("\\\\.\\")        IMDISK_CTL_DOSDEV
#define IMDISK_CTL_SYMLINK_NAME        _T("\\DosDevices\\") IMDISK_CTL_DOSDEV

///
/// The driver name and image path
///
#define IMDISK_DRIVER_NAME             _T("ImDisk")
#define IMDISK_DRIVER_PATH             _T("system32\\drivers\\imdisk.sys")

///
/// Registry settings. It is possible to specify devices to be mounted
/// automatically when the driver loads.
///
#define IMDISK_CFG_PARAMETER_KEY                  _T("\\Parameters")
#define IMDISK_CFG_MAX_DEVICES_VALUE              _T("MaxDevices")
#define IMDISK_CFG_LOAD_DEVICES_VALUE             _T("LoadDevices")
#define IMDISK_CFG_DISALLOWED_DRIVE_LETTERS_VALUE _T("DisallowedDriveLetters")
#define IMDISK_CFG_IMAGE_FILE_PREFIX              _T("FileName")
#define IMDISK_CFG_SIZE_PREFIX                    _T("Size")
#define IMDISK_CFG_FLAGS_PREFIX                   _T("Flags")
#define IMDISK_CFG_DRIVE_LETTER_PREFIX            _T("DriveLetter")

///
/// Base value for the IOCTL's.
///
#define FILE_DEVICE_IMDISK             0x8372

#define IOCTL_IMDISK_QUERY_VERSION     ((ULONG) CTL_CODE(FILE_DEVICE_IMDISK, 0x800, METHOD_BUFFERED, 0))
#define IOCTL_IMDISK_CREATE_DEVICE     ((ULONG) CTL_CODE(FILE_DEVICE_IMDISK, 0x801, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS))
#define IOCTL_IMDISK_QUERY_DEVICE      ((ULONG) CTL_CODE(FILE_DEVICE_IMDISK, 0x802, METHOD_BUFFERED, 0))
#define IOCTL_IMDISK_QUERY_DRIVER      ((ULONG) CTL_CODE(FILE_DEVICE_IMDISK, 0x803, METHOD_BUFFERED, 0))
#define IOCTL_IMDISK_REFERENCE_HANDLE  ((ULONG) CTL_CODE(FILE_DEVICE_IMDISK, 0x804, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS))
#define IOCTL_IMDISK_SET_DEVICE_FLAGS  ((ULONG) CTL_CODE(FILE_DEVICE_IMDISK, 0x805, METHOD_BUFFERED, 0))
#define IOCTL_IMDISK_REMOVE_DEVICE     ((ULONG) CTL_CODE(FILE_DEVICE_IMDISK, 0x806, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS))

///
/// Bit constants for the Flags field in IMDISK_CREATE_DATA
///

/// Read-only device
#define IMDISK_OPTION_RO                0x00000001

/// Check if flags specifies read-only
#define IMDISK_READONLY(x)              ((ULONG)(x) & 0x00000001)

/// Removable, hot-plug, device
#define IMDISK_OPTION_REMOVABLE         0x00000002

/// Check if flags specifies removable
#define IMDISK_REMOVABLE(x)             ((ULONG)(x) & 0x00000002)

/// Specifies that image files are created with sparse attribute.
#define IMDISK_OPTION_SPARSE_FILE       0x00000004

/// Check if flags specifies sparse
#define IMDISK_SPARSE_FILE(x)           ((ULONG)(x) & 0x00000004)

/// Device type is virtual harddisk partition
#define IMDISK_DEVICE_TYPE_HD           0x00000010
/// Device type is virtual floppy drive
#define IMDISK_DEVICE_TYPE_FD           0x00000020
/// Device type is virtual CD/DVD-ROM drive
#define IMDISK_DEVICE_TYPE_CD           0x00000030

/// Extracts the IMDISK_DEVICE_TYPE_xxx from flags
#define IMDISK_DEVICE_TYPE(x)           ((ULONG)(x) & 0x000000F0)

/// Virtual disk is backed by image file
#define IMDISK_TYPE_FILE                0x00000100
/// Virtual disk is backed by virtual memory
#define IMDISK_TYPE_VM                  0x00000200
/// Virtual disk is backed by proxy connection
#define IMDISK_TYPE_PROXY               0x00000300

/// Extracts the IMDISK_TYPE_xxx from flags
#define IMDISK_TYPE(x)                  ((ULONG)(x) & 0x00000F00)

/// Proxy connection is direct-type
#define IMDISK_PROXY_TYPE_DIRECT        0x00000000
/// Proxy connection is over serial line
#define IMDISK_PROXY_TYPE_COMM          0x00001000
/// Proxy connection is over TCP/IP
#define IMDISK_PROXY_TYPE_TCP           0x00002000
/// Proxy connection uses shared memory
#define IMDISK_PROXY_TYPE_SHM           0x00003000

/// Extracts the IMDISK_PROXY_TYPE_xxx from flags
#define IMDISK_PROXY_TYPE(x)            ((ULONG)(x) & 0x0000F000)

/// Extracts the IMDISK_PROXY_TYPE_xxx from flags
#define IMDISK_IMAGE_MODIFIED           0x00010000

/// Specify as device number to automatically select first free.
#define IMDISK_AUTO_DEVICE_NUMBER       ((ULONG)-1)

/**
   Structure used by the IOCTL_IMDISK_CREATE_DEVICE and
   IOCTL_IMDISK_QUERY_DEVICE calls and by the ImDiskQueryDevice() function.
*/
typedef struct _IMDISK_CREATE_DATA
{
  /// On create this can be set to IMDISK_AUTO_DEVICE_NUMBER
  ULONG           DeviceNumber;
  /// Total size in bytes (in the Cylinders field) and virtual geometry.
  DISK_GEOMETRY   DiskGeometry;
  /// The byte offset in the image file where the virtual disk begins.
  LARGE_INTEGER   ImageOffset;
  /// Creation flags. Type of device and type of connection.
  ULONG           Flags;
  /// Driveletter (if used, otherwise zero).
  WCHAR           DriveLetter;
  /// Length in bytes of the FileName member.
  USHORT          FileNameLength;
  /// Dynamically-sized member that specifies the image file name.
  WCHAR           FileName[1];
} IMDISK_CREATE_DATA, *PIMDISK_CREATE_DATA;

typedef struct _IMDISK_SET_DEVICE_FLAGS
{
  ULONG FlagsToChange;
  ULONG FlagValues;
} IMDISK_SET_DEVICE_FLAGS, *PIMDISK_SET_DEVICE_FLAGS;

#define IMDISK_API_NO_BROADCAST_NOTIFY  0x00000001
#define IMDISK_API_FORCE_DISMOUNT       0x00000002

#pragma pack(push)
#pragma pack(1)
typedef struct _FAT_BPB
{
  USHORT BytesPerSector;
  UCHAR  SectorsPerCluster;
  USHORT ReservedSectors;
  UCHAR  NumberOfFileAllocationTables;
  USHORT NumberOfRootEntries;
  USHORT NumberOfSectors;
  UCHAR  MediaDescriptor;
  USHORT SectorsPerFileAllocationTable;
  USHORT SectorsPerTrack;
  USHORT NumberOfHeads;
  union
  {
    struct
    {
      USHORT NumberOfHiddenSectors;
      USHORT TotalNumberOfSectors;
    } DOS_320;
    struct
    {
      ULONG  NumberOfHiddenSectors;
      ULONG  TotalNumberOfSectors;
    } DOS_331;
  };
} FAT_BPB, *PFAT_BPB;

typedef struct _FAT_VBR
{
  UCHAR   JumpInstruction[3];
  CHAR    OEMName[8];
  FAT_BPB BPB;
  UCHAR   FillData[512-3-8-sizeof(FAT_BPB)-1-2];
  UCHAR   PhysicalDriveNumber;
  UCHAR   Signature[2];
} FAT_VBR, *PFAT_VBR;
#pragma pack(pop)

#ifdef WINAPI

#ifdef __cplusplus
extern "C" {
#endif

/**
   Get behaviour flags for API.
*/
ULONGLONG
WINAPI
ImDiskGetAPIFlags();

/**
   Set behaviour flags for API. Returns previously defined flag field.

   Flags        New flags value to set.
*/
ULONGLONG
WINAPI
ImDiskSetAPIFlags(ULONGLONG Flags);

/**
   An interactive rundll32.exe-compatible function to show the Add New Virtual
   Disk dialog box with a file name already filled in. It is used by the
   Windows Explorer context menues.

   hWnd         Specifies a window that will be the owner window of any
                MessageBox:es or similar.

   hInst        Ignored.

   lpszCmdLine  An ANSI string specifying the image file to mount.

   nCmdShow     Ignored.
*/
void
WINAPI
RunDLL_MountFile(HWND hWnd,
		 HINSTANCE hInst,
		 LPSTR lpszCmdLine,
		 int nCmdShow);

/**
   An interactive rundll32.exe-compatible function to remove an existing ImDisk
   virtual disk. If the filesystem on the device cannot be locked and
   dismounted a MessageBox() is displayed that asks the user if dismount should
   be forced.

   hWnd         Specifies a window that will be the owner window of any
                MessageBox:es or similar.

   hInst        Ignored.

   lpszCmdLine  An ANSI string specifying the the virtual disk to remove. This
                can be on the form "F:" or "F:\" (without the quotes).

   nCmdShow     Ignored.
*/
void
WINAPI
RunDLL_RemoveDevice(HWND hWnd,
		    HINSTANCE hInst,
		    LPSTR lpszCmdLine,
		    int nCmdShow);

/**
   An interactive rundll32.exe-compatible function to save a virtual or
   physical drive as an image file. If the filesystem on the device cannot be
   locked and dismounted a MessageBox() is displayed that asks the user if the
   image saving should continue anyway.

   hWnd         Specifies a window that will be the owner window of any
                MessageBox:es or similar.

   hInst        Ignored.

   lpszCmdLine  An ANSI string specifying the the disk to save. This can be on
   the form "F:" or "F:\" (without the quotes).

   nCmdShow     Ignored.
*/
void
WINAPI
RunDLL_SaveImageFile(HWND hWnd,
		     HINSTANCE hInst,
	             LPSTR lpszCmdLine,
		     int nCmdShow);

/**
   This function displays a MessageBox() dialog with a
   FormatMessage()-formatted message.
   
   hWndParent   Parent window for the MessageBox() call.

   uStyle       Style for the MessageBox() call.

   lpTitle      Window title for the MessageBox() call.

   lpMessage    Format string to be used in call to FormatMessage() followed
                 by field parameters.
*/
BOOL
CDECL
MsgBoxPrintF(IN HWND hWndParent OPTIONAL,
	     IN UINT uStyle,
	     IN LPCWSTR lpTitle,
	     IN LPCWSTR lpMessage, ...);

/**
   This function is a quick perror()-style way of displaying an error message
   for the last failed Windows API call.

   hWndParent   Parent window for the MessageBox() call.

   Prefix       Text to print before the error message string.
*/
VOID
WINAPI
MsgBoxLastError(IN HWND hWndParent OPTIONAL,
		IN LPCWSTR Prefix);

/**
   Used to get a string describing a partition type.

   PartitionType  Partition type from partition table.

   Name           Pointer to memory that receives a string describing the
                  partition type.

   NameSize       Size of memory area pointed to by the Name parameter.
*/
VOID
WINAPI
ImDiskGetPartitionTypeName(IN BYTE PartitionType,
			   IN OUT LPWSTR Name,
			   IN DWORD NameSize);

/**
   Returns the offset in bytes to actual disk image data for some known
   "non-raw" image file formats with headers. Returns TRUE if file extension
   is recognized and the known offset has been stored in the variable pointed
   to by the Offset parameter. Otherwise the function returns FALSE and the
   value pointed to by the Offset parameter is not changed.

   ImageFile    Name of raw disk image file. This does not need to be a valid
                path or filename, just the extension is used by this function.

   Offset       Returned offset in bytes if function returns TRUE.
*/
BOOL
WINAPI
ImDiskGetOffsetByFileExt(IN LPCWSTR ImageFile,
			 IN OUT PLARGE_INTEGER Offset);

/**
   Attempts to find partition information from a partition table for a raw
   disk image file. If no master boot record is found this function returns
   FALSE. Returns TRUE if a master boot record with a partition table is found
   and values stored in the structures pointed to by the PartitionInformation
   parameter. Otherwise the function returns FALSE.

   ImageFile    Name of raw disk image file to examine.

   SectorSize   Optional sector size used on disk if different from default
                512 bytes.

   Offset       Optional offset in bytes to master boot record within file for
                use with "non-raw" image files with headers before the actual
		disk image data.

   PartitionInformation
                Pointer to an array of eight PARTITION_INFORMATION structures
		which will receive information from four recognized primary
		partition entries followed by four recognized extended entries.
*/
BOOL
WINAPI
ImDiskGetPartitionInformation(IN LPCWSTR ImageFile,
			      IN DWORD SectorSize OPTIONAL,
			      IN PLARGE_INTEGER Offset OPTIONAL,
			      IN OUT PPARTITION_INFORMATION
			      PartitionInformation);


/**
   Prototype for raw disk reader function used with ImDisk***Indirect()
   functions.

   Handle                Value that was passed as first parameter to
                         ImDiskGetPartitionInfoIndirect().

   Buffer                Buffer where read data is to be stored.

   Offset                Disk offset where read operation starts.

   NumberOfBytesToRead   Number of bytes to read from disk.

   NumberOfBytesRead     Pointer to DWORD size variable where function stores
                         number of bytes actually read into Buffer. This value
                         can be equal to or less than NumberOfBytesToRead
			 parameter.
*/
typedef BOOL (WINAPI *ImDiskReadFileProc)(IN HANDLE Handle,
					  IN OUT LPVOID Buffer,
					  IN LARGE_INTEGER Offset,
					  IN DWORD NumberOfBytesToRead,
					  IN OUT LPDWORD NumberOfBytesRead);

/**
   A device read function with ImDiskReadFileProc, which means that it can be
   used when calling ImDiskGetPartitionInfoIndirect() function.

   Handle       Operating system file handle representing a file or device
                opened for reading.

   Buffer       Buffer where read data is to be stored.

   Offset       Disk offset where read operation starts.

   NumberOfBytesToRead
                Number of bytes to read from disk.

   NumberOfBytesRead
                Pointer to DWORD size variable where function stores number of
		bytes actually read into Buffer. This value can be equal to or
		less than NumberOfBytesToRead parameter.
*/
BOOL
WINAPI
ImDiskReadFileHandle(IN HANDLE Handle,
		     IN OUT LPVOID Buffer,
		     IN LARGE_INTEGER Offset,
		     IN DWORD NumberOfBytesToRead,
		     IN OUT LPDWORD NumberOfBytesRead);

/**
   Attempts to find partition information from a partition table for a disk
   image through a supplied device reader function.

   If no master boot record is found this function returns FALSE. Returns TRUE
   if a master boot record with a partition table is found and values stored in
   the structures pointed to by the PartitionInformation parameter. Otherwise
   the function returns FALSE.

   Handle       Value that is passed as first parameter to ReadFileProc.

   ReadFileProc Procedure of type ImDiskReadFileProc that is called to read raw
                disk image.

   SectorSize   Optional sector size used on disk if different from default
                512 bytes.

   Offset       Optional offset in bytes to master boot record within file for
                use with "non-raw" image files with headers before the actual
		disk image data.

   PartitionInformation
                Pointer to an array of eight PARTITION_INFORMATION structures
		which will receive information from four recognized primary
		partition entries followed by four recognized extended entries.
*/
BOOL
WINAPI
ImDiskGetPartitionInfoIndirect(IN HANDLE Handle,
			       IN ImDiskReadFileProc ReadFileProc,
			       IN DWORD SectorSize OPTIONAL,
			       IN PLARGE_INTEGER Offset OPTIONAL,
			       IN OUT PPARTITION_INFORMATION PartitionInfo);

/**
   Finds out if image file contains an ISO9660 filesystem.

   ImageFile    Name of disk image file to examine.

   Offset       Optional offset in bytes to where raw disk data begins, for use
                with "non-raw" image files with headers before the actual disk
		image data.
*/
BOOL
WINAPI
ImDiskImageContainsISOFS(IN LPCWSTR ImageFile,
			 IN PLARGE_INTEGER Offset OPTIONAL);

/**
   Finds out if image file contains an ISO9660 filesystem, through a supplied
   device reader function.

   Handle       Value that is passed as first parameter to ReadFileProc.

   ReadFileProc Procedure of type ImDiskReadFileProc that is called to read raw
                disk image.

   Offset       Optional offset in bytes to where raw disk data begins, for use
                with "non-raw" image files with headers before the actual disk
		image data.
*/
BOOL
WINAPI
ImDiskImageContainsISOFSIndirect(IN HANDLE Handle,
				 IN ImDiskReadFileProc ReadFileProc,
				 IN PLARGE_INTEGER Offset OPTIONAL);

/**
   Starts a Win32 service or loads a kernel module or driver.

   ServiceName  Key name of the service or driver.
*/
BOOL
WINAPI
ImDiskStartService(IN LPWSTR ServiceName);

/**
   An easy way to turn an empty NTFS directory to a reparsepoint that redirects
   requests to a mounted device. Acts quite like mount points or symbolic links
   in *nix. If MountPoint specifies a character followed by a colon, a drive
   letter is instead created to point to Target.

   MountPoint   Path to empty directory on an NTFS volume, or a drive letter
                followed by a colon.

   Target       Target device path on kernel object namespace form, e.g.
                \Device\ImDisk2 or similar.
*/
BOOL
WINAPI
ImDiskCreateMountPoint(IN LPCWSTR MountPoint,
		       IN LPCWSTR Target);

/**
   Restores a reparsepoint to be an ordinary empty directory, or removes a
   drive letter mount point.

   MountPoint   Path to a reparse point on an NTFS volume, or a drive letter
                followed by a colon to remove a drive letter mount point.
*/
BOOL
WINAPI
ImDiskRemoveMountPoint(IN LPCWSTR MountPoint);

/**
   Opens a device object in the kernel object namespace.

   FileName     Full kernel object namespace path to the object to open, e.g.
                \Device\ImDisk2 or similar.

   AccessMode   Access mode to request.
*/
HANDLE
WINAPI
ImDiskOpenDeviceByName(IN PUNICODE_STRING FileName,
		       IN DWORD AccessMode);

/**
   Opens an ImDisk device by the device number.

   FileName     Native path to ImDisk device, such as "\Device\ImDisk2".

   AccessMode   Access mode to request.
*/
HANDLE
WINAPI
ImDiskOpenDeviceByNumber(IN DWORD DeviceNumber,
			 IN DWORD AccessMode);

/**
   Opens the device a junction/mount-point type reparse point is pointing to.

   MountPoint   Path to the reparse point on an NTFS volume.

   AccessMode   Access mode to request to the target device.
*/
HANDLE
WINAPI
ImDiskOpenDeviceByMountPoint(IN LPCWSTR MountPoint,
			     IN DWORD AccessMode);

/**
   Check that the user-mode library and kernel-mode driver version matches for
   an open ImDisk created device object.

   DeviceHandle Handle to an open ImDisk virtual disk or control device.
*/
BOOL
WINAPI
ImDiskCheckDriverVersion(IN HANDLE DeviceHandle);

/**
   Retrieves the version numbers of the user-mode API library and the kernel-
   mode driver.
*/
BOOL
WINAPI
ImDiskGetVersion(IN OUT PULONG LibraryVersion OPTIONAL,
		 IN OUT PULONG DriverVersion OPTIONAL);

/**
   Returns the first free drive letter in the range D-Z.
*/
WCHAR
WINAPI
ImDiskFindFreeDriveLetter();

/**
   Returns a bit-field representing ImDisk devices. Bit 0 represents device 0,
   bit 1 represents device 1 and so on. A bit is 1 if the device exists or 0 if
   the device number is free.
*/
ULONGLONG
WINAPI
ImDiskGetDeviceList();

/**
   This function sends an IOCTL_IMDISK_QUERY_DEVICE control code to an existing
   device and returns information about the device in an IMDISK_CREATE_DATA
   structue.

   DeviceNumber    Number of the ImDisk device to query.

   CreateData      Pointer to a sufficiently large IMDISK_CREATE_DATA
                   structure to receive all data including the image file name
		   where applicable.

   CreateDataSize  The size in bytes of the memory the CreateData parameter
                   points to. The function call will fail if the memory is not
		   large enough to hold the entire IMDISK_CREATE_DATA
		   structure.
*/
BOOL
WINAPI
ImDiskQueryDevice(IN DWORD DeviceNumber,
		  IN OUT PIMDISK_CREATE_DATA CreateData,
		  IN ULONG CreateDataSize);

/**
   This function creates a new ImDisk virtual disk device.

   hWndStatusText  A handle to a window that can display status message text.
                   The function will send WM_SETTEXT messages to this window.
		   If this parameter is NULL no WM_SETTEXT messages are sent
		   and the function acts non-interactive.

   DiskGeometry    The virtual geometry of the new virtual disk. Note that the
                   Cylinders member does not specify the number of Cylinders
		   but the total size in bytes of the new virtual disk. The
		   actual number of cylinders are then automatically
		   calculated and rounded down if necessary.

		   The Cylinders member can be zero if the device is backed by
		   an image file or a proxy device, but not if it is virtual
		   memory only device.

		   All or some of the other members of this structure can be
		   zero in which case they are automatically filled in with
		   most reasonable values by the driver.

   Flags           Bitwise or-ed combination of one of the IMDISK_TYPE_xxx
                   flags, one of the IMDISK_DEVICE_TYPE_xxx flags and any
		   number of IMDISK_OPTION_xxx flags. The flags can often be
		   left zero and left to the driver to automatically select.
		   For example, if a virtual disk size is specified to 1440 KB
		   and an image file name is not specified, the driver
		   automatically selects IMDISK_TYPE_VM|IMDISK_DEVICE_TYPE_FD
		   for this parameter.

   FileName        Name of disk image file. In case IMDISK_TYPE_VM is
                   specified in the Flags parameter, this file will be loaded
		   into the virtual memory-backed disk when created.

   NativePath      Set to TRUE if the FileName parameter specifies an NT
                   native path, such as \??\C:\imagefile.img or FALSE if it
		   specifies a Win32/DOS-style path such as C:\imagefile.img.

   MountPoint      Drive letter to assign to the new virtual device. It can be
                   specified on the form F: or F:\.
*/
BOOL
WINAPI
ImDiskCreateDevice(IN HWND hWndStatusText OPTIONAL,
		   IN OUT PDISK_GEOMETRY DiskGeometry OPTIONAL,
		   IN PLARGE_INTEGER ImageOffset OPTIONAL,
		   IN DWORD Flags OPTIONAL,
		   IN LPCWSTR FileName OPTIONAL,
		   IN BOOL NativePath,
		   IN LPWSTR MountPoint OPTIONAL);

/**
   This function creates a new ImDisk virtual disk device.

   hWndStatusText  A handle to a window that can display status message text.
                   The function will send WM_SETTEXT messages to this window.
		   If this parameter is NULL no WM_SETTEXT messages are sent
		   and the function acts non-interactive.

   DeviceNumber    In: Device number for device to create. Device number must
                   not be in use by an existing virtual disk. For automatic
                   allocation of device number, use IMDISK_AUTO_DEVICE_NUMBER
                   constant or specify a NULL pointer.

                   Out: If DeviceNumber parameter is not NULL, device number
		   for created device is returned in DWORD variable pointed to.

   DiskGeometry    The virtual geometry of the new virtual disk. Note that the
                   Cylinders member does not specify the number of Cylinders
		   but the total size in bytes of the new virtual disk. The
		   actual number of cylinders are then automatically
		   calculated and rounded down if necessary.

		   The Cylinders member can be zero if the device is backed by
		   an image file or a proxy device, but not if it is virtual
		   memory only device.

		   All or some of the other members of this structure can be
		   zero in which case they are automatically filled in with
		   most reasonable values by the driver.

   Flags           Bitwise or-ed combination of one of the IMDISK_TYPE_xxx
                   flags, one of the IMDISK_DEVICE_TYPE_xxx flags and any
		   number of IMDISK_OPTION_xxx flags. The flags can often be
		   left zero and left to the driver to automatically select.
		   For example, if a virtual disk size is specified to 1440 KB
		   and an image file name is not specified, the driver
		   automatically selects IMDISK_TYPE_VM|IMDISK_DEVICE_TYPE_FD
		   for this parameter.

   FileName        Name of disk image file. In case IMDISK_TYPE_VM is
                   specified in the Flags parameter, this file will be loaded
		   into the virtual memory-backed disk when created.

   NativePath      Set to TRUE if the FileName parameter specifies an NT
                   native path, such as \??\C:\imagefile.img or FALSE if it
		   specifies a Win32/DOS-style path such as C:\imagefile.img.

   MountPoint      Drive letter to assign to the new virtual device. It can be
                   specified on the form F: or F:\.
*/
BOOL
WINAPI
ImDiskCreateDeviceEx(IN HWND hWndStatusText OPTIONAL,
		     IN OUT LPDWORD DeviceNumber OPTIONAL,
		     IN OUT PDISK_GEOMETRY DiskGeometry OPTIONAL,
		     IN PLARGE_INTEGER ImageOffset OPTIONAL,
		     IN DWORD Flags OPTIONAL,
		     IN LPCWSTR FileName OPTIONAL,
		     IN BOOL NativePath,
		     IN LPWSTR MountPoint OPTIONAL);

/**
   This function removes (unmounts) an existing ImDisk virtual disk device.

   hWndStatusText  A handle to a window that can display status message text.
                   The function will send WM_SETTEXT messages to this window.
		   If this parameter is NULL no WM_SETTEXT messages are sent
		   and the function acts non-interactive.

   DeviceNumber    Number of the ImDisk device to remove. This parameter is
                   only used if MountPoint parameter is null.

   MountPoint      Drive letter of the device to remove. It can be specified
                   on the form F: or F:\.
*/
BOOL
WINAPI
ImDiskRemoveDevice(IN HWND hWndStatusText OPTIONAL,
		   IN DWORD DeviceNumber OPTIONAL,
		   IN LPCWSTR MountPoint OPTIONAL);

/**
   This function forcefully removes (unmounts) an existing ImDisk virtual disk
   device. Any unsaved data will be lost.

   Device          Handle to open device. If not NULL, it is used to query
                   device number to find out which device to remove. If this
		   parameter is NULL the DeviceNumber parameter is used
		   instead.

   DeviceNumber    Number of the ImDisk device to remove. This parameter is
                   only used if Device parameter is NULL.
*/
BOOL
WINAPI
ImDiskForceRemoveDevice(IN HANDLE Device OPTIONAL,
			IN DWORD DeviceNumber OPTIONAL);

/**
   This function changes the device characteristics of an existing ImDisk
   virtual disk device.

   hWndStatusText  A handle to a window that can display status message text.
                   The function will send WM_SETTEXT messages to this window.
		   If this parameter is NULL no WM_SETTEXT messages are sent
		   and the function acts non-interactive.

   DeviceNumber    Number of the ImDisk device to change. This parameter is
                   only used if MountPoint parameter is null.

   MountPoint      Drive letter of the device to change. It can be specified
                   on the form F: or F:\.

   FlagsToChange   A bit-field specifying which flags to edit. The flags are
                   the same as the option flags in the Flags parameter used
		   when a new virtual disk is created. Only flags set in this
		   parameter are changed to the corresponding flag value in the
		   Flags parameter.

   Flags           New values for the flags specified by the FlagsToChange
                   parameter.
*/
BOOL
WINAPI
ImDiskChangeFlags(HWND hWndStatusText OPTIONAL,
		  DWORD DeviceNumber OPTIONAL,
		  LPCWSTR MountPoint OPTIONAL,
		  DWORD FlagsToChange,
		  DWORD Flags);

/**
   This function extends the size of an existing ImDisk virtual disk device.

   hWndStatusText  A handle to a window that can display status message text.
                   The function will send WM_SETTEXT messages to this window.
		   If this parameter is NULL no WM_SETTEXT messages are sent
		   and the function acts non-interactive.

   DeviceNumber    Number of the ImDisk device to extend.

   ExtendSize      A pointer to a LARGE_INTEGER structure that specifies the
                   number of bytes to extend the device.
*/
BOOL
WINAPI
ImDiskExtendDevice(IN HWND hWndStatusText OPTIONAL,
		   IN DWORD DeviceNumber,
		   IN CONST PLARGE_INTEGER ExtendSize);

/**
   This function saves the contents of a device to an image file.

   DeviceHandle    Handle to a device for which the contents are to be saved to
                   an image file.

		   The handle must be opened for reading, may be
		   opened for sequential scan and/or without intermediate
		   buffering but cannot be opened for overlapped operation.
		   Please note that a call to this function will turn on
		   FSCTL_ALLOW_EXTENDED_DASD_IO on for this handle.

   FileHandle      Handle to an image file opened for writing. The handle
                   can be opened for operation without intermediate buffering
		   but performance is usually better if the handle is opened
		   with intermediate buffering. The handle cannot be opened for
		   overlapped operation.

   BufferSize      I/O buffer size to use when reading source disk. This
                   parameter is optional, if it is zero the buffer size to use
                   will automatically choosen.

   CancelFlag      Optional pointer to a BOOL value. If this BOOL value is set
                   to TRUE during the function call the operation is cancelled,
		   the function returns FALSE and GetLastError() will return
		   ERROR_CANCELLED. If this parameter is non-null the function
		   will also dispatch window messages for the current thread
		   between each I/O operation.
*/
BOOL
WINAPI
ImDiskSaveImageFile(IN HANDLE DeviceHandle,
		    IN HANDLE FileHandle,
		    IN DWORD BufferSize OPTIONAL,
		    IN LPBOOL CancelFlag OPTIONAL);

/**
   This function gets the size of a disk volume.

   Handle          Handle to a disk volume device.

   Size            Pointer to a 64 bit variable that upon successful completion
                   receives disk volume size as a signed integer.
*/
BOOL
WINAPI
ImDiskGetVolumeSize(IN HANDLE Handle,
		    IN OUT PLONGLONG Size);

/**
   Reads formatted geometry for a volume by parsing BPB, BIOS Parameter Block,
   from volume boot record into a DISK_GEOMETRY structure.

   If no boot record signature is found, this function returns FALSE.

   ImageFile    Path to a volume image file or a device path to a disk volume,
                such as \\.\A: or \\.\C:.

   Offset       Optional offset in bytes to volume boot record within file for
                use with "non-raw" volume image files. This parameter can be
		used to for example skip over headers for specific disk image
		formats, or to skip over master boot record in a disk image
		file that contains a complete raw disk image and not only a
		single volume.

   DiskGeometry Pointer to DISK_GEOMETRY structure that receives information
                about formatted geometry. This function zeroes the Cylinders
		member.
*/
BOOL
WINAPI
ImDiskGetFormattedGeometry(IN LPCWSTR ImageFile,
			   IN PLARGE_INTEGER Offset OPTIONAL,
			   IN OUT PDISK_GEOMETRY DiskGeometry);

/**
   Reads formatted geometry for a volume by parsing BPB, BIOS Parameter Block,
   from volume boot record into a DISK_GEOMETRY structure.

   If no boot record signature is found, this function returns FALSE.

   Handle       Value that is passed as first parameter to ReadFileProc.

   ReadFileProc Procedure of type ImDiskReadFileProc that is called to read
                disk volume.

   Offset       Optional offset in bytes to volume boot record within file for
                use with "non-raw" volume image files. This parameter can be
		used to for example skip over headers for specific disk image
		formats, or to skip over master boot record in a disk image
		file that contains a complete raw disk image and not only a
		single volume.

   DiskGeometry Pointer to DISK_GEOMETRY structure that receives information
                about formatted geometry. This function zeroes the Cylinders
		member.
*/
BOOL
WINAPI
ImDiskGetFormattedGeometryIndirect(IN HANDLE Handle,
				   IN ImDiskReadFileProc ReadFileProc,
				   IN PLARGE_INTEGER Offset OPTIONAL,
				   IN OUT PDISK_GEOMETRY DiskGeometry);

/**
   This function builds a Master Boot Record, MBR, in memory. The MBR will
   contain a default Initial Program Loader, IPL, which could be used to boot
   an operating system partition when the MBR is written to a disk.

   DiskGeometry    Pointer to a DISK_GEOMETRY or DISK_GEOMETRY_EX structure
                   that contains information about logical geometry of the
		   disk.

		   This function only uses the BytesPerSector, SectorsPerTrack
		   and TracksPerCylinder members.

		   This parameter can be NULL if NumberOfParts parameter is
		   zero.

   PartitionInfo   Pointer to an array of up to four PARTITION_INFORMATION
                   structures containing information about partitions to store
		   in MBR partition table.

		   This function only uses the StartingOffset, PartitionLength,
		   BootIndicator and PartitionType members.

		   This parameter can be NULL if NumberOfParts parameter is
		   zero.

   NumberOfParts   Number of PARTITION_INFORMATION structures in array that
                   PartitionInfo parameter points to.

		   If this parameter is zero, DiskGeometry and PartitionInfo
		   parameters are ignored and can be NULL. In that case MBR
		   will contain an empty partition table when this function
		   returns.

   MBR             Pointer to memory buffer of at least 512 bytes where MBR
                   will be built.

   MBRSize         Size of buffer pointed to by MBR parameter. This parameter
                   must be at least 512.
*/
BOOL
WINAPI
ImDiskBuildMBR(IN PDISK_GEOMETRY DiskGeometry OPTIONAL,
	       IN PPARTITION_INFORMATION PartitionInfo OPTIONAL,
	       IN BYTE NumberOfParts OPTIONAL,
	       IN OUT LPBYTE MBR,
	       IN DWORD_PTR MBRSize);

/**
   This function converts a CHS disk address to LBA format.

   DiskGeometry    Pointer to a DISK_GEOMETRY or DISK_GEOMETRY_EX structure
                   that contains information about logical geometry of the
		   disk. This function only uses the SectorsPerTrack and
		   TracksPerCylinder members.

   CHS             Pointer to CHS disk address in three-byte partition table
                   style format.
*/
DWORD
WINAPI
ImDiskConvertCHSToLBA(IN PDISK_GEOMETRY DiskGeometry,
		      IN LPBYTE CHS);

/**
   This function converts an LBA disk address to three-byte partition style CHS
   format. The three bytes are returned in the three lower bytes of a DWORD.

   DiskGeometry    Pointer to a DISK_GEOMETRY or DISK_GEOMETRY_EX structure
                   that contains information about logical geometry of the
		   disk. This function only uses the SectorsPerTrack and
		   TracksPerCylinder members.

   LBA             LBA disk address.
*/
DWORD
WINAPI
ImDiskConvertLBAToCHS(IN PDISK_GEOMETRY DiskGeometry,
		      IN DWORD LBA);

/**
   This function adjusts size of a saved image file. If file size is less than
   requested disk size, the size will be left unchanged with return value FALSE
   and GetLastError() will return ERROR_DISK_OPERATION_FAILED.

   FileHandle      Handle to file where disk image has been saved.

   FileSize        Size of original disk which image file should be adjusted
                   to.
*/
BOOL
WINAPI
ImDiskAdjustImageFileSize(IN HANDLE FileHandle,
			  IN PLARGE_INTEGER FileSize);

/**
   This function converts a native NT-style path to a Win32 DOS-style path. The
   path string is converted in-place and the start address is adjusted to skip
   over native directories such as \??\. Because of this, the Path parameter is
   a pointer to a pointer to a string so that the pointer can be adjusted to
   the new start address.

   Path            Pointer to pointer to Path string in native NT-style format.
                   Upon return the pointed address will contain the start
		   address of the Win32 DOS-style path within the original
		   buffer.
*/
VOID
WINAPI
ImDiskNativePathToWin32(IN OUT LPWSTR *Path);

/**
   This function saves the contents of a device to an image file. This is a
   user-interactive function that displays dialog boxes where user can select
   image file and other options.

   DeviceHandle    Handle to a device for which the contents are to be saved to
                   an image file.

		   The handle must be opened for reading, may be
		   opened for sequential scan and/or without intermediate
		   buffering but cannot be opened for overlapped operation.
		   Please note that a call to this function will turn on
		   FSCTL_ALLOW_EXTENDED_DASD_IO on for this handle.

   WindowHandle    Handle to existing window that will be parent to dialog
                   boxes etc.

   BufferSize      I/O buffer size to use when reading source disk. This
                   parameter is optional, if it is zero the buffer size to use
                   will automatically choosen.

   IsCdRomType     If this parameter is TRUE and the source device type cannot
                   be automatically determined this function will ask user for
		   a .iso suffixed image file name.
*/
VOID
WINAPI
ImDiskSaveImageFileInteractive(IN HANDLE DeviceHandle,
			       IN HWND WindowHandle OPTIONAL,
			       IN DWORD BufferSize OPTIONAL,
			       IN BOOL IsCdRomType OPTIONAL);

#ifdef __cplusplus
}
#endif

#endif

#endif // _INC_IMDISK_
