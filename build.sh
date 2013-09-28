gcc -Wl,--export-dynamic -Wall -O1 -shared -fPIC xfn.c -o xfn.so `pkg-config --cflags --libs gtk+-3.0` -lnotify
