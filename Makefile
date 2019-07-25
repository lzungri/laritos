ARCH ?= arm
CPU ?= cortex-a15
ABI ?= armv7-a
CROSS_COMPILE ?= arm-none-eabi-

SRC_CPU = arch/$(ARCH)/$(CPU)
SRC_BOARD = board/qemu-arm-virt

OUT = bin

AS_FLAGS = -warn --fatal-warnings -march=$(ABI) -mcpu=$(CPU) -g

$(OUT)/laritos.img: Makefile $(OUT) $(OUT)/start.o $(OUT)/vectors.o $(SRC_BOARD)/memmap.ld
	$(CROSS_COMPILE)ld -T $(SRC_BOARD)/memmap.ld $(OUT)/start.o $(OUT)/vectors.o -o $(OUT)/laritos.elf
	$(CROSS_COMPILE)objcopy -O binary $(OUT)/laritos.elf $(OUT)/laritos.bin

	@echo "Creating flash image"
	dd if=/dev/zero of=$(OUT)/laritos.img bs=1M count=64 status=none
	dd if=$(OUT)/laritos.bin of=$(OUT)/laritos.img conv=notrunc status=none
	@echo "Raw image saved in $(OUT)/laritos.img"

$(OUT):
	mkdir -p $(OUT)

$(OUT)/start.o: $(SRC_CPU)/start.S
	$(CROSS_COMPILE)as $(AS_FLAGS) $(SRC_CPU)/start.S -o $(OUT)/start.o

$(OUT)/vectors.o: $(SRC_CPU)/vectors.S
	$(CROSS_COMPILE)as $(AS_FLAGS) $(SRC_CPU)/vectors.S -o $(OUT)/vectors.o

clean:
	rm -fr $(OUT)

qemu: $(OUT)/laritos.img
	qemu-system-arm -M virt -smp 4 -m 1G -cpu cortex-a15 -nographic -S -s -drive if=pflash,file=$(OUT)/laritos.img,format=raw
