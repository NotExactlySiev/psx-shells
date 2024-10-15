TGT 		:= mipsel-unknown-none-elf
CCFLAGS		:= -O3 -flto -Wl,--oformat=elf32-littlemips -mips1 -march=r3000 \
		   -G0 -mno-abicalls -static -nolibc -nostdlib

all:    demo.exe demo.elf
.PHONY: all

demo.elf: main.c readjoy.s string.c gpu.c clock.c math.c input.c grass.c camera.c
	$(TGT)-gcc -Wl,-Map=main.map -T ps-exe.ld $(CCFLAGS) -o $@ $^

demo.exe: demo.elf
	$(TGT)-objcopy -O binary $^ $@

clean:
	rm demo.elf demo.exe
.PHONY: clean
