@echo -off
echo "gta4l: diagnostics"
ver
drivers
echo "gta4l: plug the keyboard in now, 15 s"
stall 15000000
connect -r
dh -p UsbIo
dh -p SimpleTextIn
dmpstore ConIn
echo "gta4l: done, type on the keyboard"
