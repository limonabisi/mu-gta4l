@echo -off
echo "gta4l: at power on, Vol- / F2 = Setup, Vol+ / Esc = Boot Menu, Power = Enter"
connect -r
map -r > nul
for %i run (1 99)
  if exist fs%i:\linux.nsh then
    echo "gta4l: starting fs%i:\linux.nsh"
    fs%i:
    linux.nsh
  endif
endfor
