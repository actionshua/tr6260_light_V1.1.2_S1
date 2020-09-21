@ECHO OFF

SET CYGWIN_BIN=C:\cygwin\bin
SET COMPILER_BIN=C:\nds32le-elf-mculib-v3\bin
SET PATH=%COMPILER_BIN%;%CYGWIN_BIN%;%PATH%
SET HOME=%~dp0
if not exist ..\out md ..\out

bash --login -i
