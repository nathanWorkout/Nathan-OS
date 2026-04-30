extern isr_handler
extern irq0_handler
extern irq1_handler
global isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7
global isr8, isr9, isr10, isr11, isr12, isr13, isr14, isr15
global isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23
global isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31
global isr_common
global irq0, irq1

; La c'est inversé mais c'est normal car lors de la réception en c, 
; les arguments sont inversé c'est le dernier en assembelur qui est apssé en premier
isr0:
   push 0        ; Faux error code
   push 0        ; Le numéro
   jmp isr_common; 2 push car c encore la faute d'intel ils ont décider que certains code d'errur sont moins important donc sont pas push par le CPU

isr1:
   push 0
   push 1
   jmp isr_common

isr2:
   push 0
   push 2
   jmp isr_common

isr3:
   push 0
   push 3
   jmp isr_common

isr4:
   push 0
   push 4
   jmp isr_common

isr5:
   push 0
   push 5
   jmp isr_common

isr6:
   push 0
   push 6
   jmp isr_common

isr7:
   push 0
   push 7
   jmp isr_common

isr8:
   push 8
   jmp isr_common

isr9:
   push 0
   push 9
   jmp isr_common

isr10:
   push 10
   jmp isr_common

isr11:
   push 11
   jmp isr_common

isr12:
   push 12
   jmp isr_common

isr13:
   push 13
   jmp isr_common

isr14:
   push 14
   jmp isr_common

isr15:
   push 0
   push 15
   jmp isr_common

isr16:
   push 0
   push 16
   jmp isr_common

isr17:
   push 17
   jmp isr_common

isr18:
   push 0
   push 18
   jmp isr_common

isr19:
   push 0
   push 19
   jmp isr_common

isr20:
   push 0
   push 20
   jmp isr_common

isr21:
   push 21
   jmp isr_common

isr22:
   push 0
   push 22
   jmp isr_common

isr23:
   push 0
   push 23
   jmp isr_common

isr24:
   push 0
   push 24
   jmp isr_common

isr25:
   push 0
   push 25
   jmp isr_common

isr26:
   push 0
   push 26
   jmp isr_common

isr27:
   push 0
   push 27
   jmp isr_common

isr28:
   push 0
   push 28
   jmp isr_common

isr29:
   push 0
   push 29
   jmp isr_common

isr30:
   push 0
   push 30
   jmp isr_common

isr31:
   push 0
   push 31
   jmp isr_common

isr_common:
   push rax
   push rbx
   push rcx
   push rdx
   push rsi
   push rdi
   push rbp
   push r8
   push r9
   push r10
   push r11
   push r12
   push r13
   push r14
   push r15
   cld
   
   mov rdi, rsp
   call isr_handler

   pop r15
   pop r14
   pop r13
   pop r12
   pop r11
   pop r10
   pop r9
   pop r8
   pop rbp
   pop rdi
   pop rsi
   pop rdx
   pop rcx
   pop rbx
   pop rax
   add rsp, 8
   iret

irq0:
   push rax
   push rbx
   push rcx
   push rdx
   push rsi
   push rdi
   push rbp
   push r8
   push r9
   push r10
   push r11
   push r12
   push r13
   push r14
   push r15

   call irq0_handler

   mov al, 0x20
   out 0x20, al

   pop r15
   pop r14
   pop r13
   pop r12
   pop r11
   pop r10
   pop r9
   pop r8
   pop rbp
   pop rdi
   pop rsi
   pop rdx
   pop rcx
   pop rbx
   pop rax

   iret

irq1:
   push rax
   push rbx
   push rcx
   push rdx
   push rsi
   push rdi
   push rbp
   push r8
   push r9
   push r10
   push r11
   push r12
   push r13
   push r14
   push r15

  call irq1_handler

  mov al, 0x20
  out 0x20, al

   pop r15
   pop r14
   pop r13
   pop r12
   pop r11
   pop r10
   pop r9
   pop r8
   pop rbp
   pop rdi
   pop rsi
   pop rdx
   pop rcx
   pop rbx
   pop rax

  iret
