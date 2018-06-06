#!/bin/bash

echo -e "title Deken manager for Faust~ for Pure Data"
release_version="$1"

rm -rf faust~
rm -f *.dek
rm -f *.dek.sha256
rm -f *.dek.txt

rm -f faust_tilde_darwin.zip
curl -L "https://github.com/pierreguillot/faust-pd/releases/download/v$1/faust_tilde-Darwin-amd64-32-sources.zip" -o faust_tilde_darwin.zip
tar zxvf faust_tilde_darwin.zip
deken package -v$1 faust~
rm -rf faust~
rm -f faust_tilde_darwin.zip

rm -f faust_tilde_linux.zip
curl -L "https://github.com/pierreguillot/faust-pd/releases/download/v$1/faust_tilde-Linux-amd64-32-sources.zip" -o faust_tilde_linux.zip
tar zxvf faust_tilde_linux.zip
deken package -v$1 faust~
rm -rf faust~
rm -f faust_tilde_linux.zip

rm -f faust_tilde_windows.zip
curl -L "https://github.com/pierreguillot/faust-pd/releases/download/v$1/faust_tilde-Windows-amd64-32-sources.zip" -o faust_tilde_windows.zip
tar zxvf faust_tilde_windows.zip
deken package -v$1 faust~
rm -rf faust~
rm -f faust_tilde_windows.zip

rm -f faust_tilde_archive.zip
curl -L "https://github.com/pierreguillot/faust-pd/archive/v$1.zip" -o faust_tilde_archive.zip
tar zxvf faust_tilde_archive.zip
mv faust-pd-$1 faust~
deken package -v$1 faust~
rm -rf faust~
rm -f faust_tilde_archive.zip

# deken upload faust~[v%release_version%](Darwin-amd64-32)(Sources).dek faust~[v%release_version%](Linux-amd64-32)(Sources).dek faust~[v%release_version%](Windows-amd64-32)(Sources).dek faust~[v%release_version%](Sources).dek
