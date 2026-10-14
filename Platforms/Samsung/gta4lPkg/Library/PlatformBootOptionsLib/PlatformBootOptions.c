/*
 * gta4l boot options: the Qualcomm Mass Storage app (hidden, as on the other
 * Qualcomm devices) and the UEFI Shell, put first in the boot order so the
 * tablet opens the Shell when it starts. A USB keyboard on the OTG port
 * works there.
 */
#include <Uefi.h>
#include <Guid/FileInfo.h>
#include <Guid/PcAnsi.h>
#include <Protocol/SerialIo.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/PlatformBootOptionsLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/LoadedImage.h>

extern EFI_GUID gUefiShellFileGuid;

STATIC
VOID
RegisterShellFirst (VOID)
{
  EFI_STATUS                         Status;
  EFI_LOADED_IMAGE_PROTOCOL         *LoadedImage = NULL;
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH  FileNode;
  EFI_DEVICE_PATH_PROTOCOL          *DevicePath;
  EFI_BOOT_MANAGER_LOAD_OPTION       Option;
  EFI_BOOT_MANAGER_LOAD_OPTION      *Options;
  UINTN                              Count = 0;
  INTN                               Index;

  Status = gBS->HandleProtocol (gImageHandle, &gEfiLoadedImageProtocolGuid, (VOID **)&LoadedImage);
  if (EFI_ERROR (Status)) {
    return;
  }

  EfiInitializeFwVolDevicepathNode (&FileNode, &gUefiShellFileGuid);
  DevicePath = AppendDevicePathNode (DevicePathFromHandle (LoadedImage->DeviceHandle),
                                     (EFI_DEVICE_PATH_PROTOCOL *)&FileNode);
  if (DevicePath == NULL) {
    return;
  }

  Status = EfiBootManagerInitializeLoadOption (&Option, LoadOptionNumberUnassigned,
                                               LoadOptionTypeBoot, LOAD_OPTION_ACTIVE,
                                               L"UEFI Shell", DevicePath, NULL, 0);
  FreePool (DevicePath);
  if (EFI_ERROR (Status)) {
    return;
  }

  Options = EfiBootManagerGetLoadOptions (&Count, LoadOptionTypeBoot);
  Index   = EfiBootManagerFindLoadOption (&Option, Options, Count);

  // Already there (variables kept): move it to the front by re-adding
  if (Index != -1) {
    EfiBootManagerDeleteLoadOptionVariable (Options[Index].OptionNumber, LoadOptionTypeBoot);
  }

  Status = EfiBootManagerAddLoadOptionVariable (&Option, 0);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "%a: Failed to add the UEFI Shell boot option! Status = %r\n", __FUNCTION__, Status));
  }

  EfiBootManagerFreeLoadOption (&Option);
  EfiBootManagerFreeLoadOptions (Options, Count);
}

/*
 * Put the UART on the console too (VT100+, 115200 8N1), so the Shell can be
 * used from a serial terminal when no keyboard works.
 */
STATIC
VOID
RegisterSerialConsole (VOID)
{
  EFI_STATUS                Status;
  EFI_HANDLE               *Handles = NULL;
  UINTN                     Count   = 0;
  UINTN                     Index;
  EFI_DEVICE_PATH_PROTOCOL *Path;
  VENDOR_DEVICE_PATH        Terminal;

  Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiSerialIoProtocolGuid, NULL, &Count, &Handles);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_WARN, "%a: No serial port! Status = %r\n", __FUNCTION__, Status));
    return;
  }

  Terminal.Header.Type    = MESSAGING_DEVICE_PATH;
  Terminal.Header.SubType = MSG_VENDOR_DP;
  SetDevicePathNodeLength (&Terminal.Header, sizeof (VENDOR_DEVICE_PATH));
  CopyGuid (&Terminal.Guid, &gEfiVT100PlusGuid);

  for (Index = 0; Index < Count; Index++) {
    Path = AppendDevicePathNode (DevicePathFromHandle (Handles[Index]), (EFI_DEVICE_PATH_PROTOCOL *)&Terminal);
    if (Path == NULL) {
      continue;
    }

    EfiBootManagerUpdateConsoleVariable (ConIn,  Path, NULL);
    EfiBootManagerUpdateConsoleVariable (ConOut, Path, NULL);
    EfiBootManagerUpdateConsoleVariable (ErrOut, Path, NULL);
    FreePool (Path);
  }

  FreePool (Handles);
}

/*
 * Any USB keyboard (class 3, subclass 1, protocol 1) is a console input,
 * so ConPlatform hands it to the console when it is plugged in.
 */
#pragma pack(1)
typedef struct {
  USB_CLASS_DEVICE_PATH     Keyboard;
  EFI_DEVICE_PATH_PROTOCOL  End;
} USB_KEYBOARD_DEVICE_PATH;
#pragma pack()

STATIC USB_KEYBOARD_DEVICE_PATH mUsbKeyboard = {
  {
    { MESSAGING_DEVICE_PATH, MSG_USB_CLASS_DP, { sizeof (USB_CLASS_DEVICE_PATH), 0 } },
    0xFFFF, 0xFFFF, 3, 1, 1
  },
  { END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE, { sizeof (EFI_DEVICE_PATH_PROTOCOL), 0 } }
};

STATIC
VOID
RegisterUsbKeyboard (VOID)
{
  EfiBootManagerUpdateConsoleVariable (ConIn, (EFI_DEVICE_PATH_PROTOCOL *)&mUsbKeyboard, NULL);
}

VOID
GetPlatformBootOptions (
  OUT EFI_PLATFORM_BOOT_OPTION **BootOption,
  OUT UINT8                     *BootOptionCount)
{
  EFI_PLATFORM_BOOT_OPTION *LocalBootOption;

  RegisterShellFirst ();
  RegisterSerialConsole ();
  RegisterUsbKeyboard ();

  LocalBootOption = AllocateZeroPool (sizeof (EFI_PLATFORM_BOOT_OPTION));
  if (LocalBootOption == NULL) {
    *BootOption      = NULL;
    *BootOptionCount = 0;
    return;
  }

  // Add Mass Storage Boot Option
  LocalBootOption[0].AppName       = L"Mass Storage";
  LocalBootOption[0].AppGuid       = FixedPcdGetPtr (PcdMassStorageFile);
  LocalBootOption[0].LoadAttribute = LOAD_OPTION_HIDDEN;

  *BootOption      = LocalBootOption;
  *BootOptionCount = 1;
}
