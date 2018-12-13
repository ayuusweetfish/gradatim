Use MinGW with MSYS (2013072300)

```
cd src
mkdir build
cd build
cmake -G 'Unix Makefiles' -DCMAKE_BUILD_TYPE=RELEASE -DSDL2_BUILDING_LIBRARY=1 ..
make
```
