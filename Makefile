TGT 		:= mipsel-unknown-linux-gnu
CCFLAGS		:= -Wl,--oformat=elf32-tradlittlemips -O3 -flto -march=mips1 \
			   -mips1 -fno-pic -mno-abicalls -static -nolibc -nostdlib

all:    demo.pex demo.elf
.PHONY: all

demo.elf: main.c readjoy.s string.c gpu.c clock.c math.c input.c grass.c
	$(TGT)-gcc -Wl,-Map=main.map -T ps-exe.ld $(CCFLAGS) -o $@ $^

demo.exe: demo.elf
	$(TGT)-objcopy -O binary $^ $@

clean:
	rm demo.elf demo.exe
.PHONY: clean