#define vmxvmappn(...) vmappn(opbuf, i, __VA_ARGS__, sizeof(__VA_ARGS__))

static void exit_vm(uint8_t *opbuf, size_t *i, uint16_t exit_ip, uint8_t exit_cycles)
{
    // 0x15 Bytes.

    vmxvmappn(@@asm("mov word [0x12345678],0x9abc", [0x78, 0x56, 0x34, 0x12], "(uintptr_t)&vm_ip", [0xbc, 0x9a], "exit_ip"));
    vmxvmappn(@@asm("mov byte [0x12345678],0xff", [0x78, 0x56, 0x34, 0x12], "(uintptr_t)&cycles_gone", [0xff], "exit_cycles"));
    size_t here = *i;
    vmxvmappn(@@asm("jmp dword 0x12345678", [0x78 - 5, 0x56, 0x34, 0x12], "(uint32_t)(CODE_EXITI - (here + 5))"));
}

static void exit_vm_by_ret(uint8_t *opbuf, size_t *i, uint8_t exit_cycles)
{
    // 0x1E Bytes.

    vmxvmappn(@@asm("lahf \n push eax \n mov ax,[ebp] \n add bp,2"));
    vmxvmappn(@@asm("mov [0x12345678],ax", [0x78, 0x56, 0x34, 0x12], "(uintptr_t)&vm_ip"));
    vmxvmappn(@@asm("pop eax \n sahf"));
    vmxvmappn(@@asm("mov byte [0x12345678],0xff", [0x78, 0x56, 0x34, 0x12], "(uintptr_t)&cycles_gone", [0xff], "exit_cycles"));
    size_t here = *i;
    vmxvmappn(@@asm("jmp dword 0x12345678", [0x78 - 5, 0x56, 0x34, 0x12], "(uint32_t)(CODE_EXITI - (here + 5))"));
}
