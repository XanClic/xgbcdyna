static void exit_vm(uint8_t *opbuf, size_t *i, size_t ei, uint16_t exit_ip, uint8_t exit_cycles)
{
    // mov word [vm_ip],exit_ip
    vmapp2(opbuf, *i, 0xC766);
    vmapp1(opbuf, *i, 0x05);
    vmapp4(opbuf, *i, (uintptr_t)&vm_ip);
    // mov byte [cycles_gone],exit_cycles
    vmapp4(opbuf, *i, exit_ip | 0x05C60000);
    vmapp4(opbuf, *i, (uintptr_t)&cycles_gone);
    // jmp dword ei
    vmapp2(opbuf, *i, exit_cycles | 0xE900);
    vmapp4(opbuf, *i, (uint32_t)(ei - (*i + 4)));
}

static void exit_vm_by_ret(uint8_t *opbuf, size_t *i, size_t ei, uint8_t exit_cycles)
{
    // lahf; push eax; mov ax,[ebp]
    vmapp4(opbuf, *i, 0x8B66509F);
    // add bp,2
    vmapp4(opbuf, *i, 0x83660045);
    // mov word [vm_ip],ax
    vmapp4(opbuf, *i, 0xA36602C5);
    vmapp4(opbuf, *i, (uintptr_t)&vm_ip);
    // pop eax; sahf; mov byte [cycles_gone],exit_cycles
    vmapp4(opbuf, *i, 0x05C69E58);
    vmapp4(opbuf, *i, (uintptr_t)&cycles_gone);
    // jmp dword ei
    vmapp2(opbuf, *i, exit_cycles | 0xE900);
    vmapp4(opbuf, *i, (uint32_t)(ei - (*i + 4)));
}
