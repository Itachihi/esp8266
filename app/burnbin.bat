@echo off

set BAK=%PATH%
set pwd=%~dp0
set pdir=%pwd%..
set PATH=%PATH%;%pdir%\tools

::esptool.py write_flash 0x000000 %pdir%\bin\eagle.app.flash.bin
::下面分为两部分写入,更快(减少近200k的填充字节)
esptool.py write_flash 0x000000 %pdir%\bin\eagle.app.flash.bin 0x40000 %pdir%\bin\eagle.app.v6.irom0text.bin

set PATH=%BAK%
set pdir=
set pwd=
set BAK=