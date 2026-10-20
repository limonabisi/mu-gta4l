/*
 * DSDT for the Samsung Galaxy Tab A7 10.4 2020 (SM-T500, gta4l)
 * SoC: Qualcomm Snapdragon 662 (SM6115, "Kamorta")
 *
 * Addresses and interrupts come from the mainline device tree
 * (arch/arm64/boot/dts/qcom/sm6115.dtsi + sm6115-samsung-gta4l.dts).
 * GSIV = GIC SPI number + 32.
 *
 * Only devices that Windows can drive without Qualcomm's SM6115 driver
 * package, or that Qualcomm's generic drivers match, are described:
 *   CPUs, eMMC (SDC1), SD card (SDC2), USB 3.0 XHCI host (OTG port),
 *   and read-only thermal zones on the TSENS sensors.
 *
 * DSDT-testI.asl is the table of the first image that booted WinPE.
 *
 * Build:  iasl DSDT.asl                                  (8 cores, the default)
 *         iasl -D FOUR_CORES -p DSDT_4core DSDT.asl      (CPUs 0-3, with APIC_4core.aml)
 */
DefinitionBlock ("DSDT.aml", "DSDT", 2, "QCOMM ", "SM6115 ", 4)
{
    Scope (\_SB)
    {
        //
        // Subsystem ID for the Qualcomm drivers that match QCOM* devices
        //
        Name (PSUB, "MTP06115")

        //
        // CPUs: Kryo 260 Silver 0-3 (MPIDR 0x000-0x003), Gold 4-7 (0x100-0x103)
        //
        Device (CPU0) { Name (_HID, "ACPI0007") Name (_UID, 0) Method (_STA, 0, NotSerialized) { Return (0x0F) } }
        Device (CPU1) { Name (_HID, "ACPI0007") Name (_UID, 1) Method (_STA, 0, NotSerialized) { Return (0x0F) } }
        Device (CPU2) { Name (_HID, "ACPI0007") Name (_UID, 2) Method (_STA, 0, NotSerialized) { Return (0x0F) } }
        Device (CPU3) { Name (_HID, "ACPI0007") Name (_UID, 3) Method (_STA, 0, NotSerialized) { Return (0x0F) } }
#ifdef FOUR_CORES
        Device (CPU4) { Name (_HID, "ACPI0007") Name (_UID, 4) Method (_STA, 0, NotSerialized) { Return (0x00) } }
        Device (CPU5) { Name (_HID, "ACPI0007") Name (_UID, 5) Method (_STA, 0, NotSerialized) { Return (0x00) } }
        Device (CPU6) { Name (_HID, "ACPI0007") Name (_UID, 6) Method (_STA, 0, NotSerialized) { Return (0x00) } }
        Device (CPU7) { Name (_HID, "ACPI0007") Name (_UID, 7) Method (_STA, 0, NotSerialized) { Return (0x00) } }
#else
        Device (CPU4) { Name (_HID, "ACPI0007") Name (_UID, 4) Method (_STA, 0, NotSerialized) { Return (0x0F) } }
        Device (CPU5) { Name (_HID, "ACPI0007") Name (_UID, 5) Method (_STA, 0, NotSerialized) { Return (0x0F) } }
        Device (CPU6) { Name (_HID, "ACPI0007") Name (_UID, 6) Method (_STA, 0, NotSerialized) { Return (0x0F) } }
        Device (CPU7) { Name (_HID, "ACPI0007") Name (_UID, 7) Method (_STA, 0, NotSerialized) { Return (0x0F) } }
#endif

        //
        // SDC1: eMMC, sdhci-msm v5. The "hc" registers at 0x04744000 are a
        // standard SDHCI block, so besides Qualcomm's SD driver (QCOM24BF) the
        // Windows SD host driver can take it (PNP0D40). hc_irq: SPI 348.
        //
        Device (SDC1)
        {
            Name (_HID, "QCOM24BF")
            Name (_CID, Package () { "ACPIQCOM24BF", EisaId ("PNP0D40") })
            Name (_UID, 0)
            Name (_CCA, 0)
            Method (_STA, 0, NotSerialized) { Return (0x0F) }
            Method (_SUB, 0, NotSerialized) { Return (\_SB.PSUB) }

            Name (_CRS, ResourceTemplate ()
            {
                Memory32Fixed (ReadWrite, 0x04744000, 0x00001000)
                Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive, , , ) { 380 }
            })

            Device (EMMC)
            {
                Name (_ADR, 8)
                Method (_RMV, 0, NotSerialized) { Return (0) }
            }
        }

        //
        // SDC2: microSD slot, sdhci-msm v5 at 0x04784000. hc_irq: SPI 350.
        // UEFI powers the slot when a card is present at boot.
        //
        Device (SDC2)
        {
            Name (_HID, "QCOM24BF")
            Name (_CID, Package () { "ACPIQCOM24BF", EisaId ("PNP0D40") })
            Name (_UID, 1)
            Name (_CCA, 0)
            Method (_STA, 0, NotSerialized) { Return (0x0F) }
            Method (_SUB, 0, NotSerialized) { Return (\_SB.PSUB) }

            Name (_CRS, ResourceTemplate ()
            {
                Memory32Fixed (ReadWrite, 0x04784000, 0x00001000)
                Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive, , , ) { 382 }
            })

            Device (SDMM)
            {
                Name (_ADR, 0)
                Method (_RMV, 0, NotSerialized) { Return (1) }
            }
        }

        //
        // USB0: DWC3 at 0x04E00000 in host mode (UEFI switches it and keeps
        // VBUS on across ExitBootServices), driven by the inbox usbxhci.
        // dwc_usb3 irq: SPI 255. One USB 2.0 and one SuperSpeed root port,
        // both on the USB-C connector.
        //
        Device (USB0)
        {
            Name (_HID, EisaId ("PNP0D10"))
            Name (_UID, 0)
            Name (_CCA, 0)
            Method (_STA, 0, NotSerialized) { Return (0x0F) }

            Name (_CRS, ResourceTemplate ()
            {
                Memory32Fixed (ReadWrite, 0x04E00000, 0x0000CD00)
                Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive, , , ) { 287 }
            })

            Device (RHUB)
            {
                Name (_ADR, 0)

                // USB 2.0 lines of the USB-C connector
                Device (PRT1)
                {
                    Name (_ADR, 1)
                    Name (_UPC, Package () { 0xFF, 0x09, 0, 0 })
                }

                // SuperSpeed lines of the USB-C connector
                Device (PRT2)
                {
                    Name (_ADR, 2)
                    Name (_UPC, Package () { 0xFF, 0x09, 0, 0 })
                }
            }
        }

        //
        // TSENS (tsens-v2) temperature monitor. TM_Sn_STATUS at 0x044110A0 + 4n:
        // bits 11:0 hold the last temperature, signed, in tenths of a degree
        // Celsius. Read-only, and no trip points: nothing here can shut down.
        //
        OperationRegion (TSRG, SystemMemory, 0x044110A0, 0x40)
        Field (TSRG, DWordAcc, NoLock, Preserve)
        {
            TS00, 32, TS01, 32, TS02, 32, TS03, 32,
            TS04, 32, TS05, 32, TS06, 32, TS07, 32,
            TS08, 32, TS09, 32, TS10, 32, TS11, 32,
            TS12, 32, TS13, 32, TS14, 32, TS15, 32
        }

        // TSENS status word -> tenths of a Kelvin, the unit of _TMP
        Method (TSKV, 1, NotSerialized)
        {
            Local0 = (Arg0 & 0x0FFF)
            If (Local0 & 0x0800)
            {
                Return (2732 - (0x1000 - Local0))
            }

            Return (2732 + Local0)
        }
    }

    //
    // Thermal zones; sensor numbers as in the device tree's thermal-zones.
    // _TZP: Windows polls every 10 s.
    //
    Scope (\_TZ)
    {
        ThermalZone (CPUL)      // cpu0123-thermal, sensor 12
        {
            Name (_STR, Unicode ("CPU 0-3 (Silver)"))
            Name (_TZP, 100)
            Method (_TMP, 0, NotSerialized) { Return (\_SB.TSKV (\_SB.TS12)) }
        }

        ThermalZone (CPU4)      // cpu45-thermal, sensor 10
        {
            Name (_STR, Unicode ("CPU 4-5 (Gold)"))
            Name (_TZP, 100)
            Method (_TMP, 0, NotSerialized) { Return (\_SB.TSKV (\_SB.TS10)) }
        }

        ThermalZone (CPU6)      // cpu67-thermal, sensor 11
        {
            Name (_STR, Unicode ("CPU 6-7 (Gold)"))
            Name (_TZP, 100)
            Method (_TMP, 0, NotSerialized) { Return (\_SB.TSKV (\_SB.TS11)) }
        }

        ThermalZone (GPU0)      // gpu-thermal, sensor 15
        {
            Name (_STR, Unicode ("GPU"))
            Name (_TZP, 100)
            Method (_TMP, 0, NotSerialized) { Return (\_SB.TSKV (\_SB.TS15)) }
        }

        ThermalZone (MDM0)      // modem0-thermal, sensor 13
        {
            Name (_STR, Unicode ("Modem"))
            Name (_TZP, 100)
            Method (_TMP, 0, NotSerialized) { Return (\_SB.TSKV (\_SB.TS13)) }
        }

        ThermalZone (DISP)      // display-thermal, sensor 14
        {
            Name (_STR, Unicode ("Display"))
            Name (_TZP, 100)
            Method (_TMP, 0, NotSerialized) { Return (\_SB.TSKV (\_SB.TS14)) }
        }

        ThermalZone (VIDE)      // video-thermal, sensor 4
        {
            Name (_STR, Unicode ("Video"))
            Name (_TZP, 100)
            Method (_TMP, 0, NotSerialized) { Return (\_SB.TSKV (\_SB.TS04)) }
        }

        ThermalZone (WLAN)      // wlan-thermal, sensor 2
        {
            Name (_STR, Unicode ("WLAN"))
            Name (_TZP, 100)
            Method (_TMP, 0, NotSerialized) { Return (\_SB.TSKV (\_SB.TS02)) }
        }
    }
}
