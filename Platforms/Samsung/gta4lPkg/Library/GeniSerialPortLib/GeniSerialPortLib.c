/*
 * Polled serial port on the gta4l debug UART (QUP GENI SE at 0x04A90000,
 * the port the Qualcomm firmware already prints to). XBL has set up the
 * clocks, pins and baud rate; this only moves bytes through the FIFOs,
 * one byte per FIFO word, so the Shell can be used from the UART.
 */
#include <Base.h>
#include <Uefi/UefiBaseType.h>
#include <Library/IoLib.h>
#include <Library/SerialPortLib.h>

#define UART_BASE                 0x04A90000

#define SE_GENI_STATUS            0x040
#define SE_UART_TX_TRANS_LEN      0x270
#define SE_GENI_RX_PACKING_CFG0   0x284
#define SE_GENI_RX_PACKING_CFG1   0x288
#define SE_GENI_M_CMD0            0x600
#define SE_GENI_M_CMD_CTRL_REG    0x604
#define SE_GENI_M_IRQ_STATUS      0x610
#define SE_GENI_M_IRQ_CLEAR       0x618
#define SE_GENI_S_CMD0            0x630
#define SE_GENI_S_CMD_CTRL_REG    0x634
#define SE_GENI_S_IRQ_CLEAR       0x648
#define SE_GENI_TX_FIFOn          0x700
#define SE_GENI_RX_FIFOn          0x780
#define SE_GENI_RX_FIFO_STATUS    0x804

#define M_GENI_CMD_ACTIVE         BIT0
#define S_GENI_CMD_ACTIVE         BIT12
#define M_GENI_CMD_ABORT          BIT1
#define S_GENI_CMD_ABORT          BIT1
#define M_CMD_DONE_EN             BIT0
#define M_CMD_ABORT_EN            BIT5
#define M_TX_FIFO_WATERMARK_EN    BIT30
#define RX_FIFO_WC_MSK            0x01FFFFFF
#define OPCODE_SHIFT              27
#define UART_START_TX             1
#define UART_START_READ           1

#define POLL_LIMIT                200000

STATIC
BOOLEAN
WaitBit (UINTN Offset, UINT32 Mask, BOOLEAN Set)
{
  UINTN Count;

  for (Count = 0; Count < POLL_LIMIT; Count++) {
    if (((MmioRead32 (UART_BASE + Offset) & Mask) != 0) == Set) {
      return TRUE;
    }
  }

  return FALSE;
}

STATIC
VOID
StartRx (VOID)
{
  MmioWrite32 (UART_BASE + SE_GENI_S_IRQ_CLEAR, 0xFFFFFFFF);
  MmioWrite32 (UART_BASE + SE_GENI_S_CMD0, UART_START_READ << OPCODE_SHIFT);
}

RETURN_STATUS
EFIAPI
SerialPortInitialize (VOID)
{
  // Restart the receiver with one byte per FIFO word
  if (MmioRead32 (UART_BASE + SE_GENI_STATUS) & S_GENI_CMD_ACTIVE) {
    MmioWrite32 (UART_BASE + SE_GENI_S_CMD_CTRL_REG, S_GENI_CMD_ABORT);
    WaitBit (SE_GENI_S_CMD_CTRL_REG, S_GENI_CMD_ABORT, FALSE);
  }

  MmioWrite32 (UART_BASE + SE_GENI_RX_PACKING_CFG0, 0xF);
  MmioWrite32 (UART_BASE + SE_GENI_RX_PACKING_CFG1, 0x0);
  StartRx ();

  return RETURN_SUCCESS;
}

UINTN
EFIAPI
SerialPortWrite (IN UINT8 *Buffer, IN UINTN NumberOfBytes)
{
  UINTN Index;

  if (Buffer == NULL) {
    return 0;
  }

  for (Index = 0; Index < NumberOfBytes; Index++) {
    // Let any transfer in flight (ours or the firmware's) finish
    WaitBit (SE_GENI_STATUS, M_GENI_CMD_ACTIVE, FALSE);

    MmioWrite32 (UART_BASE + SE_GENI_M_IRQ_CLEAR, M_CMD_DONE_EN | M_TX_FIFO_WATERMARK_EN);
    MmioWrite32 (UART_BASE + SE_UART_TX_TRANS_LEN, 1);
    MmioWrite32 (UART_BASE + SE_GENI_M_CMD0, UART_START_TX << OPCODE_SHIFT);
    MmioWrite32 (UART_BASE + SE_GENI_TX_FIFOn, Buffer[Index]);

    if (!WaitBit (SE_GENI_M_IRQ_STATUS, M_CMD_DONE_EN, TRUE)) {
      MmioWrite32 (UART_BASE + SE_GENI_M_CMD_CTRL_REG, M_GENI_CMD_ABORT);
      WaitBit (SE_GENI_M_IRQ_STATUS, M_CMD_ABORT_EN, TRUE);
    }

    MmioWrite32 (UART_BASE + SE_GENI_M_IRQ_CLEAR, M_CMD_DONE_EN | M_CMD_ABORT_EN | M_TX_FIFO_WATERMARK_EN);
  }

  return NumberOfBytes;
}

BOOLEAN
EFIAPI
SerialPortPoll (VOID)
{
  if (MmioRead32 (UART_BASE + SE_GENI_RX_FIFO_STATUS) & RX_FIFO_WC_MSK) {
    return TRUE;
  }

  // The receiver stops after errors; keep it running
  if (!(MmioRead32 (UART_BASE + SE_GENI_STATUS) & S_GENI_CMD_ACTIVE)) {
    StartRx ();
  }

  return FALSE;
}

UINTN
EFIAPI
SerialPortRead (OUT UINT8 *Buffer, IN UINTN NumberOfBytes)
{
  UINTN Index;

  if (Buffer == NULL) {
    return 0;
  }

  for (Index = 0; Index < NumberOfBytes; Index++) {
    while (!SerialPortPoll ()) {
    }

    Buffer[Index] = (UINT8)(MmioRead32 (UART_BASE + SE_GENI_RX_FIFOn) & 0xFF);
  }

  return NumberOfBytes;
}

RETURN_STATUS
EFIAPI
SerialPortSetControl (IN UINT32 Control)
{
  return RETURN_SUCCESS;
}

RETURN_STATUS
EFIAPI
SerialPortGetControl (OUT UINT32 *Control)
{
  *Control = 0;
  if (!SerialPortPoll ()) {
    *Control |= EFI_SERIAL_INPUT_BUFFER_EMPTY;
  }

  return RETURN_SUCCESS;
}

RETURN_STATUS
EFIAPI
SerialPortSetAttributes (
  IN OUT UINT64              *BaudRate,
  IN OUT UINT32              *ReceiveFifoDepth,
  IN OUT UINT32              *Timeout,
  IN OUT EFI_PARITY_TYPE     *Parity,
  IN OUT UINT8               *DataBits,
  IN OUT EFI_STOP_BITS_TYPE  *StopBits
  )
{
  // XBL owns the line settings (115200 8N1)
  return RETURN_SUCCESS;
}
