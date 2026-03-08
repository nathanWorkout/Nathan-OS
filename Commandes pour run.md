# Compiler
nasm -f bin boot/stage1.asm -o build/stage1.bin
nasm -f bin boot/stage2.asm -o build/stage2.bin

# Vérifier que les binaires ne sont pas vides
ls -la build/
hexdump -C boot.img | head -40  # secteur 0 = stage1, secteur 1 = stage2

# Créer l'image disque
dd if=/dev/zero of=boot.img bs=512 count=2048
dd if=build/stage1.bin of=boot.img bs=512 seek=0 conv=notrunc
dd if=build/stage2.bin of=boot.img bs=512 seek=1 conv=notrunc

OU make img

# Lancer sans debug
qemu-system-i386 -drive format=raw,file=boot.img,index=0,media=disk


# DEBUG — Terminal 1 (lancer en premier, QEMU doit être figé)
qemu-system-i386 -drive format=raw,file=boot.img,index=0,media=disk -s -S

# DEBUG — Terminal 2
gdb -ex "target remote localhost:1234"

# Dans GDB, toujours commencer par :
set pagination off
set architecture i8086

# En une commande : 
gdb -ex "target remote localhost:1234" -ex "set pagination off" -ex "set architecture i8086"

# Voir le code désassemblé à une adresse
x/30i 0x7c00    # stage 1
x/30i 0x7E00    # stage 2

# Poser un breakpoint et continuer
break *0x7c00   # début du MBR
break *0x7E00   # début du stage 2
break *0x7c25   # adresse d'une instruction précise (trouver avec x/30i)
c               # continuer jusqu'au prochain breakpoint

x/30i 0x7c00    # stage 1
x/30i 0x7E00    # stage 2

# Avancer instruction par instruction
si              # step into (rentre dans les interruptions)
ni              # next instruction (saute par dessus les interruptions)

# Inspecter les registres
info registers  # tous les registres
p/x $eax        # un registre précis en hexa
p/d $ebx         ; un registre en décimal

# Hex jump
hexdump -C boot.img | head -40

# Vérifier le résultat d'un int 0x13
# Après le breakpoint juste après le int 0x13 :
#   eax -> ah doit valoir 0x00 (succès)
#   eflags → ne doit PAS contenir CF (carry flag = erreur)
#   es -> doit valoir 0x7E0 si la destination était 0x7E00

# Inspecter la mémoire
x/4xb 0x9000     ; 4 octets en hexa à 0x9000
x/16xb 0x9000    ; 16 octets en hexa
x/s 0x9000       ; string à cette adresse
x/4xw 0x9000     ; 4 mots de 32 bits

# Désasembler
x/10i 0x7E00     ; désassembler 10 instructions à 0x7E00


