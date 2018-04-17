# faust-pd
The FAUST compiler embedded in a Pd external

## Dependencies
- [faust](https://github.com/grame-cncm/faust.git)
- [pure-data](https://github.com/pure-data/pure-data.git)
- [pd.build](https://github.com/pierreguillot/pd.build.git)

## Compilation

- macos
```
- brew install pkg-config
- git submodule update --init --recursive
- mkdir faust/build/personal && cd faust/build/personal
- cmake .. -DINCLUDE_STATIC=on -DINCLUDE_OSC=off -DINCLUDE_HTTP=off
- make
```
- linux
