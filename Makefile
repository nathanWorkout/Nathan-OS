# ==================================================
# NATHANOS - MAKEFILE
# ==================================================

# Outils
CC = i686-elf-gcc
AS = nasm
LD = i686-elf-ld

# Flags
CFLAGS  = -std=gnu99 -ffreestanding -O2 -Wall -Wextra -I kernel/idt -I kernel/tty -I kernel/serial -I kernel/pic -I kernel/io -I kernel/pit -I kernel/memory -I kernel/drivers -I kernel/drivers/keyboard -I kernel/shell -I kernel/lib -I kernel/proc


ASFLAGS = -f elf32
LDFLAGS = -nostdlib

# Dossiers
BUILD  = build
BOOT   = boot
KERNEL = kernel
IMG    = boot.img

# Sources
C_SRCS   = $(wildcard $(KERNEL)/*.c)        \
            $(wildcard $(KERNEL)/idt/*.c)    \
            $(wildcard $(KERNEL)/tty/*.c)    \
            $(wildcard $(KERNEL)/serial/*.c) \
            $(wildcard $(KERNEL)/lib/*.c)    \
            $(wildcard $(KERNEL)/pic/*.c)    \
	    			$(wildcard $(KERNEL)/pit/*.c)    \
	    			$(wildcard $(KERNEL)/memory/*.c) \
						$(wildcard $(KERNEL)/drivers/*.c) \
						$(wildcard $(KERNEL)/drivers/keyboard/*.c) \
						$(wildcard $(KERNEL)/shell/*.c) \
						$(wildcard $(KERNEL)/proc/*.c)

ASM_SRCS = $(filter-out $(KERNEL)/entry.asm, $(wildcard $(KERNEL)/*.asm)) \
            $(wildcard $(KERNEL)/idt/*.asm) \
						$(wildcard $(KERNEL)/proc/*.asm)

# Objets
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

$(BUILD)/%.o: $(KERNEL)/tty/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: $(KERNEL)/serial/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: $(KERNEL)/lib/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: $(KERNEL)/%.asm
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD)/%.o: $(KERNEL)/idt/%.asm
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD)/%.o: $(KERNEL)/pic/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: $(KERNEL)/pit/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: $(KERNEL)/memory/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: $(KERNEL)/drivers/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: $(KERNEL)/drivers/keyboard/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: $(KERNEL)/shell/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: $(KERNEL)/proc/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: $(KERNEL)/proc/%.asm
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
	dd if=/dev/zero of=$(IMG) bs=512 count=524288
	mkfs.fat -F 32 -R 32 -S 512 $(IMG)
	dd if=$(BUILD)/stage1.bin of=$(IMG) bs=512 seek=0 conv=notrunc
	dd if=$(BUILD)/stage2.bin of=$(IMG) bs=512 seek=2 conv=notrunc
	mcopy -i $(IMG) $(BUILD)/kernel.bin ::kernel.bin

run-img:
	#qemu-system-i386 -drive format=raw,file=boot.img,index=0,media=disk -serial stdio -d int,cpu 2>&1 | tail -200 > /tmp/qemu_log.txt && cat /tmp/qemu_log.txt
	qemu-system-i386 -drive format=raw,file=$(IMG),index=0,media=disk -serial stdio   


debug:
	qemu-system-i386 -drive format=raw,file=$(IMG),index=0,media=disk -serial stdio -s -S -display gtk 
clean:
	rm -f $(BUILD)/*.o $(BUILD)/*.bin $(IMG)

.PHONY: all img run-img debug clean
