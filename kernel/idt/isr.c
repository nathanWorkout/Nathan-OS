int idt_set_entry(uint8_t num, uint32_t base, uint16_t selector, uint8_t flags) {
   idt[num].offset_low = base & 0xFFFF; 
   idt[num].offset_high = (base >> 16) & 0xFFFF;
   idt[num].selector = selector;
   idt[num].reserved = 0;
   idt[num].type_attribut = flags;
}

int idt_init() {
   asm volatile(lidt"%0": : "m"(idtr));
}
