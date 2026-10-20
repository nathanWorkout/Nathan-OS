[BITS 64]
[global _start]
[global kernel_stack_top]
[extern kmain]

section .bss
align 16

kernel_stack_bottom:
  resb 16384

kernel_stack_top:

section .text

_start:
  ; enable SSE
  mov rax, cr0
  and ax, 0xFFFB ; disable FPU emulation
  or ax, 0x2     ; active monitor coprocessor
  mov cr0, rax

  ; cr4 : activates CPU extensions
  mov rax, cr4
  or rax, (1 << 9)  ; allows FXSAVE/FXRSTOR and SSE
  or rax, (1 << 10) ; Enables #XF exceptions for SSE
  mov cr4, rax

  ; gdt segments
  mov ax, 0x10
  mov ss, ax
  mov ds, ax
  mov es, ax

  ; set fs and gs to 0 because they are not used in 64 bits mode
  xor ax, ax
  mov fs, ax
  mov gs, ax

  mov rsp, kernel_stack_top
  ; align to 16 bytes (ABI System V x86_64)
  and rsp, ~0xf

  ; start of the stack
  xor rbp, rbp

  call kmain

  cli

.halt:
  hlt
  jmp .halt
