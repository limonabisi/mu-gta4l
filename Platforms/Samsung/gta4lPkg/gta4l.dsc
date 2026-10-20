##
#
#  Copyright (c) 2011 - 2022, ARM Limited. All rights reserved.
#  Copyright (c) 2014, Linaro Limited. All rights reserved.
#  Copyright (c) 2015 - 2020, Intel Corporation. All rights reserved.
#  Copyright (c) 2018, Bingxing Wang. All rights reserved.
#  Copyright (c) Microsoft Corporation.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME                  = gta4l
  PLATFORM_GUID                  = A22E8C3E-B0BE-4C0E-8192-57EE0D9980EC
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/gta4lPkg
  SUPPORTED_ARCHITECTURES        = AARCH64
  BUILD_TARGETS                  = RELEASE|DEBUG
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = gta4lPkg/gta4l.fdf
  USE_CUSTOM_DISPLAY_DRIVER      = 0

!include KamortaPkg/KamortaPkg.dsc.inc

[PcdsFixedAtBuild]
  #
  # DEBUG: keep the log, but do not halt on asserts from the Qualcomm binaries
  #
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x2E

  #
  # Connect every console input (USB keyboards too) before the boot timeout,
  # so the Esc / F2 hotkeys work
  #
  gEfiMdeModulePkgTokenSpaceGuid.PcdConInConnectOnDemand|FALSE

  #
  # DDR Memory
  #
  gArmTokenSpaceGuid.PcdSystemMemoryBase|0x40000000

  #
  # UEFI Stack
  #
  gArmPlatformTokenSpaceGuid.PcdCPUCoresStackBase|0x5FF90000
  gArmPlatformTokenSpaceGuid.PcdCPUCorePrimaryStackSize|0x40000

  #
  # SMBIOS
  #
  gSiliciumPkgTokenSpaceGuid.PcdSmbiosSystemManufacturer|"Samsung"
  gSiliciumPkgTokenSpaceGuid.PcdSmbiosSystemModel|"Galaxy Tab A7 LTE"
  gSiliciumPkgTokenSpaceGuid.PcdSmbiosSystemRetailModel|"gta4l"
  gSiliciumPkgTokenSpaceGuid.PcdSmbiosSystemRetailSku|"Galaxy_Tab_A7_LTE_gta4l"
  gSiliciumPkgTokenSpaceGuid.PcdSmbiosSystemBoardModel|"Galaxy Tab A7 LTE"

  #
  # Simple Frame Buffer
  #
  gSiliciumPkgTokenSpaceGuid.PcdFrameBufferWidth|1200
  gSiliciumPkgTokenSpaceGuid.PcdFrameBufferHeight|2000
  gSiliciumPkgTokenSpaceGuid.PcdFrameBufferColorDepth|32

  #
  # Platform PEI
  #
  gQcomPkgTokenSpaceGuid.PcdPlatformType|"LA"

[LibraryClasses]
  #
  # Debug output on the GENI UART instead of the screen
  #
  SerialPortLib|gta4lPkg/Library/GeniSerialPortLib/GeniSerialPortLib.inf

  #
  # Memory Libraries
  #
  MemoryMapLib|gta4lPkg/Library/MemoryMapLib/MemoryMapLib.inf

  #
  # QCOM Libraries
  #
  ConfigurationMapLib|gta4lPkg/Library/ConfigurationMapLib/ConfigurationMapLib.inf

  #
  # Boot Options: UEFI Shell first
  #
  PlatformBootOptionsLib|gta4lPkg/Library/PlatformBootOptionsLib/PlatformBootOptionsLib.inf

  #
  # Boot Manager: Vol+/Esc Boot Menu, Vol-/F2 Setup, Power = Enter
  #
  PlatformBootManagerLib|gta4lPkg/Library/BootManagerLib/BootManagerLib.inf

  #
  # Setup (UiApp) and ext4
  #
  CustomizedDisplayLib|MdeModulePkg/Library/CustomizedDisplayLib/CustomizedDisplayLib.inf
  FileExplorerLib|MdeModulePkg/Library/FileExplorerLib/FileExplorerLib.inf
  BaseUcs2Utf8Lib|Ext4Pkg/Library/BaseUcs2Utf8Lib/BaseUcs2Utf8Lib.inf

  #
  # UEFI Shell
  #
  ShellLib|ShellPkg/Library/UefiShellLib/UefiShellLib.inf
  HandleParsingLib|ShellPkg/Library/UefiHandleParsingLib/UefiHandleParsingLib.inf
  OrderedCollectionLib|MdePkg/Library/BaseOrderedCollectionRedBlackTreeLib/BaseOrderedCollectionRedBlackTreeLib.inf
  BcfgCommandLib|ShellPkg/Library/UefiShellBcfgCommandLib/UefiShellBcfgCommandLib.inf
  ShellCEntryLib|ShellPkg/Library/UefiShellCEntryLib/UefiShellCEntryLib.inf

[Components]
  #
  # ACPI Tables
  #
  #gta4l/AcpiTables.inf

  #
  # Setup: front page with Device Manager, Boot Manager, Boot Maintenance
  #
  MdeModulePkg/Universal/DisplayEngineDxe/DisplayEngineDxe.inf
  MdeModulePkg/Application/UiApp/UiApp.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/DeviceManagerUiLib/DeviceManagerUiLib.inf
      NULL|MdeModulePkg/Library/BootManagerUiLib/BootManagerUiLib.inf
      NULL|MdeModulePkg/Library/BootMaintenanceManagerUiLib/BootMaintenanceManagerUiLib.inf
  }

  #
  # ext4, to load Linux from the postmarketOS boot partition
  #
  Ext4Pkg/Ext4Dxe/Ext4Dxe.inf

  #
  # Shell on the debug UART, and startup.nsh read from the firmware volume
  #
  MdeModulePkg/Universal/SerialDxe/SerialDxe.inf {
    <LibraryClasses>
      SerialPortLib|gta4lPkg/Library/GeniSerialPortLib/GeniSerialPortLib.inf
  }
  MdeModulePkg/Universal/Console/TerminalDxe/TerminalDxe.inf
  MdeModulePkg/Universal/FvSimpleFileSystemDxe/FvSimpleFileSystemDxe.inf

  #
  # UEFI Shell
  #
  ShellPkg/Application/Shell/Shell.inf {
    <LibraryClasses>
      ShellCommandLib|ShellPkg/Library/UefiShellCommandLib/UefiShellCommandLib.inf
      NULL|ShellPkg/Library/UefiShellLevel2CommandsLib/UefiShellLevel2CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellLevel1CommandsLib/UefiShellLevel1CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellLevel3CommandsLib/UefiShellLevel3CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellDriver1CommandsLib/UefiShellDriver1CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellDebug1CommandsLib/UefiShellDebug1CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellInstall1CommandsLib/UefiShellInstall1CommandsLib.inf
    <PcdsFixedAtBuild>
      gEfiShellPkgTokenSpaceGuid.PcdShellLibAutoInitialize|FALSE
      gEfiMdePkgTokenSpaceGuid.PcdUefiLibMaxPrintBufferSize|8000
  }
