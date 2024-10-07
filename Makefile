TGT 		:= mipsel-unknown-linux-gnu
CCFLAGS		:= -O3 -flto -march=mips1 -mips1 -fno-pic -mno-abicalls -static -nolibc -nostdlib -nodefaultlibs

OUT			:= demo.elf

all:    $(OUT)
.PHONY: all

$(OUT): main.c readjoy.s string.c gpu.c clock.c math.c input.c grass.c
	$(TGT)-gcc -Wl,-Map=main.map -T executable.ld $(CCFLAGS) -o $@ $^
	$(TGT)-objcopy --remove-section .pdr $@

clean:
	rm $(OUT)
.PHONY: clean