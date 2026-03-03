# Compilation tools
CC = i686-elf-gcc        # C compiler for x86
AS = nasm                # Assembler
LD = i686-elf-gcc        # Linker

# Compilation flags
CFLAGS  = -std=gnu99 -ffreestanding -O2 -Wall -Wextra
ASFLAGS = -f elf32
LDFLAGS = -ffreestanding -O2 -nostdlib

# Folders
BUILD   = build
BOOT    = boot
KERNEL  = kernel
DRIVERS = drivers
LIB     = lib

# Source files (auto-detected)
ASM_SRCS = $(wildcard $(BOOT)/*.asm)
C_SRCS   = $(wildcard $(KERNEL)/*.c) \
			$(wildcard $(DRIVERS)/*.c) \
			$(wildcard $(LIB)/*.c)

# Object files
ASM_OBJS = $(patsubst %.asm, $(BUILD)/%.o, $(notdir $(ASM_SRCS)))
C_OBJS   = $(patsubst %.c,   $(BUILD)/%.o, $(notdir $(C_SRCS)))
OBJS     = $(ASM_OBJS) $(C_OBJS)

# Main target
all: $(BUILD)/kernel.bin

# Link all .o into kernel.bin
$(BUILD)/kernel.bin: $(OBJS) linker.ld
	$(LD) -T linker.ld -o $@ $(OBJS) $(LDFLAGS)

# Compile ASM → .o
$(BUILD)/%.o: $(BOOT)/%.asm
	$(AS) $(ASFLAGS) $< -o $@

# Compile C → .o
$(BUILD)/%.o: $(KERNEL)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: $(DRIVERS)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: $(LIB)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Generate bootable ISO
iso: $(BUILD)/kernel.bin
	cp $(BUILD)/kernel.bin iso/boot/
	grub-mkrescue -o nathanos.iso iso

# Run in QEMU
run: nathanos.iso
	qemu-system-i386 -cdrom nathanos.iso

# Clean build files
clean:
	rm -f $(BUILD)/*.o $(BUILD)/kernel.bin nathanos.iso

.PHONY: all iso run clean


# make            -> compile everything
# make iso        -> generate bootable ISO
# make run        -> launch in QEMU
# make clean      -> delete compiled files