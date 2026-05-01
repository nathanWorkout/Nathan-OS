
# Outils 
CC  = x86_64-elf-gcc
AS  = nasm
LD  = x86_64-elf-ld

# Flags compilateur
CFLAGS = \
    -m64                  \
    -mcmodel=kernel       \
    -mno-red-zone         \
    -mno-mmx              \
    -mno-sse              \
    -ffreestanding        \
    -fno-stack-protector  \
    -O2                   \
    -Wall -Wextra         \
    -I kernel             \
    -I kernel/idt         \
    -I kernel/tty         \
    -I kernel/serial      \
    -I kernel/pic         \
    -I kernel/io          \
    -I kernel/pit         \
    -I kernel/memory      \
    -I kernel/drivers     \
    -I kernel/drivers/keyboard \
    -I kernel/shell       \
    -I kernel/lib         \
    -I kernel/proc        \
    -I kernel/Graphic

ASFLAGS = -f elf64

LDFLAGS = -m elf_x86_64 -T linker.ld --no-warn-rwx-segments -nostdlib


BUILD  = build
KERNEL = kernel
IMG    = boot.img
LIMINE = $(HOME)/limine


C_SRCS = \
    $(wildcard $(KERNEL)/*.c)                   \
    $(wildcard $(KERNEL)/idt/*.c)               \
    $(wildcard $(KERNEL)/tty/*.c)               \
    $(wildcard $(KERNEL)/serial/*.c)            \
    $(wildcard $(KERNEL)/lib/*.c)               \
    $(wildcard $(KERNEL)/pic/*.c)               \
    $(wildcard $(KERNEL)/pit/*.c)               \
    $(wildcard $(KERNEL)/memory/*.c)            \
    $(wildcard $(KERNEL)/drivers/*.c)           \
    $(wildcard $(KERNEL)/drivers/keyboard/*.c)  \
    $(wildcard $(KERNEL)/shell/*.c)             \
    $(wildcard $(KERNEL)/proc/*.c)              \
		$(wildcard $(KERNEL)/Graphic/*.c) 


ASM_SRCS = \
    $(filter-out $(KERNEL)/entry.asm, $(wildcard $(KERNEL)/*.asm)) \
    $(wildcard $(KERNEL)/idt/*.asm)   \
    $(wildcard $(KERNEL)/proc/*.asm)


C_OBJS   = $(patsubst %.c,   $(BUILD)/%.o, $(notdir $(C_SRCS)))
ASM_OBJS = $(patsubst %.asm, $(BUILD)/%.o, $(notdir $(ASM_SRCS)))

all: $(BUILD)/kernel.elf

$(BUILD)/kernel.elf: $(BUILD)/entry.o $(ASM_OBJS) $(C_OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(BUILD)/entry.o $(ASM_OBJS) $(C_OBJS)
	@echo "[OK] kernel.elf généré"
	@x86_64-elf-readelf -h $@ | grep "Class" 
	@x86_64-elf-readelf -S $@ | grep -E "\.text|\.rodata|\.data|\.bss"

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
$(BUILD)/%.o: $(KERNEL)/Graphic/%.c
	$(CC) $(CFLAGS) -c $< -o $@


# Règles génériques ASM
$(BUILD)/%.o: $(KERNEL)/%.asm
	$(AS) $(ASFLAGS) $< -o $@
$(BUILD)/%.o: $(KERNEL)/idt/%.asm
	$(AS) $(ASFLAGS) $< -o $@
$(BUILD)/%.o: $(KERNEL)/proc/%.asm
	$(AS) $(ASFLAGS) $< -o $@

img: $(BUILD)/kernel.elf
	dd if=/dev/zero of=$(IMG) bs=512 count=524288
	printf '\x80\x00\x02\x00\x0c\xfe\xff\xff\x00\x08\x00\x00\x00\xf8\x07\x00' | dd of=$(IMG) bs=1 seek=446 conv=notrunc
	printf '\x55\xaa' | dd of=$(IMG) bs=1 seek=510 conv=notrunc
	mformat -i $(IMG)@@1M -F -v VAULTOS ::
	mmd -i $(IMG)@@1M ::/boot
	mcopy -i $(IMG)@@1M $(BUILD)/kernel.elf ::/boot/kernel.elf
	mcopy -i $(IMG)@@1M limine.conf ::/limine.conf
	mcopy -i $(IMG)@@1M $(LIMINE)/limine-bios.sys ::/limine-bios.sys
	$(LIMINE)/limine bios-install $(IMG)
	
# QEMU
run: $(IMG)
	qemu-system-x86_64 \
	    -drive format=raw,file=$(IMG),index=0,media=disk \
	    -serial stdio \
	    -m 128M \
	    -display sdl

debug: $(IMG)
	qemu-system-x86_64 \
	    -drive format=raw,file=$(IMG),index=0,media=disk \
	    -serial stdio \
	    -m 128M \
	    -s -S -display gtk

clean:
	rm -f $(BUILD)/*.o $(BUILD)/*.elf $(IMG)

.PHONY: all img run debug clean
