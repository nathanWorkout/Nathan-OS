# Compilation tools
CC = i686-elf-gcc
AS = nasm
LD = i686-elf-gcc

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
IMG     = boot.img

# Source files (auto-detected)
C_SRCS = $(wildcard $(KERNEL)/*.c) \
         $(wildcard $(DRIVERS)/*.c) \
         $(wildcard $(LIB)/*.c)

# Object files
C_OBJS = $(patsubst %.c, $(BUILD)/%.o, $(notdir $(C_SRCS)))

# Compiler kernel
all: $(BUILD)/kernel.bin

$(BUILD)/kernel.bin: $(C_OBJS) linker.ld
	$(LD) -T linker.ld -o $@ $(C_OBJS) $(LDFLAGS)

$(BUILD)/%.o: $(KERNEL)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: $(DRIVERS)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: $(LIB)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Bootloader
$(BUILD)/stage1.bin: $(BOOT)/stage1.asm
	$(AS) -f bin $< -o $@

$(BUILD)/stage2.bin: $(BOOT)/stage2.asm
	$(AS) -f bin $< -o $@

img: $(BUILD)/stage1.bin $(BUILD)/stage2.bin
	dd if=/dev/zero of=$(IMG) bs=512 count=2048
	dd if=$(BUILD)/stage1.bin of=$(IMG) bs=512 seek=0 conv=notrunc
	dd if=$(BUILD)/stage2.bin of=$(IMG) bs=512 seek=1 conv=notrunc

run-img: img
	qemu-system-i386 -drive format=raw,file=$(IMG),index=0,media=disk

debug: img
	qemu-system-i386 -drive format=raw,file=$(IMG),index=0,media=disk -s -S

clean:
	rm -f $(BUILD)/*.o $(BUILD)/*.bin $(IMG)

.PHONY: all img run-img debug clean

# ==================================================
# COMMANDES
# ==================================================
# make              -> compile le kernel
# make img          -> compile stage1 + stage2 et crée boot.img
# make run-img      -> compile + crée boot.img + lance QEMU
# make debug        -> compile + crée boot.img + lance QEMU en attente de GDB
# make clean        -> supprime tous les fichiers compilés
#
# DEBUG GDB (dans un 2ème terminal après make debug) :
#   gdb -ex "target remote localhost:1234"
#   set pagination off
#   set architecture i8086
#   break *0x7c00       -> breakpoint début stage 1
#   break *0x7E00       -> breakpoint début stage 2
#   break *0xADDR       -> breakpoint à une adresse précise
#   c                   -> continuer jusqu'au prochain breakpoint
#   si                  -> exécuter une instruction (entre dans les interruptions)
#   ni                  -> exécuter une instruction (saute les interruptions)
#   x/30i 0x7c00        -> désassembler 30 instructions à cette adresse
#   info registers      -> afficher tous les registres
#   p/x $eax            -> afficher un registre en hexa
#
# VÉRIFICATIONS :
#   ls -la build/               -> vérifier que les .bin ne sont pas vides
#   hexdump -C boot.img | head  -> vérifier le contenu de l'image disque
# ==================================================