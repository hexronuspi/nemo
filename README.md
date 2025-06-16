To run,

1. Install WSL2

2. Install mingw32-w63_x86 - g++ 

```
cd src
x86_64-w64-mingw32-g++ \
  -static -static-libgcc -static-libstdc++ \
  -std=c++20 \
  -I../include \
  main.cpp \
  data_loader.cpp \
  algo/simple_moving_average.cpp \
  metrics/backtester.cpp \
  logger/logger.cpp \
  -o BacktestingApp.exe
```