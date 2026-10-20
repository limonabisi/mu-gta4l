DefinitionBlock ("DSDT.aml", "DSDT", 2, "QCOMM ", "SM6115 ", 3)
{
    Scope (\_SB)
    {
        Device (CPU0) { Name (_HID, "ACPI0007") Name (_UID, 0) Method (_STA, 0, NotSerialized) { Return (0x0F) } }
        Device (CPU1) { Name (_HID, "ACPI0007") Name (_UID, 1) Method (_STA, 0, NotSerialized) { Return (0x0F) } }
        Device (CPU2) { Name (_HID, "ACPI0007") Name (_UID, 2) Method (_STA, 0, NotSerialized) { Return (0x0F) } }
        Device (CPU3) { Name (_HID, "ACPI0007") Name (_UID, 3) Method (_STA, 0, NotSerialized) { Return (0x0F) } }
        Device (CPU4) { Name (_HID, "ACPI0007") Name (_UID, 4) Method (_STA, 0, NotSerialized) { Return (0x00) } }
        Device (CPU5) { Name (_HID, "ACPI0007") Name (_UID, 5) Method (_STA, 0, NotSerialized) { Return (0x00) } }
        Device (CPU6) { Name (_HID, "ACPI0007") Name (_UID, 6) Method (_STA, 0, NotSerialized) { Return (0x00) } }
        Device (CPU7) { Name (_HID, "ACPI0007") Name (_UID, 7) Method (_STA, 0, NotSerialized) { Return (0x00) } }

        Device (SDC1)
        {
            Name (_HID, "QCOM24BF")
            Name (_CID, "ACPIQCOM24BF")
            Name (_UID, 0)
            Name (_CCA, 0)
            Method (_STA, 0, NotSerialized) { Return (0x0F) }

            Name (_CRS, ResourceTemplate ()
            {
                Memory32Fixed (ReadWrite, 0x04744000, 0x00002000)
                Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive, ,, ) { 0x17C }
            })

            Device (EMMC)
            {
                Name (_ADR, 8)
                Name (_RMV, 0)
            }
        }

        Device (USB0)
        {
            Name (_HID, EisaId ("PNP0D10"))
            Name (_UID, Zero)
            Name (_CCA, Zero)
            Method (_STA, 0, NotSerialized) { Return (0x0F) }
            Name (_CRS, ResourceTemplate ()
            {
                Memory32Fixed (ReadWrite, 0x04E00000, 0x0000CD00)
                Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive, , , ) { 0x0000011F }
            })
            Device (RHUB)
            {
                Name (_ADR, Zero)
                Device (PRT1) { Name (_ADR, One) }
            }
        }
    }
}
