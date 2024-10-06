TGT := mipsel-unknown-linux-gnu
GCC	:= $(TGT)-gcc -g -O3 -fno-pic -mno-abicalls -mfp32 -mips1 -march=mips1 -nolibc -nostdlib
LD  := $(TGT)-ld

OBJS	:= main.o readjoy.o string.o
# math.o clock.o gpu.o input.o
all:    main.exe

.PHONY: all

main.exe:  $(OBJS)
	$(LD) -o main.exe -Map=main.map -T ps-exe.ld $(OBJS) 


%.o:    %.s
	$(GCC) -c -o $*.o $*.s

%.o:    %.c
	$(GCC) -c -o $*.o $*.c

clean:
	rm *.o
	rm *.exe

.PHONY: clean
