/**
  Copyright (C) DuoWoA authors. All rights reserved.
  Copyright (C) Microsoft Corporation. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Library/AcpiTableUpdateLib.h>
#include <Library/DeviceBootManagerLib.h>
#include <Library/PlatformBootManagerLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/SecureBootHandlerLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/MsBootOptionsLib.h>
#include <Library/BootGraphicsLib.h>
#include <Library/BootGraphics.h>
#include <Library/PrintLib.h>
#include <Library/DevicePathLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/UefiLib.h>
#include <Protocol/BlockIo.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/UsbIo.h>

#include <Configuration/BootDevices.h>

#include "BootManager.h"

//
// Global Variables
//
STATIC UINTN XPos = 0;
STATIC UINTN YPos = 0;

//
// BDS Combo Message
//
STATIC CONST CHAR8 *ComboMessage = "[Vol+ / Esc] Boot Menu    [Vol- / F2] Setup";

//
// QCOM_USB_CONFIG_PROTOCOL GUID (E722B03F-B250-42CE-8EBD-5BD51812D037)
//
STATIC EFI_GUID mQcomUsbConfigProtocolGuid = {
  0xE722B03F, 0xB250, 0x42CE, { 0x8E, 0xBD, 0x5B, 0xD5, 0x18, 0x12, 0xD0, 0x37 }
};

typedef
EFI_STATUS
(EFIAPI *QCOM_USB_ENABLE_VBUS) (
  IN VOID    *This,
  IN UINT32   CoreNum
  );

/*
 * Connect all handles that carry the given protocol.
 */
STATIC
VOID
ConnectControllersByProtocol (IN EFI_GUID *ProtocolGuid)
{
  EFI_HANDLE *Handles = NULL;
  UINTN       Count   = 0;
  UINTN       Index;

  if (EFI_ERROR (gBS->LocateHandleBuffer (ByProtocol, ProtocolGuid, NULL, &Count, &Handles))) {
    return;
  }

  for (Index = 0; Index < Count; Index++) {
    gBS->ConnectController (Handles[Index], NULL, NULL, TRUE);
  }

  FreePool (Handles);
}

/*
 * Switch the Qualcomm USB controller on gta4l from peripheral-only static
 * configuration (modeType = 4 on platform 0x1D) to XHCI host mode
 * (modeType = 1, initMode = 8), enable 5V OTG VBUS, and signal
 * gUsbControllerInitGuid via DeviceBootManagerOnDemandConInConnect() so
 * XhciPciEmulation, XhciDxe, UsbBusDxe, and UsbKbDxe enumerate OTG devices
 * before BdsDxe connects the default consoles.
 */
STATIC
VOID
StartUsbHost (VOID)
{
  EFI_STATUS            Status;
  VOID                 *UsbConfigProtocol = NULL;
  UINT8                *Dev;
  VOID                 *UsbCoreIfc;
  QCOM_USB_ENABLE_VBUS  EnableVbus;

  Status = gBS->LocateProtocol (&mQcomUsbConfigProtocolGuid, NULL, &UsbConfigProtocol);
  if (!EFI_ERROR (Status) && UsbConfigProtocol != NULL) {
    Dev = (UINT8 *)UsbConfigProtocol - 0x100;

    // Verify gUsbConfigDevice[0] signature ('usbc')
    if (*(UINT32 *)Dev == SIGNATURE_32 ('u', 's', 'b', 'c')) {
      UsbCoreIfc = *(VOID **)(Dev + 0x18);
      if (UsbCoreIfc != NULL) {
        *(UINT32 *)UsbCoreIfc = 8; // USB_HOST_MODE
      }

      Dev[0xD0]               = 1; // EnableStaticCfg = TRUE
      *(UINT32 *)(Dev + 0xD4) = 1; // USB_HOST_MODE_XHCI

      // Prevent UsbConfigLibExitBootService (0x39b4) from putting USB into LPM on ExitBootServices
      // so Windows PE (usbxhci.sys) and Linux keep XHCI clocks and 5V OTG VBUS active
      *(Dev - 0x4DDC)         = 1;
    }

    // Allow Usb30EnableVbus / Usb30GetVbusSt to call UsbPwrCtrlDxe on platform 0x1D
    if (*(UINT32 *)((UINT8 *)UsbConfigProtocol + 0x268) == 0x1D) {
      *(UINT32 *)((UINT8 *)UsbConfigProtocol + 0x268) = 0x0B;
    }

    EnableVbus = *(QCOM_USB_ENABLE_VBUS *)((UINT8 *)UsbConfigProtocol + 0x98);
    if (EnableVbus != NULL) {
      EnableVbus (UsbConfigProtocol, 0);
    }
  }

  // Signal gUsbControllerInitGuid (skipped by BdsDxe when PcdConInConnectOnDemand is FALSE)
  DeviceBootManagerOnDemandConInConnect ();
}

VOID
EFIAPI
PlatformBootManagerBeforeConsole ()
{
  // Signal End Of DXE
  EfiEventGroupSignal (&gEfiEndOfDxeEventGroupGuid);

  // Dispatch Deferred Images
  EfiBootManagerDispatchDeferredImages ();

  // Start USB Host Mode (XHCI) and trigger early SD card initialization
  StartUsbHost ();
  DeviceBootManagerAfterConsole (); // Signals gSdCardInitGuid early so SdccDxe powers up SD card during console setup
  ConnectControllersByProtocol (&gEfiFirmwareVolume2ProtocolGuid);
  ConnectControllersByProtocol (&gEfiUsbIoProtocolGuid);

  // Register Boot Options
  MsBootOptionsLibRegisterDefaultBootOptions ();

  // Register Console Variables
  EfiBootManagerUpdateConsoleVariable (ConIn,  (EFI_DEVICE_PATH_PROTOCOL *)&KeypadDevicePath,  NULL);
  EfiBootManagerUpdateConsoleVariable (ConOut, (EFI_DEVICE_PATH_PROTOCOL *)&DisplayDevicePath, NULL);
  EfiBootManagerUpdateConsoleVariable (ErrOut, (EFI_DEVICE_PATH_PROTOCOL *)&DisplayDevicePath, NULL);

  // Execute Secondary Before Console
  DeviceBootManagerBeforeConsole (NULL, NULL);
}

VOID
PostGopSetup (OUT EFI_GRAPHICS_OUTPUT_PROTOCOL **GopProtocol)
{
  EFI_STATUS                    Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL *LocalGopProtocol;

  // Verify Parameters
  if (GopProtocol == NULL) {
    return;
  }

  // Locate GOP Protocol
  Status = gBS->HandleProtocol (gST->ConsoleOutHandle, &gEfiGraphicsOutputProtocolGuid, (VOID *)&LocalGopProtocol);
  if (EFI_ERROR (Status) && Status != EFI_UNSUPPORTED) {
    DEBUG ((EFI_D_ERROR, "%a: Failed to Locate GOP Protocol from Handle! Status = %r\n", __FUNCTION__, Status));
    return;
  }

  // Check Status
  if (Status == EFI_UNSUPPORTED) {
    // Locate GOP Protocol
    Status = gBS->LocateProtocol (&gEfiGraphicsOutputProtocolGuid, NULL, (VOID *)&LocalGopProtocol);
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "%a: Failed to Locate GOP Protocol! Status = %r\n", __FUNCTION__, Status));
      return;
    }

    // Install GOP Protocol
    Status = gBS->InstallProtocolInterface (&gST->ConsoleOutHandle, &gEfiGraphicsOutputProtocolGuid, EFI_NATIVE_INTERFACE, LocalGopProtocol);
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "%a: Failed to Install GOP Protocol onto \"ConsoleOutHandle\"! Status = %r\n", __FUNCTION__, Status));
    }
  }

  // Pass GOP Protocol
  *GopProtocol = LocalGopProtocol;
}

VOID
EFIAPI
PlatformBootManagerAfterConsole ()
{
  EFI_STATUS                    Status      = EFI_SUCCESS;
  EFI_GRAPHICS_OUTPUT_PROTOCOL *GopProtocol = NULL;

  // Post Setup GOP
  PostGopSetup (&GopProtocol);

  // Display Boot Logo
  Status = DisplayBootGraphic (BG_SYSTEM_LOGO);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "%a: Failed to Display Boot Logo! Status = %r\n", __FUNCTION__, Status));
  }

  // Register Key Callback
  Status = RegisterKeyCallback ((EFI_DEVICE_PATH_PROTOCOL *)&KeypadDevicePath);
  if (!EFI_ERROR (Status) && GopProtocol != NULL) {
    // Set Screen Resolution
    UINT32 ScreenWidth  = GopProtocol->Mode->Info->HorizontalResolution;
    UINT32 ScreenHeight = GopProtocol->Mode->Info->VerticalResolution;

    // Set Combo Message Position
    XPos = (ScreenWidth - AsciiStrLen (ComboMessage) * EFI_GLYPH_WIDTH) / 2;
    YPos = ScreenHeight * 48 / 50;
  }

  // gSdCardInitGuid was already signalled in PlatformBootManagerBeforeConsole;
  // give SdccDxe time to finish card identification (CMD0/CMD8/ACMD41)
  gBS->Stall (100000);

  // Pass 1: Connect raw disk devices (SD card, USB mass storage, eMMC)
  ConnectControllersByProtocol (&gEfiBlockIoProtocolGuid);
  // Pass 2: Connect partition handles produced by PartitionDxe to filesystem drivers
  ConnectControllersByProtocol (&gEfiBlockIoProtocolGuid);
  ConnectControllersByProtocol (&gEfiFirmwareVolume2ProtocolGuid);
  ConnectControllersByProtocol (&gEfiUsbIoProtocolGuid);
}

VOID
EFIAPI
PlatformBootManagerOnDemandConInConnect ()
{
  // Execute Secondary On Demand
  DeviceBootManagerOnDemandConInConnect ();
}

VOID
EFIAPI
PlatformBootManagerProcessBootCompletion (OUT EFI_BOOT_MANAGER_LOAD_OPTION *BootOption)
{
  return;
}

//
// UiApp, the setup front page (Device Manager, Boot Manager, Boot Maintenance)
//
STATIC EFI_GUID mUiAppFileGuid = { 0x462CAA21, 0x7614, 0x4503, { 0x83, 0x6E, 0x8A, 0xB6, 0xF4, 0x66, 0x23, 0x31 } };

STATIC
VOID
BootSetup (VOID)
{
  EFI_STATUS                         Status;
  EFI_LOADED_IMAGE_PROTOCOL         *LoadedImage;
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH  FileNode;
  EFI_DEVICE_PATH_PROTOCOL          *DevicePath;
  EFI_BOOT_MANAGER_LOAD_OPTION       LoadOption;

  Status = gBS->HandleProtocol (gImageHandle, &gEfiLoadedImageProtocolGuid, (VOID **)&LoadedImage);
  if (EFI_ERROR (Status)) {
    return;
  }

  EfiInitializeFwVolDevicepathNode (&FileNode, &mUiAppFileGuid);
  DevicePath = AppendDevicePathNode (DevicePathFromHandle (LoadedImage->DeviceHandle), (EFI_DEVICE_PATH_PROTOCOL *)&FileNode);
  if (DevicePath == NULL) {
    return;
  }

  Status = EfiBootManagerInitializeLoadOption (&LoadOption, LoadOptionNumberUnassigned, LoadOptionTypeBoot,
                                               LOAD_OPTION_CATEGORY_APP | LOAD_OPTION_ACTIVE | LOAD_OPTION_HIDDEN,
                                               L"Setup", DevicePath, NULL, 0);
  FreePool (DevicePath);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "%a: Failed to create the Setup load option! Status = %r\n", __FUNCTION__, Status));
    return;
  }

  EfiBootManagerBoot           (&LoadOption);
  EfiBootManagerFreeLoadOption (&LoadOption);
}

VOID
EFIAPI
PlatformBootManagerPriorityBoot (IN OUT UINT16 **BootNext)
{
  EFI_STATUS                   Status;
  EFI_BOOT_MANAGER_LOAD_OPTION LoadOption;
  BOOLEAN                      BootManagerFlag;
  BOOLEAN                      SetupFlag;

  // Unregister Key Callbacks
  UnregisterKeyCallback ();

  BootManagerFlag = EnterBootManager ();
  SetupFlag       = EnterSetup ();

  // Vol- / F2: Setup, then the normal boot order
  if (SetupFlag) {
    BootSetup ();
    return;
  }

  if (BootManagerFlag == FALSE) {
    return;
  }

  // Get Boot Manager Load Option
  Status = MsBootOptionsLibGetBootManagerMenu (&LoadOption, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "%a: Failed to get Boot Manager Load Option! Status = %r", __FUNCTION__, Status));
    return;
  }

  // Execute Boot Manager Load Option
  EfiBootManagerBoot           (&LoadOption);
  EfiBootManagerFreeLoadOption (&LoadOption);
}

VOID
EFIAPI
PlatformBootManagerWaitCallback (IN UINT16 TimeoutRemain)
{
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL Color;

  // Verify Combo Message Position
  if (XPos == 0 && YPos == 0) {
    return;
  }

  // Get Timeout Time
  UINT16 Timeout = PcdGet16 (PcdPlatformBootTimeOut);

  // Check Remaining Timeout
  if (TimeoutRemain == Timeout || TimeoutRemain == 0) {
    // Set Draw Color
    UINT8 DrawColor = (TimeoutRemain == Timeout) ? 0xFF : 0x00;

    // Set GOP Draw Color
    Color.Red = Color.Green = Color.Blue = DrawColor;

    // Print Combo Message
    AsciiPrintXY (XPos, YPos, &Color, NULL, ComboMessage);
  }
}

VOID
EFIAPI
PlatformBootManagerUnableToBoot ()
{
  EFI_STATUS Status;

  // Execute Secondary Unable To Boot
  DeviceBootManagerUnableToBoot ();

  // Display No Boot OS Logo
  Status = DisplayBootGraphic (BG_NO_BOOT_OS);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "%a: Failed to Display No Boot OS Logo! Status = %r\n", __FUNCTION__, Status));
    Print (L"No Operating System Found! Please make sure that an OS is Installed.\n");
  }

  // Wait 10s
  gBS->Stall (10000000);

  // Shutdown
  gRT->ResetSystem (EfiResetShutdown, EFI_SUCCESS, 0, NULL);

  // Do Cpu Dead Loop
  CpuDeadLoop ();
}

VOID
EFIAPI
PlatformBootManagerBdsEntry ()
{
  EFI_STATUS Status;

  // Setup Secure Boot
  Status = SetupSecureBoot ();
  ASSERT_EFI_ERROR (Status);

  // Update ACPI Tables
  UpdateAcpiTables ();

  // Execute Secondary BDS Entry
  DeviceBootManagerBdsEntry ();
}
