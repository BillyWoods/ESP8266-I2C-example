CC = xtensa-lx106-elf-gcc
CFLAGS = -I. -mlongcalls -I/home/billy/esp8266/esp-open-sdk/sdk/include
LDLIBS = -L. -L/home/billy/esp8266/esp-open-sdk/sdk/lib -nostdlib -Wl,--start-group -lmain -ldriver -lnet80211 -lwpa -llwip -lpp -lphy -Wl,--end-group -lgcc
LDFLAGS = -Teagle.app.v6.ld

main.bin: main
	esptool.py elf2image $^

main: main.o

main.o: main.c

flash: main.bin
	esptool.py --port /dev/ttyUSB0 write_flash 0x00000 main-0x00000.bin 0x10000 main-0x10000.bin

clean:
	rm -f main main.o main-0x00000.bin main-0x10000.bin
