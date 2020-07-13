

\gbdk\bin\lcc.exe -Wa-l -Wl-m -Wl-j -DUSE_SFR_FOR_REG -c -o build/main.o source/main.c
\gbdk\bin\lcc -Wa-l -Wl-m -Wl-j -DUSE_SFR_FOR_REG -c -o build/gbTimer.o source/gbTimer.c
\gbdk\bin\lcc -Wa-l -Wl-m -Wl-j -DUSE_SFR_FOR_REG -o gbReact.gb build/main.o build/gbTimer.o