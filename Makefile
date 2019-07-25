ARCH ?= arm
CROSS_COMPILE ?= arm-none-eabi-

SRC = arch/arm/cortex-a15
BOARD = board/qemu-arm-virt

OUT = bin

AS_FLAGS = -warn --fatal-warnings -march=armv7-a -mcpu=cortex-a15 -g

$(OUT)/laritos.img: $(OUT) $(OUT)/start.o $(OUT)/vectors.o $(BOARD)/memmap.ld
	$(CROSS_COMPILE)ld -T $(BOARD)/memmap.ld $(OUT)/start.o $(OUT)/vectors.o -o $(OUT)/laritos.elf
	$(CROSS_COMPILE)objcopy -O binary $(OUT)/laritos.elf $(OUT)/laritos.bin

	@echo "Creating flash image"
	dd if=/dev/zero of=$(OUT)/laritos.img bs=1M count=64 status=none
	dd if=$(OUT)/laritos.bin of=$(OUT)/laritos.img conv=notrunc status=none

$(OUT):
	mkdir -p $(OUT)

$(OUT)/start.o: $(SRC)/start.S
	$(CROSS_COMPILE)as $(AS_FLAGS) $(SRC)/start.S -o $(OUT)/start.o

$(OUT)/vectors.o: $(SRC)/vectors.S
	$(CROSS_COMPILE)as $(AS_FLAGS) $(SRC)/vectors.S -o $(OUT)/vectors.o

clean:
	rm -fr $(OUT)

qemu: $(OUT)/laritos.img
	qemu-system-arm -M virt -smp 4 -m 1G -cpu cortex-a15 -nographic -S -s -drive file=$(OUT)/laritos.img,format=raw,if=pflash
