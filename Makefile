# ==================================================
# NATHANOS - MAKEFILE
# ==================================================

# Compilation tools
CC = i686-elf-gcc
AS = nasm
LD = i686-elf-ld

# Compilation flags
CFLAGS  = -std=gnu99 -ffreestanding -O2 -Wall -Wextra -I kernel/idt
ASFLAGS = -f elf32
LDFLAGS = -nostdlib

# Folders
BUILD   = build
BOOT    = boot
KERNEL  = kernel
DRIVERS = drivers
LIB     = lib
IMG     = boot.img

# Source files
C_SRCS   = $(wildcard $(KERNEL)/*.c) \
            $(wildcard $(KERNEL)/idt/*.c) \
            $(wildcard $(DRIVERS)/*.c) \
            $(wildcard $(LIB)/*.c)

ASM_SRCS = $(filter-out $(KERNEL)/entry.asm, $(wildcard $(KERNEL)/*.asm)) \
            $(wildcard $(KERNEL)/idt/*.asm)

# Object files
C_OBJS   = $(patsubst %.c,   $(BUILD)/%.o, $(notdir $(C_SRCS)))
ASM_OBJS = $(patsubst %.asm, $(BUILD)/%.o, $(notdir $(ASM_SRCS)))

# ==================================================
# KERNEL
# ==================================================

all: $(BUILD)/kernel.bin

$(BUILD)/kernel.bin: $(BUILD)/entry.o $(ASM_OBJS) $(C_OBJS) linker.ld
	$(LD) -T linker.ld -o $@ $(BUILD)/entry.o $(ASM_OBJS) $(C_OBJS) $(LDFLAGS)

$(BUILD)/entry.o: $(KERNEL)/entry.asm
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD)/%.o: $(KERNEL)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: $(KERNEL)/idt/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: $(DRIVERS)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: $(LIB)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: $(KERNEL)/%.asm
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD)/%.o: $(KERNEL)/idt/%.asm
	$(AS) $(ASFLAGS) $< -o $@

# ==================================================
# BOOTLOADER
# ==================================================

$(BUILD)/stage1.bin: $(BOOT)/stage1.asm
	$(AS) -f bin $< -o $@

$(BUILD)/stage2.bin: $(BOOT)/stage2.asm
	$(AS) -f bin $< -o $@

# ==================================================
# IMAGE DISQUE
# ==================================================

img: $(BUILD)/stage1.bin $(BUILD)/stage2.bin $(BUILD)/kernel.bin
	dd if=/dev/zero      of=$(IMG) bs=512 count=524288
	mkfs.fat -F 32 -R 32 -S 512 $(IMG)
	dd if=$(BUILD)/stage1.bin of=$(IMG) bs=512 seek=0 conv=notrunc
	dd if=$(BUILD)/stage2.bin of=$(IMG) bs=512 seek=2 conv=notrunc
	mcopy -i $(IMG) $(BUILD)/kernel.bin ::kernel.bin
	@echo ""
	@echo "Vérification : stage2 doit commencer par 31 c0 8e d8 à l'offset 0x400"
	@hexdump -C $(IMG) | grep -A1 "00000400"

run-img:
	qemu-system-i386 -drive format=raw,file=$(IMG),index=0,media=disk

debug:
	qemu-system-i386 -drive format=raw,file=$(IMG),index=0,media=disk -s -S

clean:
	rm -f $(BUILD)/*.o $(BUILD)/*.bin $(IMG)

.PHONY: all img run-img debug clean
