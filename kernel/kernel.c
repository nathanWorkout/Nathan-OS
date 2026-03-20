void kmain(void)
{
    volatile unsigned short *vga = (unsigned short *)0xB8000;
    vga[0] = 0x2F00 | 'H';   // vert sur noir pour bien voir la lettre
    vga[1] = 0x2F00 | 'E';
    vga[2] = 0x2F00 | 'L';
    vga[3] = 0x2F00 | 'L';
    vga[4] = 0x2F00 | 'O';
    vga[5] = 0x2F00 | '_';
    vga[6] = 0x2F00 | 'W';
    vga[7] = 0x2F00 | 'O';
    vga[8] = 0x2F00 | 'R';
    vga[9] = 0x2F00 | 'L';
    vga[10] = 0x2F00 | 'D';
    while (1); 
}
