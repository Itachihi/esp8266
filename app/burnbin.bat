@echo off

set BAK=%PATH%
set pwd=%~dp0
set pdir=%pwd%..
set PATH=%PATH%;%pdir%\tools

::esptool.py write_flash 0x000000 %pdir%\bin\eagle.app.flash.bin
::�����Ϊ������д��,����(���ٽ�200k������ֽ�)
esptool.py write_flash 0x000000 %pdir%\bin\eagle.app.flash.bin 0x40000 %pdir%\bin\eagle.app.v6.irom0text.bin

set PATH=%BAK%
set pdir=
set pwd=
set BAK=