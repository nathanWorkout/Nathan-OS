void kmain(void)
{
    volatile unsigned short *vga = (unsigned short *)0xB8000;
    vga[0] = 0x2F00 | 'A';   // vert sur noir
    vga[1] = 0x2F00 | 'B';
    vga[2] = 0x2F00 | 'C';
    while (1);
}