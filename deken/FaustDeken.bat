@echo off
title Deken manager for faustgen~ for Pure Data
set release_version=%1

rmdir /S /Q faustgen~
del /F /S /Q /A *.dek
del /F /S /Q /A *.dek.sha256
del /F /S /Q /A *.dek.txt

del /F /Q faustgen_tilde_darwin.zip
curl -L https://github.com/pierreguillot/faust-pd/releases/download/v%release_version%/faustgen_tilde-Darwin-amd64-32-sources.zip -o faustgen_tilde_darwin.zip
7z x faustgen_tilde_darwin.zip
deken package -v%release_version% faustgen~
rmdir /S /Q faustgen~
del /F /Q faustgen_tilde_darwin.zip

del /F /Q faustgen_tilde_linux.zip
curl -L https://github.com/pierreguillot/faust-pd/releases/download/v%release_version%/faustgen_tilde-Linux-amd64-32-sources.zip -o faustgen_tilde_linux.zip
7z x faustgen_tilde_linux.zip
deken package -v%release_version% faustgen~
rmdir /S /Q faustgen~
del /F /Q faustgen_tilde_linux.zip

del /F /Q faustgen_tilde_windows.zip
curl -L https://github.com/pierreguillot/faust-pd/releases/download/v%release_version%/faustgen_tilde-Windows-amd64-32-sources.zip -o faustgen_tilde_windows.zip
7z x faustgen_tilde_windows.zip
deken package -v%release_version% faustgen~
rmdir /S /Q faustgen~
del /F /Q faustgen_tilde_windows.zip

del /F /Q faustgen_tilde_archive.zip
curl -L https://github.com/pierreguillot/faust-pd/archive/v%release_version%.zip -o faustgen_tilde_archive.zip
7z x faustgen_tilde_archive.zip
rename faust-pd-%release_version% faustgen~
deken package -v%release_version% faustgen~
rmdir /S /Q faustgen~
del /F /Q faustgen_tilde_archive.zip

rem deken upload faustgen~[v%release_version%](Darwin-amd64-32)(Sources).dek faustgen~[v%release_version%](Linux-amd64-32)(Sources).dek faustgen~[v%release_version%](Windows-amd64-32)(Sources).dek faustgen~[v%release_version%](Sources).dek
pause
