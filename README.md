## graphic sandbox
for performance test

## debug
```bash
cmake --build build && .\build\bin\GraphicSandbox.exe
```

## compile (Debug)
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
```

## compile (Release)
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
```