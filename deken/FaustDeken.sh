#!/bin/bash

echo -e "Deken manager for faustgen~"

deken_prepare() {

rm -rf faustgen~
rm -f *.dek
rm -f *.dek.sha256
rm -f *.dek.txt

rm -f faustgen_tilde_darwin.zip
curl -L "https://github.com/pierreguillot/faust-pd/releases/download/v$1/faustgen_tilde-Darwin-amd64-32-sources.zip" -o faustgen_tilde_darwin.zip
tar zxvf faustgen_tilde_darwin.zip
deken package -v$1 faustgen~
rm -rf faustgen~
rm -f faustgen_tilde_darwin.zip

rm -f faustgen_tilde_linux.zip
curl -L "https://github.com/pierreguillot/faust-pd/releases/download/v$1/faustgen_tilde-Linux-amd64-32-sources.zip" -o faustgen_tilde_linux.zip
tar zxvf faustgen_tilde_linux.zip
deken package -v$1 faustgen~
rm -rf faustgen~
rm -f faustgen_tilde_linux.zip

rm -f faustgen_tilde_windows.zip
curl -L "https://github.com/pierreguillot/faust-pd/releases/download/v$1/faustgen_tilde-Windows-amd64-32-sources.zip" -o faustgen_tilde_windows.zip
tar zxvf faustgen_tilde_windows.zip
deken package -v$1 faustgen~
rm -rf faustgen~
rm -f faustgen_tilde_windows.zip

rm -f faustgen_tilde_archive.zip
curl -L "https://github.com/pierreguillot/faust-pd/archive/v$1.zip" -o faustgen_tilde_archive.zip
tar zxvf faustgen_tilde_archive.zip
mv pd-faustgen-$1 faustgen~
deken package -v$1 faustgen~
rm -rf faustgen~
rm -f faustgen_tilde_archive.zip

}

deken_upload() {
deken upload faustgen~[v$2](Darwin-amd64-32)(Sources).dek faustgen~[v$2](Linux-amd64-32)(Sources).dek faustgen~[v$2](Windows-amd64-32)(Sources).dek faustgen~[v$2](Sources).dek
}

if [ "$1" == "upload" ]; then
  deken_upload
else
  deken_prepare
fi
