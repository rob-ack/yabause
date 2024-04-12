
# how to build


```
mkdir build
cd build
cmake ../yabause -DCMAKE_TOOLCHAIN_FILE=../yabause/src/ios/ios.toolchain.cmake -DPLATFORM=OS64 -DYAB_PORTS=ios -DYAB_WANT_C68K=FALSE -DYAB_WANT_SH2_CACHE=TRUE -DSH2_DYNAREC=FALSE -DYAB_WANT_DYNAREC_DEVMIYAX=FALSE
```