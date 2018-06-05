@echo off
title Deken manager for Faust~ for Pure Data
set release_version=%1

del /F /Q faust~
del /F /Q faust_tilde_darwin.zip
del /F /Q faust_tilde_linux.zip
del /F /Q faust_tilde_windows.zip
del /F /Q faust_tilde_archive.zip

curl -L https://github.com/pierreguillot/faust-pd/releases/download/%release_version%/faust_tilde-Darwin-amd64-32-sources.zip -o faust_tilde_darwin.zip
curl -L https://github.com/pierreguillot/faust-pd/releases/download/%release_version%/faust_tilde-Linux-amd64-32-sources.zip -o faust_tilde_linux.zip
curl -L https://github.com/pierreguillot/faust-pd/releases/download/%release_version%/faust_tilde-Windows-amd64-32-sources.zip -o faust_tilde_windows.zip
curl -L https://github.com/pierreguillot/faust-pd/releases/archive/%release_version%.zip -o faust_tilde_archive.zip

7z x faust_tilde_darwin.zip
deken package -%release_version% faust~
del /F /Q faust~
del /F /Q faust_tilde_darwin.zip

7z x faust_tilde_linux.zip
deken package -%release_version% faust~
del /F /Q faust~
del /F /Q faust_tilde_linux.zip

7z x faust_tilde_windows.zip
deken package -%release_version% faust~
del /F /Q faust~
del /F /Q faust_tilde_windows.zip

7z x faust_tilde_archive.zip
deken package -%release_version% faust~
del /F /Q faust~
del /F /Q faust_tilde_archive.zip

deken upload faust~[%release_version%](Darwin-amd64-32)(Sources).dek faust~[%release_version%](Linux-amd64-32)(Sources).dek faust~[%release_version%](Windows-amd64-32)(Sources).dek faust~[%release_version%](Sources).dek
pause
