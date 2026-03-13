# ==================================================
# NATHANOS - MAKEFILE
# ==================================================

# Compilation tools
CC = i686-elf-gcc
AS = nasm
LD = i686-elf-ld

# Compilation flags
CFLAGS  = -std=gnu99 -ffreestanding -O2 -Wall -Wextra
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
            $(wildcard $(DRIVERS)/*.c) \
            $(wildcard $(LIB)/*.c)

# entry.asm exclu du wildcard car linké explicitement en premier
ASM_SRCS = $(filter-out $(KERNEL)/entry.asm, $(wildcard $(KERNEL)/*.asm))

# Object files
C_OBJS   = $(patsubst %.c,   $(BUILD)/%.o, $(notdir $(C_SRCS)))
ASM_OBJS = $(patsubst %.asm, $(BUILD)/%.o, $(notdir $(ASM_SRCS)))

# ==================================================
# KERNEL
# ==================================================

all: $(BUILD)/kernel.bin

# entry.o en premier pour que _start soit au bon offset dans kernel.bin
$(BUILD)/kernel.bin: $(BUILD)/entry.o $(ASM_OBJS) $(C_OBJS) linker.ld
	$(LD) -T linker.ld -o $@ $(BUILD)/entry.o $(ASM_OBJS) $(C_OBJS) $(LDFLAGS)

$(BUILD)/entry.o: $(KERNEL)/entry.asm
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD)/%.o: $(KERNEL)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: $(DRIVERS)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: $(LIB)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: $(KERNEL)/%.asm
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
#
# ORDRE CRITIQUE :
#   1. dd zero        -> image vierge
#   2. mkfs.fat       -> structure FAT32 (réserve secteurs 0-31)
#                        secteur 0 = boot sector
#                        secteur 1 = FSInfo
#                        secteur 6 = backup boot sector
#                        secteurs 2-5, 8-31 = libres (mis à 0)
#   3. dd stage1      -> secteur 0  (écrase le boot sector FAT32)
#   4. dd stage2      -> secteur 2  (APRÈS mkfs.fat sinon écrasé)
#   5. mcopy kernel   -> copie kernel.bin dans la partition FAT32
#
# stage2 au secteur 2 (LBA 2, CHS 0/0/3) car :
#   - secteur 0 = stage1 (MBR)
#   - secteur 1 = FSInfo (utilisé par FAT32)
#   - secteur 2 = libre, non écrasé par mkfs.fat avec -R 32
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

# ==================================================
# COMMANDES
# ==================================================
# make              -> compile le kernel
# make img          -> compile tout + crée boot.img
# make run-img      -> lance QEMU (sans recompiler)
# make debug        -> lance QEMU en attente de GDB (sans recompiler)
# make clean        -> supprime tous les fichiers compilés
#
# WORKFLOW NORMAL :
#   make clean && make img && make run-img
#
# VÉRIFICATIONS :
#   hexdump -C boot.img | grep -A2 "00000400"
#     -> doit afficher : 31 c0 8e d8 8e c0 8e d0  (stage2 ok)
#   hexdump -C boot.img | grep -A2 "00000200"
#     -> doit afficher : 52 52 61 41  (FSInfo - normal, c'est FAT32)
#
# DEBUG GDB (dans un 2ème terminal après make debug) :
#   gdb -ex "target remote localhost:1234"
#   set pagination off
#   set architecture i8086
#   break *0x7c00       -> breakpoint début stage1
#   break *0x7e00       -> breakpoint début stage2
#   break *0x100000     -> breakpoint début kernel
#   c                   -> continuer jusqu'au prochain breakpoint
#   si                  -> step instruction (entre dans les appels)
#   ni                  -> next instruction (saute les appels)
#   x/30i 0x7c00        -> désassembler 30 instructions
#   info registers      -> afficher tous les registres
#   p/x $eax            -> afficher un registre en hexa
# ==================================================