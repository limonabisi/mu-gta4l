#
# ACPI tables (AcpiPlatformDxe installs every RAW section of this file).
# Sources and how each .aml is made: gta4lPkg/AcpiTables/README.md
#
FILE FREEFORM = 7E374E25-8E01-4FEE-87F2-390C23C606CD {
  SECTION RAW = gta4lPkg/AcpiTables/APIC.aml
  SECTION RAW = gta4lPkg/AcpiTables/DSDT.aml
  SECTION RAW = gta4lPkg/AcpiTables/FACP.aml
  SECTION RAW = gta4lPkg/AcpiTables/GTDT.aml
  SECTION RAW = gta4lPkg/AcpiTables/PPTT.aml
  SECTION RAW = gta4lPkg/AcpiTables/SPCR.aml
  SECTION RAW = gta4lPkg/AcpiTables/DBG2.aml
  SECTION UI = "AcpiTables"
}
