# gta4l ACPI tables

Tables installed by `Include/ACPI.inc` (Samsung Galaxy Tab A7 10.4 2020, SM6115 "Kamorta"):

| Table | Source | Notes |
|---|---|---|
| APIC | `APIC.aml` (= `APIC_8core.aml`) | GICv3 at 0x0F200000, redistributors 0x0F300000 + 0x20000 per core, all 8 CPUs. `APIC_4core.aml`: CPUs 0-3 only |
| DSDT | `DSDT.asl` → `iasl DSDT.asl` | CPUs, eMMC (SDC1), microSD (SDC2), USB XHCI, TSENS thermal zones. 4 cores: `iasl -D FOUR_CORES -p DSDT_4core DSDT.asl` |
| FACP | `FACP.aml` | Hardware-reduced, PSCI, tablet profile |
| GTDT | `GTDT.aml` | Arch timer PPIs 1/2/3/0 (GSIV 17/18/19/16), memory-mapped timer at 0x0F120000 |
| PPTT | `python3 gen-pptt.py` | Package, two clusters (A53 x4 with 512 KiB L2, A73 x4 with 1 MiB L2), per-CPU L1I/L1D |
| SPCR, DBG2 | `python3 gen-serial-tables.py` | GENI debug UART at 0x04A90000, GSIV 363, 115200 8N1, subtype 0x12 |

The Silicium common SSDT (`SSDT.aml`, an empty "SSDE" device) is not
installed: Windows has no driver for it and shows it as a problem device.

`FACP.aml`, `GTDT.aml` and `APIC_4core.aml` are Silicium-ACPI's Kamorta
tables (`Silicon/Qualcomm/Kamorta`). `DSDT-testI.asl` / `.aml` is the DSDT of
the first image that booted WinPE, kept for comparison.

## What Windows can use

Windows has no SM6115 driver package, so only devices with inbox drivers,
or with Qualcomm's generic drivers, can work:

- CPUs, GIC, timers: inbox
- Display: the UEFI frame buffer (Microsoft Basic Display Adapter)
- USB-C port in host mode: inbox `usbxhci` (PNP0D10)
- eMMC and microSD: Qualcomm SD driver if present (QCOM24BF), otherwise the
  inbox SD host driver (PNP0D40, standard SDHCI registers)
- Thermal zones: inbox ACPI thermal driver, read-only
- Debug UART: Windows kernel debugger / EMS via DBG2 and SPCR

Not possible without Qualcomm or vendor Windows drivers: GPU (Adreno 610),
touchscreen (SPI), Wi-Fi/Bluetooth (WCN3990), audio, battery and charger
(PMI632 over SPMI), buttons (PMIC/TLMM GPIO), sensors, camera, modem.
