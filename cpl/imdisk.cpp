/*
    Control Panel Applet for the ImDisk Virtual Disk Driver for
    Windows NT/2000/XP.

    Copyright (C) 2007-2012 Olof Lagerkvist.

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

#include <windows.h>
#include <winioctl.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>
#include <cpl.h>

#include <stdio.h>
#include <malloc.h>

#include "..\inc\ntumapi.h"
#include "..\inc\imdisk.h"

#include "drvio.h"
#include "mbr.h"

#include "imdisk.rc.h"

/// Macros for "human readable" file sizes.
#define _1KB  (1ui64<<10)
#define _1MB  (1ui64<<20)
#define _1GB  (1ui64<<30)
#define _1TB  (1ui64<<40)

#define _B(n)  ((double)(n))
#define _KB(n) ((double)(n)/_1KB)
#define _MB(n) ((double)(n)/_1MB)
#define _GB(n) ((double)(n)/_1GB)
#define _TB(n) ((double)(n)/_1TB)

#define _h(n) ((n)>=_1TB ? _TB(n) : (n)>=_1GB ? _GB(n) :	\
	       (n)>=_1MB ? _MB(n) : (n)>=_1KB ? _KB(n) : (n))
#define _p(n) ((n)>=_1TB ? L"TB" : (n)>=_1GB ? L"GB" :			\
	       (n)>=_1MB ? L"MB" : (n)>=_1KB ? L"KB": \
	       (n)==1 ? L"byte" : L"bytes")

#define TXT_CURRENT_IMAGE_FILE_SIZE L"(current image file size)"

#define PROP_NAME_HKEY_MOUNTPOINTS2 L"HKEY_MountPoints2"
#define KEY_NAME_HKEY_MOUNTPOINTS2  \
  L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MountPoints2"

extern "C" HINSTANCE hInstance = NULL;

// Define DEBUG if you want debug output.
//#define DEBUG

#ifndef DEBUG

#define KdPrint(x)

#else

#define KdPrint(x)          DbgPrintF x

BOOL
DbgPrintF(LPCSTR Message, ...)
{
  va_list param_list;
  LPSTR lpBuf = NULL;

  va_start(param_list, Message);

  if (!FormatMessageA(FORMAT_MESSAGE_MAX_WIDTH_MASK |
		      FORMAT_MESSAGE_ALLOCATE_BUFFER |
		      FORMAT_MESSAGE_FROM_STRING, Message, 0, 0,
		      (LPSTR) &lpBuf, 0, &param_list))
    return FALSE;

  OutputDebugStringA(lpBuf);
  LocalFree(lpBuf);
  return TRUE;
}
#endif

int
LoadDeviceToList(HWND hWnd, int iDeviceNumber, bool SelectItem)
{
  PIMDISK_CREATE_DATA create_data = (PIMDISK_CREATE_DATA)
    _alloca(sizeof(IMDISK_CREATE_DATA) + (MAX_PATH << 2));

  if (create_data == NULL)
    {
      MessageBox(hWnd, L"Memory allocation error.", L"ImDisk", MB_ICONSTOP);
      return -1;
    }

  if (!ImDiskQueryDevice(iDeviceNumber, create_data,
			 sizeof(IMDISK_CREATE_DATA) + (MAX_PATH << 2)))
    return -1;

  WCHAR wcMountPoint[3] = L"";
  if (create_data->DriveLetter != 0)
    {
      wcMountPoint[0] = create_data->DriveLetter;
      wcMountPoint[1] = L':';
    }

  LVITEM lvi;
  lvi.mask = LVIF_IMAGE | LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
  lvi.iItem = 0;
  lvi.iSubItem = 0;
  lvi.pszText = wcMountPoint;

  if (SelectItem)
    lvi.state = LVIS_SELECTED;
  else
    lvi.state = 0;

  switch (IMDISK_DEVICE_TYPE(create_data->Flags))
    {
    case IMDISK_DEVICE_TYPE_FD:
      lvi.iImage = 1;
      break;

    case IMDISK_DEVICE_TYPE_CD:
      lvi.iImage = 2;
      break;

    default:
      lvi.iImage = 0;
    }

  lvi.lParam = iDeviceNumber;
  lvi.iItem = ListView_InsertItem(hWnd, &lvi);

  lvi.mask = LVIF_TEXT;
  lvi.iSubItem = 1;
  switch (IMDISK_TYPE(create_data->Flags))
    {
    case IMDISK_TYPE_FILE:
    case IMDISK_TYPE_VM:
      if (create_data->FileNameLength == 0)
	{
	  lvi.pszText = L"Virtual memory";
	  break;
	}

      lvi.pszText = (LPWSTR) _alloca(create_data->FileNameLength +
				     sizeof(*create_data->FileName));
      if (lvi.pszText == NULL)
	return lvi.iItem;

      wcsncpy(lvi.pszText, create_data->FileName,
	      create_data->FileNameLength >> 1);
      lvi.pszText[create_data->FileNameLength >> 1] = 0;

      ImDiskNativePathToWin32(&lvi.pszText);

      if (IMDISK_TYPE(create_data->Flags) == IMDISK_TYPE_VM)
	{
	  LPWSTR filename_part = lvi.pszText;

	  WCHAR Text[] = L"Virtual memory, from ";
	  lvi.pszText = (LPWSTR) _alloca(sizeof(Text) +
					 (wcslen(lvi.pszText) << 1) + 2);
	  if (lvi.pszText == NULL)
	    return lvi.iItem;

	  wcscpy(lvi.pszText, Text);
	  wcscat(lvi.pszText, filename_part);
	}
      break;

    case IMDISK_TYPE_PROXY:
      {
	WCHAR Text[] = L"Proxy through ";
	lvi.pszText = (LPWSTR) _alloca(create_data->FileNameLength +
				       sizeof(*create_data->FileName) +
				       sizeof(Text));
	if (lvi.pszText == NULL)
	  return lvi.iItem;
	wcsncpy(lvi.pszText, Text, sizeof(Text) >> 1);
	lvi.pszText[sizeof(Text) >> 1] = 0;
	wcsncat(lvi.pszText, create_data->FileName,
		create_data->FileNameLength >> 1);
	lvi.pszText[(create_data->FileNameLength + sizeof(Text) - 1) >> 1] =
	  0;
      }
      break;

    default:
      lvi.pszText = L"";
    }

  ListView_SetItem(hWnd, &lvi);

  WCHAR wcBuffer[128];
  switch (create_data->DiskGeometry.Cylinders.QuadPart)
    {
    case 2880 << 10:
      wcscpy(wcBuffer, L"2,880 KB");
      break;

    case 1722 << 10:
      wcscpy(wcBuffer, L"1,722 KB");
      break;

    case 1680 << 10:
      wcscpy(wcBuffer, L"1,680 KB");
      break;

    case 1440 << 10:
      wcscpy(wcBuffer, L"1,440 KB");
      break;

    case 1200 << 10:
      wcscpy(wcBuffer, L"1,200 KB");
      break;

    default:
      _snwprintf(wcBuffer, sizeof(wcBuffer)/sizeof(*wcBuffer)-1,
		 L"%.4g %s",
		 _h(create_data->DiskGeometry.Cylinders.QuadPart),
		 _p(create_data->DiskGeometry.Cylinders.QuadPart));
    }

  wcBuffer[sizeof(wcBuffer)/sizeof(*wcBuffer) - 1] = 0;

  lvi.iSubItem = 2;
  lvi.pszText = wcBuffer;
  ListView_SetItem(hWnd, &lvi);

  lvi.iSubItem = 3;
  if (IMDISK_READONLY(create_data->Flags))
    if (IMDISK_REMOVABLE(create_data->Flags))
      lvi.pszText = L"Read-only, Removable";
    else
      lvi.pszText = L"Read-only";
  else
    if (IMDISK_REMOVABLE(create_data->Flags))
      lvi.pszText = L"Read/write, Removable";
    else
      lvi.pszText = L"Read/write";

  ListView_SetItem(hWnd, &lvi);

  lvi.iSubItem = 4;
  if (create_data->DriveLetter != 0)
    if (GetVolumeInformation(wcMountPoint, NULL, 0, NULL, NULL, NULL,
			     wcBuffer, sizeof(wcBuffer)/sizeof(*wcBuffer)))
      lvi.pszText = wcBuffer;
    else
      lvi.pszText = L"N/A";
  else
    lvi.pszText = L"";
  ListView_SetItem(hWnd, &lvi);

  return lvi.iItem;
}

bool
RefreshList(HWND hWnd, DWORD SelectDeviceNumber)
{
  ListView_DeleteAllItems(hWnd);

  DWORDLONG device_list = ImDiskGetDeviceList();

  if (device_list == 0)
    switch (GetLastError())
      {
      case NO_ERROR:
      case ERROR_FILE_NOT_FOUND:
	return true;

      default:
	MsgBoxLastError(hWnd,
			L"Cannot control the ImDisk Virtual Disk Driver:");

	return false;
      }

  for (DWORD counter = 0; device_list != 0; device_list >>= 1, counter++)
    if (device_list & 1)
      LoadDeviceToList(hWnd, counter, counter == SelectDeviceNumber);

  return true;
}

INT_PTR
CALLBACK
AboutDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
    {
    case WM_INITDIALOG:
      {
	SetWindowText(hWnd, wcstok((LPWSTR) lParam, L"|"));
	SetDlgItemText(hWnd, IDC_EDT_ABOUTTEXT, wcstok(NULL, L""));
	return TRUE;
      }

    case WM_COMMAND:
      switch (LOWORD(wParam))
	{
	case IDCANCEL:
	case IDOK:
	  EndDialog(hWnd, IDOK);
	  return TRUE;
	}

      return TRUE;

    case WM_CLOSE:
      EndDialog(hWnd, IDCANCEL);
      return TRUE;
    }

  return FALSE;
}

VOID
CDECL
DisplayAboutBox(HWND hWnd, LPCWSTR lpMessage, ...)
{
  va_list param_list;
  LPWSTR lpBuf = NULL;

  va_start(param_list, lpMessage);

  if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
		     FORMAT_MESSAGE_FROM_STRING, lpMessage, 0, 0,
		     (LPWSTR) &lpBuf, 0, &param_list))
    return;

  DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_ABOUT_BOX), hWnd,
		 AboutDlgProc, (LPARAM) lpBuf);

  LocalFree(lpBuf);
}

INT_PTR
CALLBACK
OptionsSaveDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
    {
    case WM_INITDIALOG:
      {
	PIMDISK_CREATE_DATA create_data = (PIMDISK_CREATE_DATA) lParam;

	SetProp(hWnd, L"create_data", create_data);

	LONGLONG new_offset = (LONGLONG)
	  create_data->DiskGeometry.BytesPerSector *
	  create_data->DiskGeometry.SectorsPerTrack;

	WCHAR buffer[MAX_PATH << 1];

	if (create_data->FileNameLength > 0)
	  if (create_data->ImageOffset.QuadPart > 0)
	    {
	      _snwprintf(buffer, sizeof(buffer) / sizeof(*buffer),
			 L"Save disk image at offset %I64i in image file "
			 L"%.*ws (where it was originally loaded from). "
			 L"Original MBR or header will be left untouched.",
			 create_data->ImageOffset,
			 create_data->FileNameLength >> 1,
			 create_data->FileName);
	      buffer[sizeof(buffer)/sizeof(*buffer)-1] = 0;
	      SetDlgItemText(hWnd, IDC_BTN_ORIG_FILE_ORIG_OFFSET, buffer);
	      ShowWindow(GetDlgItem(hWnd, IDC_BTN_ORIG_FILE_ORIG_OFFSET),
			 SW_SHOWNORMAL);

	      _snwprintf(buffer, sizeof(buffer) / sizeof(*buffer),
			 L"Save disk image at offset 0 in image file %.*ws "
			 L"(destroying original MBR or other header).",
			 create_data->FileNameLength >> 1,
			 create_data->FileName);
	      buffer[sizeof(buffer)/sizeof(*buffer)-1] = 0;
	      SetDlgItemText(hWnd, IDC_BTN_ORIG_FILE_BASE, buffer);
	      ShowWindow(GetDlgItem(hWnd, IDC_BTN_ORIG_FILE_BASE),
			 SW_SHOWNORMAL);

	      CheckDlgButton(hWnd, IDC_BTN_ORIG_FILE_ORIG_OFFSET, BST_CHECKED);
	      SetFocus(GetDlgItem(hWnd, IDC_BTN_ORIG_FILE_ORIG_OFFSET));
	    }
	  else
	    {
	      _snwprintf(buffer, sizeof(buffer) / sizeof(*buffer),
			 L"Save disk image at offset 0 in image file %.*ws "
			 L"(where it was originally loaded from).",
			 create_data->FileNameLength >> 1,
			 create_data->FileName);
	      buffer[sizeof(buffer)/sizeof(*buffer)-1] = 0;
	      SetDlgItemText(hWnd, IDC_BTN_ORIG_FILE_BASE, buffer);
	      ShowWindow(GetDlgItem(hWnd, IDC_BTN_ORIG_FILE_BASE),
			 SW_SHOWNORMAL);

	      CheckDlgButton(hWnd, IDC_BTN_ORIG_FILE_BASE, BST_CHECKED);
	      SetFocus(GetDlgItem(hWnd, IDC_BTN_ORIG_FILE_BASE));
	    }
	else
	  {
	    CheckDlgButton(hWnd, IDC_BTN_NEW_FILE_BASE, BST_CHECKED);
	    SetFocus(GetDlgItem(hWnd, IDC_BTN_NEW_FILE_BASE));
	  }

	_snwprintf(buffer, sizeof(buffer) / sizeof(*buffer),
		   L"Save disk image at offset %I64i in new image file. New "
		   L"MBR will be created at beginning of new image file. "
		   L"Existing file contents will be overwritten.",
		   new_offset);
	buffer[sizeof(buffer)/sizeof(*buffer)-1] = 0;
	SetDlgItemText(hWnd, IDC_BTN_NEW_FILE_WITH_MBR, buffer);

	_snwprintf(buffer, sizeof(buffer) / sizeof(*buffer),
		   L"Save disk image at offset 0 in new image file. No MBR "
		   L"will be created. Existing file contents will be "
		   L"overwritten.");
	buffer[sizeof(buffer)/sizeof(*buffer)-1] = 0;
	SetDlgItemText(hWnd, IDC_BTN_NEW_FILE_BASE, buffer);

	return FALSE;
      }

    case WM_COMMAND:
      switch (LOWORD(wParam))
	{
	case IDOK:
	  {
	    PIMDISK_CREATE_DATA create_data =
	      (PIMDISK_CREATE_DATA) GetProp(hWnd, L"create_data");

	    if (IsDlgButtonChecked(hWnd, IDC_BTN_ORIG_FILE_BASE) ==
		BST_CHECKED)
	      create_data->ImageOffset.QuadPart = 0;
	    else if (IsDlgButtonChecked(hWnd, IDC_BTN_NEW_FILE_WITH_MBR) ==
		     BST_CHECKED)
	      {
		create_data->FileNameLength = 0;
		create_data->ImageOffset.QuadPart =
		  (LONGLONG)
		  create_data->DiskGeometry.SectorsPerTrack *
		  create_data->DiskGeometry.BytesPerSector;
	      }
	    else if (IsDlgButtonChecked(hWnd, IDC_BTN_NEW_FILE_BASE) ==
		     BST_CHECKED)
	      {
		create_data->FileNameLength = 0;
		create_data->ImageOffset.QuadPart = 0;
	      }

	    EndDialog(hWnd, IDOK);
	    return TRUE;
	  }

	case IDCANCEL:
	  EndDialog(hWnd, IDCANCEL);
	  return TRUE;
	}

      return TRUE;

    case WM_NCDESTROY:
      RemoveProp(hWnd, L"create_data");
      return TRUE;

    case WM_CLOSE:
      EndDialog(hWnd, IDCANCEL);
      return TRUE;
    }

  return FALSE;
}

void
SaveSelectedDeviceToImageFile(HWND hWnd)
{
  LVITEM lvi = { 0 };
  lvi.mask = LVIF_IMAGE | LVIF_TEXT | LVIF_PARAM;
  lvi.iItem = (int)
    SendDlgItemMessage(hWnd, IDC_LISTVIEW, LVM_GETNEXTITEM, (WPARAM) -1,
		       MAKELPARAM((UINT) LVNI_SELECTED, 0));

  if (lvi.iItem == -1)
    return;

  lvi.iSubItem = 0;
  WCHAR wcBuffer[3] = L"";
  lvi.pszText = wcBuffer;
  lvi.cchTextMax = sizeof(wcBuffer)/sizeof(*wcBuffer);
  SendDlgItemMessage(hWnd, IDC_LISTVIEW, LVM_GETITEM, 0, (LPARAM) &lvi);

  HANDLE hDev = ImDiskOpenDeviceByNumber((DWORD) lvi.lParam, GENERIC_READ);

  if (hDev == INVALID_HANDLE_VALUE)
    {
      MsgBoxLastError(hWnd, L"Cannot open drive for direct access:");
      return;
    }

  ImDiskSaveImageFileInteractive(hDev, hWnd, 0, FALSE);

  CloseHandle(hDev);
}

VOID
WINAPI
ImDiskSaveImageFileInteractive(IN HANDLE hDev,
			       IN HWND hWnd,
			       IN DWORD BufferSize,
			       IN BOOL IsCdRomType)
{
  DWORD create_data_size = sizeof(IMDISK_CREATE_DATA) + (MAX_PATH << 2);
  PIMDISK_CREATE_DATA create_data = (PIMDISK_CREATE_DATA)
    _alloca(create_data_size);
  PDISK_GEOMETRY disk_geometry = &create_data->DiskGeometry;
  PLARGE_INTEGER disk_size = &create_data->DiskGeometry.Cylinders;

  if (create_data == NULL)
    {
      MessageBox(hWnd, L"Memory allocation error.", L"ImDisk", MB_ICONSTOP);
      return;
    }
  
  ZeroMemory(create_data, create_data_size);

  DWORD dwRet;

  if (!DeviceIoControl(hDev, IOCTL_IMDISK_QUERY_DEVICE, NULL, 0, create_data,
		       create_data_size, &dwRet,
		       NULL))
    {
      if (!DeviceIoControl(hDev, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0,
			   disk_geometry,
			   sizeof(DISK_GEOMETRY),
			   &dwRet, NULL))
	return;

      if (!ImDiskGetVolumeSize(hDev, &disk_size->QuadPart))
	return;
    }

  WCHAR file_name[MAX_PATH + 1] = L"";

  if (IMDISK_TYPE(create_data->Flags) != IMDISK_TYPE_VM)
    create_data->FileNameLength = 0;

  if (create_data->FileNameLength > 0)
    if (create_data->FileNameLength >= sizeof file_name)
      create_data->FileNameLength = 0;
    else
      {
	wcsncpy(file_name, create_data->FileName,
		create_data->FileNameLength >> 1);

	LPWSTR win32_path = file_name;
	ImDiskNativePathToWin32(&win32_path);
	if (win32_path != file_name)
	  wcscpy(file_name, win32_path);

	create_data->FileNameLength = (USHORT) wcslen(file_name) << 1;
	memcpy(create_data->FileName, file_name, create_data->FileNameLength);
      }

  BOOL use_original_file_name = FALSE;
  LARGE_INTEGER save_offset = { 0 };
  if ((IMDISK_DEVICE_TYPE(create_data->Flags) == IMDISK_DEVICE_TYPE_HD) ?
      TRUE : !IsCdRomType)
    {
      INT_PTR i = DialogBoxParam(hInstance,
				 MAKEINTRESOURCE(IDD_DLG_OPTIONS_SAVE),
				 hWnd, OptionsSaveDlgProc,
				 (LPARAM) create_data);

      if (i == IDCANCEL)
	return;

      save_offset = create_data->ImageOffset;
      if (create_data->FileNameLength > 0)
	use_original_file_name = TRUE;
    }

  if (DeviceIoControl(hDev, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &dwRet, NULL))
    DeviceIoControl(hDev, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &dwRet,
		    NULL);
  else
    if (MessageBox(hWnd,
		   L"Cannot lock the drive. It may be in use by another "
		   L"program. Do you want to continue anyway?",
		   L"ImDisk Virtual Disk Driver",
		   MB_ICONEXCLAMATION | MB_OKCANCEL | MB_DEFBUTTON2) != IDOK)
      return;

  OPENFILENAME_NT4 ofn = { sizeof ofn };
  ofn.hwndOwner = hWnd;
  ofn.lpstrFilter = L"Image files (*.img)\0*.img\0";
  ofn.lpstrFile = file_name;
  ofn.nMaxFile = sizeof(file_name)/sizeof(*file_name);
  ofn.lpstrTitle = L"Save contents of virtual disk to image file";
  ofn.Flags =
    OFN_EXPLORER | OFN_LONGNAMES | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
  ofn.lpstrDefExt = L"img";

  if ((IMDISK_DEVICE_TYPE(create_data->Flags) == IMDISK_DEVICE_TYPE_CD) ?
      TRUE : IsCdRomType)
    {
      ofn.lpstrFilter = L"ISO image (*.iso)\0*.iso\0";
      ofn.lpstrDefExt = L"iso";
    }

  DWORD dwCreationDisposition;
  if (use_original_file_name)
    dwCreationDisposition = OPEN_EXISTING;
  else
    {
      if (!GetSaveFileName((LPOPENFILENAMEW) &ofn))
	return;

      dwCreationDisposition = CREATE_ALWAYS;
    }

  HANDLE hImage = CreateFile(ofn.lpstrFile,
			     GENERIC_WRITE,
			     FILE_SHARE_READ | FILE_SHARE_DELETE,
			     NULL,
			     dwCreationDisposition,
			     FILE_ATTRIBUTE_NORMAL,
			     NULL);

  if (hImage == INVALID_HANDLE_VALUE)
    {
      MsgBoxLastError(hWnd, L"Cannot open image file:");
      return;
    }

  // Saving to an image file with offset
  if (save_offset.QuadPart > 0)
    {
      // Not saving to existing image file? Create new MBR.
      if (!use_original_file_name)
	{
	  LPBYTE mbr = (LPBYTE)_alloca(default_mbr_size);
	  PARTITION_INFORMATION partition_info = { 0 };
	  partition_info.StartingOffset.QuadPart =
	    (LONGLONG)
	    disk_geometry->BytesPerSector *
	    disk_geometry->SectorsPerTrack;
	  partition_info.PartitionLength = *disk_size;
	  partition_info.BootIndicator = 0x80;
	  partition_info.PartitionType = 0x06;

	  if (!ImDiskBuildMBR(disk_geometry,
			      &partition_info,
			      1,
			      mbr,
			      default_mbr_size))
	    {
	      MsgBoxLastError(hWnd, L"Error creating new MBR:");
	      CloseHandle(hImage);
	      return;
	    }

	  if (!WriteFile(hImage, mbr, default_mbr_size, &dwRet, NULL))
	    {
	      MsgBoxLastError(hWnd, L"Error writing new MBR:");
	      CloseHandle(hImage);
	      return;
	    }
	}

      // Move to position where disk image should begin.
      if (SetFilePointer(hImage, save_offset.LowPart, &save_offset.HighPart,
			 FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	if (GetLastError() != ERROR_SUCCESS)
	  {
	    MsgBoxLastError(hWnd, L"Image file error:");
	    CloseHandle(hImage);
	    return;
	  }
    }

  BOOL bCancelFlag = FALSE;

  EnableWindow(hWnd, FALSE);
  HWND hWndStatus =
    CreateDialogParam(hInstance, MAKEINTRESOURCE(IDD_DLG_STATUS), hWnd,
		      StatusDlgProc, (LPARAM) &bCancelFlag);

  SetDlgItemText(hWndStatus, IDC_STATUS_MSG, L"Saving image file...");

  if (ImDiskSaveImageFile(hDev, hImage, BufferSize, &bCancelFlag))
    {
      DestroyWindow(hWndStatus);
      EnableWindow(hWnd, TRUE);

      if (!use_original_file_name)
	{
	  save_offset.QuadPart += disk_size->QuadPart;

	  if (!ImDiskAdjustImageFileSize(hImage, &save_offset))
	    {
	      DWORD dwLastError = GetLastError();

	      DeleteFile(ofn.lpstrFile);
	      CloseHandle(hImage);

	      SetLastError(dwLastError);

	      MsgBoxLastError(hWnd,
			      L"Drive contents could not be saved. Check that "
			      L"it contains a supported filesystem.");

	      return;
	    }
	}

      CloseHandle(hImage);
      MsgBoxPrintF(hWnd, MB_ICONINFORMATION,
		   L"ImDisk Virtual Disk Driver",
		   L"Successfully saved the contents of drive '%1!c!:' to "
		   L"image file '%2'.",
		   create_data->DriveLetter ? create_data->DriveLetter : L' ',
		   ofn.lpstrFile);
      return;
    }

  MsgBoxLastError(hWnd, L"Error saving image:");

  DestroyWindow(hWndStatus);
  EnableWindow(hWnd, TRUE);

  if (GetFileSize(hImage, NULL) == 0)
    DeleteFile(ofn.lpstrFile);

  CloseHandle(hImage);
}

INT_PTR
CALLBACK
SelectPartitionDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
    {
    case WM_INITDIALOG:
      {
	SendDlgItemMessage(hWnd, IDC_SELECT_PARTITION_LIST,
			   LB_ADDSTRING, 0, (LPARAM) L"Use entire image file");

	WCHAR wcBuffer[128];
	bool has_extended_partition = false;

	for (int i = 0; i < 4; i++)
	  {
	    PPARTITION_INFORMATION part_rec =
	      ((PPARTITION_INFORMATION) lParam) + i;

	    WCHAR part_name[128];
	    ImDiskGetPartitionTypeName(part_rec->PartitionType,
				       part_name,
				       sizeof(part_name)/sizeof(*part_name));

	    if (part_rec->PartitionLength.QuadPart != 0)
	      {
		_snwprintf(wcBuffer, sizeof(wcBuffer)/sizeof(*wcBuffer)-1,
			   L"Primary partition %i - %.4g %s %s",
			   i + 1,
			   _h(part_rec->PartitionLength.QuadPart),
			   _p(part_rec->PartitionLength.QuadPart),
			   part_name);
		wcBuffer[sizeof(wcBuffer)/sizeof(*wcBuffer) - 1] = 0;
		SendDlgItemMessage(hWnd, IDC_SELECT_PARTITION_LIST,
				   LB_ADDSTRING, 0, (LPARAM) wcBuffer);
	      }
	    else
	      {
		_snwprintf(wcBuffer, sizeof(wcBuffer)/sizeof(*wcBuffer)-1,
			   L"Primary partition %i - %s",
			   i + 1,
			   part_name);
		wcBuffer[sizeof(wcBuffer)/sizeof(*wcBuffer) - 1] = 0;
		SendDlgItemMessage(hWnd, IDC_SELECT_PARTITION_LIST,
				   LB_ADDSTRING, 0, (LPARAM) wcBuffer);
	      }

	    if (IsContainerPartition(part_rec->PartitionType))
	      has_extended_partition = true;
	  }

	if (has_extended_partition)
	  for (int i = 4; i < 8; i++)
	    {
	      PPARTITION_INFORMATION part_rec =
		((PPARTITION_INFORMATION) lParam) + i;

	      WCHAR part_name[128];
	      ImDiskGetPartitionTypeName(part_rec->PartitionType,
					 part_name,
					 sizeof(part_name)/sizeof(*part_name));

	      if ((part_rec->StartingOffset.QuadPart != 0) &
		  (part_rec->PartitionLength.QuadPart != 0))
		{
		  _snwprintf(wcBuffer, sizeof(wcBuffer)/sizeof(*wcBuffer)-1,
			     L"Logical partition %i - %.4g %s %s",
			     i - 3,
			     _h(part_rec->PartitionLength.QuadPart),
			     _p(part_rec->PartitionLength.QuadPart),
			     part_name);
		  wcBuffer[sizeof(wcBuffer)/sizeof(*wcBuffer) - 1] = 0;
		  SendDlgItemMessage(hWnd, IDC_SELECT_PARTITION_LIST,
				     LB_ADDSTRING, 0, (LPARAM) wcBuffer);
		}
	      else
		{
		  _snwprintf(wcBuffer, sizeof(wcBuffer)/sizeof(*wcBuffer)-1,
			     L"Logical partition %i - %s",
			     i - 3,
			     part_name);
		  wcBuffer[sizeof(wcBuffer)/sizeof(*wcBuffer) - 1] = 0;
		  SendDlgItemMessage(hWnd, IDC_SELECT_PARTITION_LIST,
				     LB_ADDSTRING, 0, (LPARAM) wcBuffer);
		}
	    }

	SetFocus(GetDlgItem(hWnd, IDC_SELECT_PARTITION_LIST));

	SendDlgItemMessage(hWnd, IDC_SELECT_PARTITION_LIST,
			   LB_SETSEL, TRUE, 0);
	
	return TRUE;
      }

    case WM_COMMAND:
      switch (LOWORD(wParam))
	{
	case IDCANCEL:
	case IDOK:
	  {
	    EndDialog(hWnd, SendDlgItemMessage(hWnd, IDC_SELECT_PARTITION_LIST,
					       LB_GETCURSEL, 0, 0));
	    return TRUE;
	  }
	}

      return TRUE;

    case WM_CLOSE:
      EndDialog(hWnd, IDCANCEL);
      return TRUE;
    }

  return FALSE;
}

void
AutoFindOffsetAndSize(LPCWSTR lpwszFileName, HWND hWnd)
{
  LARGE_INTEGER offset = { 0 };
  ImDiskGetOffsetByFileExt(lpwszFileName, &offset);

  LARGE_INTEGER size = { 0 };
  PARTITION_INFORMATION partition_information[8] = { 0 };
  int found_partitions = 0;

  if (ImDiskGetPartitionInformation(lpwszFileName,
				    0,
				    &offset,
				    partition_information))
    for (PPARTITION_INFORMATION part_rec = partition_information;
	 part_rec < partition_information + 8;
	 part_rec++)
      if ((part_rec->PartitionLength.QuadPart != 0) &
	  !IsContainerPartition(part_rec->PartitionType))
	++found_partitions;

  if (found_partitions > 1)
    {
      INT_PTR i = DialogBoxParam(hInstance,
				 MAKEINTRESOURCE(IDD_SELECT_PARTITION_DLG),
				 hWnd, SelectPartitionDlgProc,
				 (LPARAM) partition_information);
      if ((i >= 1) & (i <= 8))
	{
	  PPARTITION_INFORMATION part_rec = partition_information + i - 1;

	  if ((part_rec->StartingOffset.QuadPart != 0) &
	      (part_rec->PartitionLength.QuadPart != 0) &
	      !IsContainerPartition(part_rec->PartitionType))
	    {
	      offset.QuadPart += part_rec->StartingOffset.QuadPart;
	      size = part_rec->PartitionLength;
	    }
	}
    }
  else if (found_partitions == 1)
    for (PPARTITION_INFORMATION part_rec = partition_information;
	 part_rec < partition_information + 8;
	 part_rec++)
      if ((part_rec->StartingOffset.QuadPart != 0) &
	  (part_rec->PartitionLength.QuadPart != 0) &
	  !IsContainerPartition(part_rec->PartitionType))
	{
	  offset.QuadPart += part_rec->StartingOffset.QuadPart;
	  size = part_rec->PartitionLength;
	  break;
	}

  CheckDlgButton(hWnd, IDC_OFFSET_UNIT_B, BST_UNCHECKED);
  CheckDlgButton(hWnd, IDC_OFFSET_UNIT_BLOCKS, BST_UNCHECKED);
  CheckDlgButton(hWnd, IDC_OFFSET_UNIT_KB, BST_UNCHECKED);
  CheckDlgButton(hWnd, IDC_OFFSET_UNIT_MB, BST_UNCHECKED);
  CheckDlgButton(hWnd, IDC_OFFSET_UNIT_GB, BST_UNCHECKED);
  offset.QuadPart >>= 9;
  if ((offset.QuadPart > 0) & (offset.HighPart == 0))
    {
      SetDlgItemInt(hWnd, IDC_EDT_IMAGE_OFFSET, offset.LowPart, FALSE);
      CheckDlgButton(hWnd, IDC_OFFSET_UNIT_BLOCKS, BST_CHECKED);
    }
  else
    {
      SetDlgItemText(hWnd, IDC_EDT_IMAGE_OFFSET, L"0");
      CheckDlgButton(hWnd, IDC_OFFSET_UNIT_B, BST_CHECKED);
    }

  CheckDlgButton(hWnd, IDC_UNIT_B, BST_UNCHECKED);
  CheckDlgButton(hWnd, IDC_UNIT_BLOCKS, BST_UNCHECKED);
  CheckDlgButton(hWnd, IDC_UNIT_KB, BST_UNCHECKED);
  CheckDlgButton(hWnd, IDC_UNIT_MB, BST_UNCHECKED);
  CheckDlgButton(hWnd, IDC_UNIT_GB, BST_UNCHECKED);
  size.QuadPart >>= 9;
  if ((size.QuadPart > 0) & (size.HighPart == 0))
    {
      SetDlgItemInt(hWnd, IDC_EDT_SIZE, size.LowPart, FALSE);
      CheckDlgButton(hWnd, IDC_UNIT_BLOCKS, BST_CHECKED);
    }
  else
    {
      SetDlgItemText(hWnd, IDC_EDT_SIZE, TXT_CURRENT_IMAGE_FILE_SIZE);
      CheckDlgButton(hWnd, IDC_UNIT_B, BST_CHECKED);
    }
}

INT_PTR
CALLBACK
NewDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
    {
    case WM_INITDIALOG:
      {
	WCHAR free_drive_letter[3] = L" ";
	free_drive_letter[0] = ImDiskFindFreeDriveLetter();
	SendDlgItemMessage(hWnd, IDC_EDT_DRIVE, WM_SETTEXT, 0,
			   (LPARAM)(LPCWSTR) free_drive_letter);

	SendDlgItemMessage(hWnd, IDC_EDT_IMAGEFILE, EM_SETLIMITTEXT,
			   (WPARAM) MAX_PATH, 0);
	SendDlgItemMessage(hWnd, IDC_EDT_DRIVE, EM_SETLIMITTEXT, (WPARAM) 1,
			   0);

	CheckDlgButton(hWnd, IDC_UNIT_B, BST_CHECKED);
	CheckDlgButton(hWnd, IDC_OFFSET_UNIT_B, BST_CHECKED);
	CheckDlgButton(hWnd, IDC_DT_AUTO, BST_CHECKED);

	if (lParam != 0)
	  {
	    // Clear any leading and trailing quotes
	    LPWSTR file_name = (LPWSTR) lParam;
	    if (file_name[0] == L'"')
	      {
		size_t file_name_last_pos = wcslen(file_name) - 1;
		if (file_name[file_name_last_pos] == L'"')
		  (file_name++)[file_name_last_pos] = 0;
	      }

	    SetDlgItemText(hWnd, IDC_EDT_IMAGEFILE, file_name);
	    AutoFindOffsetAndSize(file_name, hWnd);
	  }

	return TRUE;
      }

    case WM_COMMAND:
      {
	if ((GetWindowTextLength(GetDlgItem(hWnd, IDC_EDT_IMAGEFILE)) > 0) |
	    (GetDlgItemInt(hWnd, IDC_EDT_SIZE, NULL, FALSE) > 0))
	  EnableWindow(GetDlgItem(hWnd, IDOK), TRUE);
	else
	  EnableWindow(GetDlgItem(hWnd, IDOK), FALSE);

	if (GetWindowTextLength(GetDlgItem(hWnd, IDC_EDT_IMAGEFILE)) > 0)
	  EnableWindow(GetDlgItem(hWnd, IDC_CHK_VM), TRUE);
	else
	  EnableWindow(GetDlgItem(hWnd, IDC_CHK_VM), FALSE);

	switch (LOWORD(wParam))
	  {
	  case IDC_BTN_BROWSE_FILE:
	    {
	      WCHAR file_name[MAX_PATH+1] = L"*";
	      if (GetWindowTextLength(GetDlgItem(hWnd, IDC_EDT_IMAGEFILE)) > 0)
		{
		  GetDlgItemText(hWnd, IDC_EDT_IMAGEFILE, file_name,
				 sizeof(file_name)/sizeof(*file_name)-1);
		  file_name[sizeof(file_name)/sizeof(*file_name)-1] = 0;
		}

	      OPENFILENAME_NT4 ofn = { sizeof ofn };
	      ofn.hwndOwner = hWnd;
	      ofn.lpstrFile = file_name;
	      ofn.nMaxFile = sizeof(file_name)/sizeof(*file_name);
	      ofn.lpstrTitle = L"Select image file";
	      ofn.Flags = OFN_CREATEPROMPT | OFN_EXPLORER | OFN_HIDEREADONLY |
		OFN_LONGNAMES | OFN_PATHMUSTEXIST;

	      if (GetOpenFileName((LPOPENFILENAMEW) &ofn))
		{
		  SetDlgItemText(hWnd, IDC_EDT_IMAGEFILE, file_name);
		  AutoFindOffsetAndSize(file_name, hWnd);
		}
	    }
	    return TRUE;

	  case IDOK:
	    {
	      EnableWindow(hWnd, FALSE);
	      HWND hWndStatus =
		CreateDialog(hInstance, MAKEINTRESOURCE(IDD_DLG_STATUS), hWnd,
			     StatusDlgProc);

	      SetDlgItemText(hWndStatus, IDC_STATUS_MSG,
			     L"Creating virtual disk...");

	      DISK_GEOMETRY disk_geometry = { 0 };
	      disk_geometry.Cylinders.QuadPart =
		GetDlgItemInt(hWnd, IDC_EDT_SIZE, NULL, FALSE);

	      if (IsDlgButtonChecked(hWnd, IDC_UNIT_GB))
		disk_geometry.Cylinders.QuadPart <<= 30;
	      else if (IsDlgButtonChecked(hWnd, IDC_UNIT_MB))
		disk_geometry.Cylinders.QuadPart <<= 20;
	      else if (IsDlgButtonChecked(hWnd, IDC_UNIT_KB))
		disk_geometry.Cylinders.QuadPart <<= 10;
	      else if (IsDlgButtonChecked(hWnd, IDC_UNIT_BLOCKS))
		disk_geometry.Cylinders.QuadPart <<= 9;

	      LARGE_INTEGER image_offset = { 0 };
	      image_offset.QuadPart =
		GetDlgItemInt(hWnd, IDC_EDT_IMAGE_OFFSET, NULL, FALSE);

	      if (IsDlgButtonChecked(hWnd, IDC_OFFSET_UNIT_GB))
		image_offset.QuadPart <<= 30;
	      else if (IsDlgButtonChecked(hWnd, IDC_OFFSET_UNIT_MB))
		image_offset.QuadPart <<= 20;
	      else if (IsDlgButtonChecked(hWnd, IDC_OFFSET_UNIT_KB))
		image_offset.QuadPart <<= 10;
	      else if (IsDlgButtonChecked(hWnd, IDC_OFFSET_UNIT_BLOCKS))
		image_offset.QuadPart <<= 9;

	      DWORD flags = 0;
	      
	      if (IsDlgButtonChecked(hWnd, IDC_DT_HD))
		flags |= IMDISK_DEVICE_TYPE_HD;
	      else if (IsDlgButtonChecked(hWnd, IDC_DT_FD))
		flags |= IMDISK_DEVICE_TYPE_FD;
	      else if (IsDlgButtonChecked(hWnd, IDC_DT_CD))
		flags |= IMDISK_DEVICE_TYPE_CD;

	      if (IsDlgButtonChecked(hWnd, IDC_CHK_READONLY))
		flags |= IMDISK_OPTION_RO;

	      if (IsDlgButtonChecked(hWnd, IDC_CHK_REMOVABLE))
		flags |= IMDISK_OPTION_REMOVABLE;

	      if (IsDlgButtonChecked(hWnd, IDC_CHK_VM))
		flags |= IMDISK_TYPE_VM;
	      
	      WCHAR file_name[MAX_PATH+1] = L"";
	      UINT uLen = GetDlgItemText(hWnd, IDC_EDT_IMAGEFILE, file_name,
					 sizeof(file_name)/sizeof(*file_name));
	      file_name[uLen] = 0;

	      WCHAR drive_letter[3];
	      GetDlgItemText(hWnd, IDC_EDT_DRIVE, drive_letter, 3);

	      drive_letter[0] = towupper(drive_letter[0]);
	      drive_letter[1] = L':';
	      drive_letter[2] = 0;

	      WCHAR buffer[MAX_PATH << 1];
	      if (QueryDosDevice(drive_letter, buffer, sizeof(buffer) >> 1))
		if (MessageBox(hWnd, L"Specified drive letter is already in "
			       L"use on this system. Are you sure you want "
			       L"to use it for the virtual disk drive you are "
			       L"about to create anyway?\r\n"
			       L"\r\n"
			       L"Existing drive will not be accessible until "
			       L"it has been assigned a new drive letter.",
			       L"ImDisk Virtual Disk Driver",
			       MB_ICONEXCLAMATION | MB_YESNO |
			       MB_DEFBUTTON2) == IDNO)
		  {
		    EnableWindow(hWnd, TRUE);
		    DestroyWindow(hWndStatus);
		    return TRUE;
		  }

	      DWORD device_number = IMDISK_AUTO_DEVICE_NUMBER;

	      BOOL status =
		ImDiskCreateDeviceEx(GetDlgItem(hWndStatus, IDC_STATUS_MSG),
				     &device_number,
				     &disk_geometry,
				     &image_offset,
				     flags,
				     file_name[0] == 0 ? NULL : file_name,
				     FALSE,
				     drive_letter[0] == 0 ?
				     NULL : drive_letter);

	      EnableWindow(hWnd, TRUE);
	      DestroyWindow(hWndStatus);
	      if (status)
		EndDialog(hWnd, device_number);
	      else
		EndDialog(hWnd, IMDISK_AUTO_DEVICE_NUMBER);
	    }
	    return TRUE;

	  case IDCANCEL:
	    EndDialog(hWnd, IMDISK_AUTO_DEVICE_NUMBER);
	    return TRUE;

	  case IDC_EDT_SIZE:
	    switch (HIWORD(wParam))
	      {
	      case EN_SETFOCUS:
		if (GetDlgItemInt(hWnd, IDC_EDT_SIZE, NULL, FALSE) == 0)
		  SetDlgItemText(hWnd, IDC_EDT_SIZE, L"");
		return TRUE;

	      case EN_KILLFOCUS:
		if (GetDlgItemInt(hWnd, IDC_EDT_SIZE, NULL, FALSE) == 0)
		  SetDlgItemText(hWnd, IDC_EDT_SIZE,
				 TXT_CURRENT_IMAGE_FILE_SIZE);
		return TRUE;
	      }
	    return TRUE;

	  case IDC_EDT_IMAGE_OFFSET:
	    switch (HIWORD(wParam))
	      {
	      case EN_KILLFOCUS:
		if (GetDlgItemInt(hWnd, IDC_EDT_IMAGE_OFFSET, NULL, FALSE) ==
		    0)
		  SetDlgItemText(hWnd, IDC_EDT_IMAGE_OFFSET, L"0");
		return TRUE;
	      }
	    return TRUE;
	  }

	return TRUE;
      }

    case WM_CLOSE:
      EndDialog(hWnd, IMDISK_AUTO_DEVICE_NUMBER);
      return TRUE;
    }

  return FALSE;
}

INT_PTR
CALLBACK
ExtendDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/)
{
  switch (uMsg)
    {
    case WM_INITDIALOG:
      CheckDlgButton(hWnd, IDC_UNIT_B, BST_CHECKED);
      return TRUE;

    case WM_COMMAND:
      {
	if (GetDlgItemInt(hWnd, IDC_EDT_EXTEND_SIZE, NULL, FALSE) > 0)
	  EnableWindow(GetDlgItem(hWnd, IDOK), TRUE);
	else
	  EnableWindow(GetDlgItem(hWnd, IDOK), FALSE);

	switch (LOWORD(wParam))
	  {
	  case IDOK:
	    {
	      EnableWindow(hWnd, FALSE);
	      HWND hWndStatus =
		CreateDialog(hInstance, MAKEINTRESOURCE(IDD_DLG_STATUS), hWnd,
			     StatusDlgProc);

	      LVITEM lvi = { 0 };
	      lvi.iItem = (int)
		SendDlgItemMessage(GetParent(hWnd), IDC_LISTVIEW,
				   LVM_GETNEXTITEM, (WPARAM) -1,
				   MAKELPARAM((UINT) LVNI_SELECTED, 0));

	      if (lvi.iItem == -1)
		{
		  EndDialog(hWnd, IDCANCEL);
		  return TRUE;
		}

	      lvi.iSubItem = 0;
	      lvi.mask = LVIF_PARAM;
	      SendDlgItemMessage(GetParent(hWnd), IDC_LISTVIEW, LVM_GETITEM, 0,
				 (LPARAM) &lvi);

	      LARGE_INTEGER extend_size;
	      extend_size.QuadPart =
		GetDlgItemInt(hWnd, IDC_EDT_EXTEND_SIZE, NULL, FALSE);

	      if (IsDlgButtonChecked(hWnd, IDC_UNIT_GB))
		extend_size.QuadPart <<= 30;
	      else if (IsDlgButtonChecked(hWnd, IDC_UNIT_MB))
		extend_size.QuadPart <<= 20;
	      else if (IsDlgButtonChecked(hWnd, IDC_UNIT_KB))
		extend_size.QuadPart <<= 10;
	      else if (IsDlgButtonChecked(hWnd, IDC_UNIT_BLOCKS))
		extend_size.QuadPart <<= 9;

	      BOOL status =
		ImDiskExtendDevice(GetDlgItem(hWndStatus, IDC_STATUS_MSG),
				   (DWORD) lvi.lParam,
				   &extend_size);

	      Sleep(100);

	      EnableWindow(hWnd, TRUE);
	      DestroyWindow(hWndStatus);

	      if (status)
		EndDialog(hWnd, IDOK);
	      else
		EndDialog(hWnd, IDCANCEL);

	      return TRUE;
	    }

	  case IDCANCEL:
	    EndDialog(hWnd, IDCANCEL);
	    return TRUE;
	  }
	return TRUE;
      }

    case WM_CLOSE:
      EndDialog(hWnd, IDCANCEL);
      return TRUE;
    }

  return FALSE;
}

INT_PTR
CALLBACK
CPlAppletDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
    {
    case WM_INITDIALOG:
      {
	SetClassLongPtr(hWnd, GCLP_HICON,
			(LONG_PTR) LoadIcon(hInstance,
					    MAKEINTRESOURCE(IDI_APPICON)));

	HKEY hKeyMountPoints2;
	DWORD dwErrCode = RegOpenKey(HKEY_CURRENT_USER,
				     KEY_NAME_HKEY_MOUNTPOINTS2,
				     &hKeyMountPoints2);
	if (dwErrCode == NO_ERROR)
	  SetProp(hWnd, PROP_NAME_HKEY_MOUNTPOINTS2, hKeyMountPoints2);

	HWND hWndListView = GetDlgItem(hWnd, IDC_LISTVIEW);

	LVCOLUMN lvc = { 0 };
	lvc.mask = LVCF_WIDTH | LVCF_TEXT;
	lvc.cx = 40;
	lvc.pszText = L"Drive";

	ListView_InsertColumn(hWndListView, 0, &lvc);

	lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.cx = 255;
	lvc.pszText = L"Image file";

	ListView_InsertColumn(hWndListView, 1, &lvc);

	lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.cx = 60;
	lvc.pszText = L"Size";

	ListView_InsertColumn(hWndListView, 2, &lvc);

	lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.cx = 135;
	lvc.pszText = L"Properties";

	ListView_InsertColumn(hWndListView, 3, &lvc);

	lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.cx = 60;
	lvc.pszText = L"Filesystem";

	ListView_InsertColumn(hWndListView, 4, &lvc);

	HIMAGELIST hImageList =
	  ImageList_Create(GetSystemMetrics(SM_CXSMICON),
			   GetSystemMetrics(SM_CYSMICON),
			   ILC_COLOR8 | ILC_MASK, 1, 1);

	ImageList_AddIcon(hImageList,
			  LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICONHD)));
	ImageList_AddIcon(hImageList,
			  LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICONFD)));
	ImageList_AddIcon(hImageList,
			  LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICONCD)));

	ListView_SetImageList(hWndListView, hImageList, LVSIL_SMALL);

	RefreshList(hWndListView, IMDISK_AUTO_DEVICE_NUMBER);

	return TRUE;
      }

    case WM_NOTIFY:
      switch ((int) wParam)
	{
	case IDC_LISTVIEW:
	  switch (((LPNMHDR) lParam)->code)
	    {
	    case NM_CUSTOMDRAW:
	      {
		BOOL item_selected =
		  SendDlgItemMessage(hWnd, IDC_LISTVIEW, LVM_GETNEXTITEM,
				     (WPARAM) -1,
				     MAKELPARAM((UINT) LVNI_SELECTED, 0)) !=
		  -1;

		EnableMenuItem(GetMenu(hWnd), CM_CPL_APPLET_SELECTED,
			       (item_selected ? MF_ENABLED : MF_GRAYED) |
			       MF_BYCOMMAND);

		EnableWindow(GetDlgItem(hWnd, CM_CPL_APPLET_SELECTED_UNMOUNT),
			     item_selected);
		EnableWindow(GetDlgItem(hWnd,
					CM_CPL_APPLET_SELECTED_EXTEND_SIZE),
			     item_selected);
		EnableWindow(GetDlgItem(hWnd, CM_CPL_APPLET_SELECTED_FORMAT),
			     item_selected);
		EnableWindow(GetDlgItem(hWnd,
					CM_CPL_APPLET_SELECTED_SAVE_IMAGE),
			     item_selected);
	      }

	      DrawMenuBar(hWnd);

	      return TRUE;

	    case NM_RCLICK:
	      {
		INT_PTR item_selected =
		  SendDlgItemMessage(hWnd, IDC_LISTVIEW, LVM_GETNEXTITEM,
				     (WPARAM) -1,
				     MAKELPARAM((UINT) LVNI_SELECTED, 0));
		if (item_selected == -1)
		  return TRUE;

		POINT item_pos = { 0, 0 };
		SendDlgItemMessage(hWnd, IDC_LISTVIEW, LVM_GETITEMPOSITION,
				   (WPARAM) item_selected, (LPARAM) &item_pos);

		item_pos.x = 35;

		MapWindowPoints(GetDlgItem(hWnd, IDC_LISTVIEW), NULL,
				&item_pos, 1);
		
		TrackPopupMenu(GetSubMenu(GetMenu(hWnd), 1), TPM_RIGHTBUTTON,
			       item_pos.x, item_pos.y, 0, hWnd, NULL);
	      }
	      return TRUE;

	    case NM_DBLCLK:
	      wParam = IDOK;
	      break;

	    case LVN_KEYDOWN:
	      switch (((LV_KEYDOWN *) lParam)->wVKey)
		{
		  // The Delete and F5 keys in the listview translates into
		  // buttons/menu items and then passed to the WM_COMMAND
		  // case handler following this WM_NOTIFY case handler.
		case VK_DELETE:
		  wParam = CM_CPL_APPLET_SELECTED_UNMOUNT;
		  break;

		case VK_F5:
		  wParam = CM_CPL_APPLET_WINDOW_REFRESH;
		  break;

		default:
		  return FALSE;
		}

	      break;

	    default:
	      return FALSE;
	    }

	  break;

	default:
	  return TRUE;
	}

    case WM_COMMAND:
      switch (LOWORD(wParam))
	{
	case CM_CPL_APPLET_FILE_MOUNT_NEW:
	  {
	    DWORD device_number = (DWORD)
	      DialogBox(hInstance, MAKEINTRESOURCE(IDD_NEWDIALOG), hWnd,
			NewDlgProc);
	    if (device_number != IMDISK_AUTO_DEVICE_NUMBER)
	      RefreshList(GetDlgItem(hWnd, IDC_LISTVIEW), device_number);

	    return TRUE;
	  }

	case IDCANCEL:
	case CM_CPL_APPLET_FILE_EXIT:
	  EndDialog(hWnd, IDOK);

	  return TRUE;

	case IDOK:
	case CM_CPL_APPLET_SELECTED_OPEN:
	  {
	    WCHAR mount_point[3] = L"";
	    LVITEM lvi = { 0 };
	    lvi.iItem = (int)
	      SendDlgItemMessage(hWnd, IDC_LISTVIEW, LVM_GETNEXTITEM,
				 (WPARAM) -1,
				 MAKELPARAM((UINT) LVNI_SELECTED, 0));

	    if (lvi.iItem == -1)
	      return TRUE;
	    
	    lvi.iSubItem = 0;
	    lvi.mask = LVIF_TEXT;
	    lvi.pszText = mount_point;
	    lvi.cchTextMax = sizeof(mount_point) / sizeof(*mount_point);
	    SendDlgItemMessage(hWnd, IDC_LISTVIEW, LVM_GETITEM, 0,
			       (LPARAM) &lvi);

	    if (mount_point[0] == 0)
	      return TRUE;

	    HKEY hKeyMountPoints2 = (HKEY)
	      GetProp(hWnd, PROP_NAME_HKEY_MOUNTPOINTS2);
	    if (hKeyMountPoints2 != NULL)
	      {
		WCHAR mount_key_name[] = { mount_point[0], 0 };
		HKEY hKeyMountPoint;
		HWND hWndStatus = NULL;
		bool bCancelFlag = false;

		for (DWORD dwStartTime = GetTickCount();
		     (GetTickCount() - dwStartTime) < 5000;
		     Sleep(100), DoEvents(NULL))
		  {
		    DWORD dwErrCode = RegOpenKey(hKeyMountPoints2,
						 mount_key_name,
						 &hKeyMountPoint);
		    if (dwErrCode == NO_ERROR)
		      {
			RegCloseKey(hKeyMountPoint);
			break;
		      }

		    if (bCancelFlag)
		      break;

		    if (hWndStatus == NULL)
		      {
			EnableWindow(hWnd, FALSE);
			hWndStatus =
			  CreateDialogParam(hInstance,
					    MAKEINTRESOURCE(IDD_DLG_STATUS),
					    hWnd,
					    StatusDlgProc,
					    (LPARAM) &bCancelFlag);
			SetDlgItemText(hWndStatus,
				       IDC_STATUS_MSG,
				       L"Waiting for Windows Explorer...");
		      }
		  }

		if (hWndStatus != NULL)
		  {
		    DestroyWindow(hWndStatus);
		    EnableWindow(hWnd, TRUE);
		  }
	      }

	    if ((INT_PTR) ShellExecute(hWnd, L"open", mount_point, NULL,
					  NULL, SW_SHOWNORMAL) <= 32)
	      MessageBox(hWnd,
			 L"Cannot open the drive. Check that the drive is "
			 L"formatted with a compatible filesystem and that it "
			 L"is not locked by another process.",
			 L"ImDisk Virtual Disk Driver",
			 MB_ICONEXCLAMATION);
	  }

	  return TRUE;

	case CM_CPL_APPLET_SELECTED_UNMOUNT:
	  {
	    LVITEM lvi = { 0 };
	    lvi.mask = LVIF_TEXT | LVIF_PARAM;
	    lvi.iItem = (int)
	      SendDlgItemMessage(hWnd, IDC_LISTVIEW, LVM_GETNEXTITEM,
				 (WPARAM) -1,
				 MAKELPARAM((UINT) LVNI_SELECTED, 0));
	    if (lvi.iItem == -1)
	      return TRUE;

	    lvi.iSubItem = 0;
	    WCHAR wcBuffer[3] = L"";
	    lvi.pszText = wcBuffer;
	    lvi.cchTextMax = sizeof(wcBuffer)/sizeof(*wcBuffer);
	    SendDlgItemMessage(hWnd, IDC_LISTVIEW, LVM_GETITEM, 0,
			       (LPARAM) &lvi);

	    EnableWindow(hWnd, FALSE);
	    HWND hWndStatus =
	      CreateDialog(hInstance, MAKEINTRESOURCE(IDD_DLG_STATUS), hWnd,
			   StatusDlgProc);

	    ImDiskRemoveDevice(GetDlgItem(hWndStatus, IDC_STATUS_MSG),
			       (DWORD) lvi.lParam,
			       lvi.pszText[0] == 0 ? NULL : lvi.pszText);

	    Sleep(100);

	    EnableWindow(hWnd, TRUE);
	    DestroyWindow(hWndStatus);

	    DoEvents(NULL);

	    RefreshList(GetDlgItem(hWnd, IDC_LISTVIEW),
			IMDISK_AUTO_DEVICE_NUMBER);

	    return TRUE;
	  }

	case CM_CPL_APPLET_SELECTED_SET_RO:
	case CM_CPL_APPLET_SELECTED_SET_RW:
	case CM_CPL_APPLET_SELECTED_SET_REM:
	case CM_CPL_APPLET_SELECTED_SET_FIXED:
	  {
	    LVITEM lvi = { 0 };
	    lvi.mask = LVIF_TEXT | LVIF_PARAM;
	    lvi.iItem = (int)
	      SendDlgItemMessage(hWnd, IDC_LISTVIEW, LVM_GETNEXTITEM,
				 (WPARAM) -1,
				 MAKELPARAM((UINT) LVNI_SELECTED, 0));
	    if (lvi.iItem == -1)
	      return TRUE;

	    lvi.iSubItem = 0;
	    WCHAR wcBuffer[3] = L"";
	    lvi.pszText = wcBuffer;
	    lvi.cchTextMax = sizeof(wcBuffer)/sizeof(*wcBuffer);
	    SendDlgItemMessage(hWnd, IDC_LISTVIEW, LVM_GETITEM, 0,
			       (LPARAM) &lvi);

	    EnableWindow(hWnd, FALSE);
	    HWND hWndStatus =
	      CreateDialog(hInstance, MAKEINTRESOURCE(IDD_DLG_STATUS), hWnd,
			   StatusDlgProc);

	    DWORD flags_to_change = 0;
	    DWORD flags = 0;

	    switch (LOWORD(wParam))
	      {
	      case CM_CPL_APPLET_SELECTED_SET_RO:
		flags = IMDISK_OPTION_RO;
	      case CM_CPL_APPLET_SELECTED_SET_RW:
		flags_to_change = IMDISK_OPTION_RO;
		break;

	      case CM_CPL_APPLET_SELECTED_SET_REM:
		flags = IMDISK_OPTION_REMOVABLE;
	      case CM_CPL_APPLET_SELECTED_SET_FIXED:
		flags_to_change = IMDISK_OPTION_REMOVABLE;
		break;
	      }

	    ImDiskChangeFlags(GetDlgItem(hWndStatus, IDC_STATUS_MSG),
			      (DWORD) lvi.lParam,
			      lvi.pszText[0] == 0 ? NULL : lvi.pszText,
			      flags_to_change, flags);

	    Sleep(100);

	    EnableWindow(hWnd, TRUE);
	    DestroyWindow(hWndStatus);

	    DoEvents(NULL);

	    RefreshList(GetDlgItem(hWnd, IDC_LISTVIEW),
			IMDISK_AUTO_DEVICE_NUMBER);

	    return TRUE;
	  }

	case CM_CPL_APPLET_SELECTED_EXTEND_SIZE:
	  if (DialogBox(hInstance, MAKEINTRESOURCE(IDD_DLG_EXTEND), hWnd,
			ExtendDlgProc) == IDOK)
	    RefreshList(GetDlgItem(hWnd, IDC_LISTVIEW),
			IMDISK_AUTO_DEVICE_NUMBER);

	  return TRUE;

	case CM_CPL_APPLET_SELECTED_FORMAT:
	  {
	    WCHAR mount_point[3] = L"";
	    LVITEM lvi = { 0 };
	    lvi.iItem = (int)
	      SendDlgItemMessage(hWnd, IDC_LISTVIEW, LVM_GETNEXTITEM,
				 (WPARAM) -1,
				 MAKELPARAM((UINT) LVNI_SELECTED, 0));

	    if (lvi.iItem == -1)
	      return TRUE;
	    
	    lvi.iSubItem = 0;
	    lvi.mask = LVIF_TEXT;
	    lvi.pszText = mount_point;
	    lvi.cchTextMax = sizeof(mount_point) / sizeof(*mount_point);
	    SendDlgItemMessage(hWnd, IDC_LISTVIEW, LVM_GETITEM, 0,
			       (LPARAM) &lvi);

	    if ((mount_point[0] < 'A') | (mount_point[0] > 'Z'))
	      {
		MsgBoxPrintF(hWnd, MB_ICONSTOP, L"Unsupported mount point",
			     L"It is only possible to format drives with a "
			     L"drive letter A: - Z:. This mount point, '%1' "
			     L"is not supported.", mount_point);
		return TRUE;
	      }

	    SHFormatDrive(hWnd, (mount_point[0] & 0x1F) - 1, SHFMT_ID_DEFAULT,
			  SHFMT_OPT_FULL);

	    RefreshList(GetDlgItem(hWnd, IDC_LISTVIEW),
			IMDISK_AUTO_DEVICE_NUMBER);
	  }

	  return TRUE;

	case CM_CPL_APPLET_SELECTED_SAVE_IMAGE:
	  SaveSelectedDeviceToImageFile(hWnd);

	  return TRUE;

	case CM_CPL_APPLET_WINDOW_REFRESH:
	  RefreshList(GetDlgItem(hWnd, IDC_LISTVIEW),
		      IMDISK_AUTO_DEVICE_NUMBER);

	  return TRUE;

	case CM_CPL_APPLET_HELP_ABOUT:
	  DisplayAboutBox
	    (hWnd,
	     L"About ImDisk Virtual Disk Driver"
	     L"|"  // Boundary dialog caption text/textbox text
	     L"ImDisk Virtual Disk Driver for Windows NT/2000/XP/2003.\r\n"
	     L"Version %1!i!.%2!i!.%3!i! - (Compiled %4!hs!)\r\n"
	     L"\r\n"
	     L"Copyright (C) 2004-2010 Olof Lagerkvist.\r\n"
	     L"http://www.ltr-data.se     olof@ltr-data.se\r\n"
	     L"\r\n"
	     L"Permission is hereby granted, free of charge, to any person\r\n"
	     L"obtaining a copy of this software and associated documentation\r\n"
	     L"files (the \"Software\"), to deal in the Software without\r\n"
	     L"restriction, including without limitation the rights to use,\r\n"
	     L"copy, modify, merge, publish, distribute, sublicense, and/or\r\n"
	     L"sell copies of the Software, and to permit persons to whom the\r\n"
	     L"Software is furnished to do so, subject to the following\r\n"
	     L"conditions:\r\n"
	     L"\r\n"
	     L"The above copyright notice and this permission notice shall be\r\n"
	     L"included in all copies or substantial portions of the Software.\r\n"
	     L"\r\n"
	     L"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND,\r\n"
	     L"EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES\r\n"
	     L"OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND\r\n"
	     L"NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT\r\n"
	     L"HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,\r\n"
	     L"WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING\r\n"
	     L"FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR\r\n"
	     L"OTHER DEALINGS IN THE SOFTWARE.\r\n"
	     L"\r\n"
	     L"This program contains some GNU GPL licensed code:\r\n"
	     L"- Parts related to floppy emulation based on VFD by Ken Kato.\r\n"
	     L"  http://chitchat.at.infoseek.co.jp/vmware/vfd.html\r\n"
	     L"Copyright (C) Free Software Foundation, Inc.\r\n"
	     L"Read gpl.txt for the full GNU GPL license.\r\n"
	     L"\r\n"
	     L"This program may contain BSD licensed code:\r\n"
	     L"- Some code ported to NT from the FreeBSD md driver by Olof Lagerkvist.\r\n"
	     L"  http://www.ltr-data.se\r\n"
	     L"Copyright (C) The FreeBSD Project.\r\n"
	     L"Copyright (C) The Regents of the University of California.\r\n",
	     (int) (IMDISK_VERSION & 0xFF00) >> 8,
	     (int) (IMDISK_VERSION & 0xF0) >> 4,
	     (int) IMDISK_VERSION & 0xF,
	     __DATE__);

	  return TRUE;
	}

      return TRUE;

    case WM_NCDESTROY:
      {
	HKEY hKeyMountPoints2 = (HKEY)
	  GetProp(hWnd, PROP_NAME_HKEY_MOUNTPOINTS2);

	if (hKeyMountPoints2 != NULL)
	  {
	    RegCloseKey(hKeyMountPoints2);
	    RemoveProp(hWnd, PROP_NAME_HKEY_MOUNTPOINTS2);
	  }

	return TRUE;
      }

    case WM_CLOSE:
      EndDialog(hWnd, IDOK);
      return TRUE;
    }

  return FALSE;
}

EXTERN_C LONG APIENTRY
CPlApplet(HWND hwndCPl,	        // handle to Control Panel window
	  UINT uMsg,	        // message
	  LPARAM /*lParam1*/,	// first message parameter
	  LPARAM lParam2 	// second message parameter
	  )
{
  switch (uMsg)
    {
    case CPL_DBLCLK:
      if (DialogBox(hInstance, MAKEINTRESOURCE(IDD_CPLAPPLET), hwndCPl,
		    CPlAppletDlgProc) == -1)
	MsgBoxLastError(hwndCPl,
			L"Error loading dialog box:");

      return 0;

    case CPL_EXIT:
      return 0;

    case CPL_GETCOUNT:
      return 1;

    case CPL_INIT:
      return 1;

    case CPL_INQUIRE:
      {
        LPCPLINFO lpcpli = (LPCPLINFO)lParam2;
        lpcpli->idIcon = IDI_APPICON;
        lpcpli->idName = IDS_CPLAPPLET_TITLE;
        lpcpli->idInfo = IDS_CPLAPPLET_DESCRIPTION;
        lpcpli->lData = 0;
        return 0;
      }

    case CPL_STOP:
      return 0;

    default:
      return 1;
    }
}

EXTERN_C BOOL WINAPI
DllMain(HINSTANCE hinstDLL, DWORD, LPVOID)
{
  hInstance = hinstDLL;
  return TRUE;
}
