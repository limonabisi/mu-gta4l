/**
  gta4l boot hotkeys and keypad mapping.

  While the boot timeout runs:
    Vol+ or Esc  -> Boot Menu
    Vol- or F2   -> Setup (UiApp)

  The Power key on the keypad is turned into Enter, so the menus and the
  Setup pages can be used with the three tablet keys alone.

  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DevicePathLib.h>

#include <Protocol/SimpleTextIn.h>
#include <Protocol/SimpleTextInEx.h>

#include "BootManager.h"

#define MAX_NOTIFY  4

typedef struct {
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *Protocol;
  VOID                               *Handle;
} KEY_NOTIFY_ENTRY;

STATIC EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *mKeypadInEx       = NULL;
STATIC KEY_NOTIFY_ENTRY                    mNotify[MAX_NOTIFY];
STATIC UINTN                               mNotifyCount      = 0;
STATIC BOOLEAN                             mEnterBootManager = FALSE;
STATIC BOOLEAN                             mEnterSetup       = FALSE;

//
// Power key -> Enter on the keypad
//
STATIC EFI_INPUT_READ_KEY     mOrigReadKey   = NULL;
STATIC EFI_INPUT_READ_KEY_EX  mOrigReadKeyEx = NULL;

STATIC
VOID
MapKeypadKey (IN OUT EFI_INPUT_KEY *Key)
{
  // The keypad only has Vol+, Vol- and Power; anything else is Power
  if (Key->ScanCode != SCAN_NULL && Key->ScanCode != SCAN_UP && Key->ScanCode != SCAN_DOWN) {
    Key->ScanCode    = SCAN_NULL;
    Key->UnicodeChar = CHAR_CARRIAGE_RETURN;
  }
}

STATIC
EFI_STATUS
EFIAPI
KeypadReadKey (IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This, OUT EFI_INPUT_KEY *Key)
{
  EFI_STATUS Status = mOrigReadKey (This, Key);

  if (!EFI_ERROR (Status)) {
    MapKeypadKey (Key);
  }

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
KeypadReadKeyEx (IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This, OUT EFI_KEY_DATA *KeyData)
{
  EFI_STATUS Status = mOrigReadKeyEx (This, KeyData);

  if (!EFI_ERROR (Status)) {
    MapKeypadKey (&KeyData->Key);
  }

  return Status;
}

STATIC
VOID
MapPowerKeyToEnter (IN EFI_HANDLE StiHandle)
{
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL *TextIn;

  if (mOrigReadKeyEx == NULL && mKeypadInEx != NULL) {
    mOrigReadKeyEx               = mKeypadInEx->ReadKeyStrokeEx;
    mKeypadInEx->ReadKeyStrokeEx = KeypadReadKeyEx;
  }

  if (mOrigReadKey == NULL &&
      !EFI_ERROR (gBS->HandleProtocol (StiHandle, &gEfiSimpleTextInProtocolGuid, (VOID **)&TextIn))) {
    mOrigReadKey          = TextIn->ReadKeyStroke;
    TextIn->ReadKeyStroke = KeypadReadKey;
  }
}

//
// Hotkeys
//
STATIC
EFI_STATUS
EFIAPI
BootMenuNotify (IN EFI_KEY_DATA *KeyData)
{
  mEnterBootManager = TRUE;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
SetupNotify (IN EFI_KEY_DATA *KeyData)
{
  mEnterSetup = TRUE;
  return EFI_SUCCESS;
}

STATIC
VOID
AddNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *Protocol,
  IN UINT16                              ScanCode,
  IN EFI_KEY_NOTIFY_FUNCTION             Function
  )
{
  EFI_STATUS   Status;
  EFI_KEY_DATA KeyData;

  if (Protocol == NULL || mNotifyCount >= MAX_NOTIFY) {
    return;
  }

  KeyData.KeyState.KeyToggleState = 0;
  KeyData.KeyState.KeyShiftState  = 0;
  KeyData.Key.UnicodeChar         = CHAR_NULL;
  KeyData.Key.ScanCode            = ScanCode;

  Status = Protocol->RegisterKeyNotify (Protocol, &KeyData, Function, &mNotify[mNotifyCount].Handle);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "%a: Failed to register key 0x%x! Status = %r\n", __FUNCTION__, ScanCode, Status));
    return;
  }

  mNotify[mNotifyCount].Protocol = Protocol;
  mNotifyCount++;
}

EFI_STATUS
SetupKeypad (IN EFI_DEVICE_PATH_PROTOCOL *DevicePath)
{
  EFI_STATUS  Status;
  EFI_HANDLE  StiHandle;
  EFI_HANDLE *KeypadBuffer;
  UINTN       KeypadCount;

  // Locate Keypad Controller Protocols
  Status = gBS->LocateHandleBuffer (ByProtocol, &gKeypadDeviceProtocolGuid, NULL, &KeypadCount, &KeypadBuffer);
  if (!EFI_ERROR (Status)) {
    for (UINTN i = 0; i < KeypadCount; i++) {
      gBS->ConnectController (KeypadBuffer[i], NULL, NULL, TRUE);
    }

    FreePool (KeypadBuffer);
  }

  // Locate STI Protocol Handle
  Status = gBS->LocateDevicePath (&gEfiSimpleTextInputExProtocolGuid, &DevicePath, &StiHandle);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "%a: Failed to Locate STI Device Path! Status = %r\n", __FUNCTION__, Status));
    return Status;
  }

  Status = gBS->HandleProtocol (StiHandle, &gEfiSimpleTextInputExProtocolGuid, (VOID *)&mKeypadInEx);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "%a: Failed to get STI Protocol from Handle! Status = %r\n", __FUNCTION__, Status));
    return Status;
  }

  mKeypadInEx->Reset (mKeypadInEx, TRUE);

  MapPowerKeyToEnter (StiHandle);

  return EFI_SUCCESS;
}

EFI_STATUS
RegisterKeyCallback (IN EFI_DEVICE_PATH_PROTOCOL *DevicePath)
{
  EFI_STATUS                         Status;
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *ConInEx = NULL;

  // Keypad: Vol+ -> Boot Menu, Vol- -> Setup
  Status = SetupKeypad (DevicePath);
  if (!EFI_ERROR (Status)) {
    AddNotify (mKeypadInEx, SCAN_UP,   BootMenuNotify);
    AddNotify (mKeypadInEx, SCAN_DOWN, SetupNotify);
  }

  // Keyboards (USB, serial): Esc -> Boot Menu, F2 -> Setup
  if (gST->ConsoleInHandle != NULL &&
      !EFI_ERROR (gBS->HandleProtocol (gST->ConsoleInHandle, &gEfiSimpleTextInputExProtocolGuid, (VOID **)&ConInEx))) {
    AddNotify (ConInEx, SCAN_ESC, BootMenuNotify);
    AddNotify (ConInEx, SCAN_F2,  SetupNotify);
  }

  return (mNotifyCount > 0) ? EFI_SUCCESS : EFI_NOT_FOUND;
}

VOID
UnregisterKeyCallback ()
{
  for (UINTN i = 0; i < mNotifyCount; i++) {
    mNotify[i].Protocol->UnregisterKeyNotify (mNotify[i].Protocol, mNotify[i].Handle);
  }

  mNotifyCount = 0;
}

BOOLEAN
EnterBootManager ()
{
  return mEnterBootManager;
}

BOOLEAN
EnterSetup ()
{
  return mEnterSetup;
}
