@echo -off
echo "gta4l: Vol- / F2 = Setup, Vol+ / Esc = Boot Menu, Power = Enter"
map -r > nul
for %i run (0 20)
  if exist fs%i:\linux.nsh then
    echo "gta4l: fs%i:\linux.nsh bulundu, baslatiliyor"
    fs%i:
    linux.nsh
  endif
  if exist fs%i:\EFI\BOOT\BOOTAA64.EFI then
    echo "gta4l: fs%i:\EFI\BOOT\BOOTAA64.EFI bulundu, baslatiliyor"
    fs%i:\EFI\BOOT\BOOTAA64.EFI
  endif
endfor
echo "gta4l: HICBIR DISKTE BOOTAA64.EFI BULUNAMADI"
echo "gta4l: diskleri gormek icin: map -b   etiket icin: vol fs0:"
