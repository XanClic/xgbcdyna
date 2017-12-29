#define DYNAREC_LOAD 8
#define DYNAREC_CNST 0

#define drop(n) static void _dynarec_##n(void)
#define dran(o, n) [o] = &_dynarec_##n

static uint8_t *drc;
static size_t dri; // Instruction position in x86 stream?
static uint16_t drip; // Instruction position in Z80 stream?
static unsigned dr_cycle_counter;

#define drvmapp1(v) vmapp1(drc, dri, v)
#define drvmapp2(v) vmapp2(drc, dri, v)
#define drvmapp4(v) vmapp4(drc, dri, v)

#define drvmappn(...) vmappn(drc, &dri, __VA_ARGS__, sizeof(__VA_ARGS__))

#define JUMP_DISTANCE_START() \
    size_t _start_dri = dri

#define ASSERT_JUMP_DISTANCE(dist) \
    assert(_start_dri + dist == dri)

drop(jr_n)
{
    drip += (int8_t)MEM8(drip) + 1;
}

drop(jrnz_n)
{
    JUMP_DISTANCE_START();
    drvmappn(@@asm("jz $+0x17"));
    exit_vm(drc, &dri, drip + (int8_t)MEM8(drip) + 1, dr_cycle_counter);
    ASSERT_JUMP_DISTANCE(0x17);
    drip++;
}

drop(jrz_n)
{
    JUMP_DISTANCE_START();
    drvmappn(@@asm("jnz $+0x17"));
    exit_vm(drc, &dri, drip + (int8_t)MEM8(drip) + 1, dr_cycle_counter);
    ASSERT_JUMP_DISTANCE(0x17);
    drip++;
}

drop(jrnc_n)
{
    JUMP_DISTANCE_START();
    drvmappn(@@asm("jc $+0x17"));
    exit_vm(drc, &dri, drip + (int8_t)MEM8(drip) + 1, dr_cycle_counter);
    ASSERT_JUMP_DISTANCE(0x17);
    drip++;
}

drop(jrc_n)
{
    JUMP_DISTANCE_START();
    drvmappn(@@asm("jnc $+0x17"));
    exit_vm(drc, &dri, drip + (int8_t)MEM8(drip) + 1, dr_cycle_counter);
    ASSERT_JUMP_DISTANCE(0x17);
    drip++;
}

drop(retnz)
{
    JUMP_DISTANCE_START();
    drvmappn(@@asm("jz $+0x20"));
    exit_vm_by_ret(drc, &dri, dr_cycle_counter);
    ASSERT_JUMP_DISTANCE(0x20);
}

drop(retz)
{
    JUMP_DISTANCE_START();
    drvmappn(@@asm("jnz $+0x20"));
    exit_vm_by_ret(drc, &dri, dr_cycle_counter);
    ASSERT_JUMP_DISTANCE(0x20);
}

drop(retnc)
{
    JUMP_DISTANCE_START();
    drvmappn(@@asm("jc $+0x20"));
    exit_vm_by_ret(drc, &dri, dr_cycle_counter);
    ASSERT_JUMP_DISTANCE(0x20);
}

drop(retc)
{
    JUMP_DISTANCE_START();
    drvmappn(@@asm("jnc $+0x20"));
    exit_vm_by_ret(drc, &dri, dr_cycle_counter);
    ASSERT_JUMP_DISTANCE(0x20);
}

drop(jp_nn)
{
    drip = MEM16(drip);
}

drop(jpnz_nn)
{
    JUMP_DISTANCE_START();
    drvmappn(@@asm("jz $+0x17"));
    exit_vm(drc, &dri, MEM16(drip), dr_cycle_counter);
    ASSERT_JUMP_DISTANCE(0x17);
    drip += 2;
}

drop(jpz_nn)
{
    JUMP_DISTANCE_START();
    drvmappn(@@asm("jnz $+0x17"));
    exit_vm(drc, &dri, MEM16(drip), dr_cycle_counter);
    ASSERT_JUMP_DISTANCE(0x17);
    drip += 2;
}

drop(jpnc_nn)
{
    JUMP_DISTANCE_START();
    drvmappn(@@asm("jc $+0x17"));
    exit_vm(drc, &dri, MEM16(drip), dr_cycle_counter);
    ASSERT_JUMP_DISTANCE(0x17);
    drip += 2;
}

drop(jpc_nn)
{
    JUMP_DISTANCE_START();
    drvmappn(@@asm("jnc $+0x17"));
    exit_vm(drc, &dri, MEM16(drip), dr_cycle_counter);
    ASSERT_JUMP_DISTANCE(0x17);
    drip += 2;
}

drop(ret)
{
    exit_vm_by_ret(drc, &dri, dr_cycle_counter);
}

drop(reti)
{
    drvmappn(@@asm("mov byte [0x12345678],1", [0x78, 0x56, 0x34, 0x12], "(uintptr_t)&ime"));
    // Und dann ein normales RET
    exit_vm_by_ret(drc, &dri, dr_cycle_counter);
}

drop(call_nn)
{
    drvmappn((uint8_t[]){
                 @@asmr("lahf \n sub bp,2 \n sahf"),
                 @@asmr("mov word [ebp],0x1234", [0x34, 0x12], "drip + 2")
             });
    drip = MEM16(drip);
}

drop(callnz_nn)
{
    JUMP_DISTANCE_START();
    drvmappn((uint8_t[]){
                 @@asmr("jz $+0x23 \n lahf \n sub bp,2 \n sahf"),
                 @@asmr("mov word [ebp],0x1234", [0x34, 0x12], "drip + 2")
             });
    exit_vm(drc, &dri, MEM16(drip), dr_cycle_counter);
    ASSERT_JUMP_DISTANCE(0x23);
    drip += 2;
}

drop(callz_nn)
{
    JUMP_DISTANCE_START();
    drvmappn((uint8_t[]){
                 @@asmr("jnz $+0x23 \n lahf \n sub bp,2 \n sahf"),
                 @@asmr("mov word [ebp],0x1234", [0x34, 0x12], "drip + 2")
             });
    exit_vm(drc, &dri, MEM16(drip), dr_cycle_counter);
    ASSERT_JUMP_DISTANCE(0x23);
    drip += 2;
}

drop(callnc_nn)
{
    JUMP_DISTANCE_START();
    drvmappn((uint8_t[]){
                 @@asmr("jc $+0x23 \n lahf \n sub bp,2 \n sahf"),
                 @@asmr("mov word [ebp],0x1234", [0x34, 0x12], "drip + 2")
             });
    exit_vm(drc, &dri, MEM16(drip), dr_cycle_counter);
    ASSERT_JUMP_DISTANCE(0x23);
    drip += 2;
}

drop(callc_nn)
{
    JUMP_DISTANCE_START();
    drvmappn((uint8_t[]){
                 @@asmr("jnc $+0x23 \n lahf \n sub bp,2 \n sahf"),
                 @@asmr("mov word [ebp],0x1234", [0x34, 0x12], "drip + 2")
             });
    exit_vm(drc, &dri, MEM16(drip), dr_cycle_counter);
    ASSERT_JUMP_DISTANCE(0x23);
    drip += 2;
}

#define def_drop_rst(num) \
    drop(rst_##num) \
    { \
        drvmappn((uint8_t[]){ \
                     @@asmr("lahf \n sub bp,2 \n sahf"), \
                     @@asmr("mov word [ebp],0x1234", [0x34, 0x12], "drip") \
                 }); \
        drip = num; \
    }

def_drop_rst(0x00)
def_drop_rst(0x08)
def_drop_rst(0x10)
def_drop_rst(0x18)
def_drop_rst(0x20)
def_drop_rst(0x28)
def_drop_rst(0x30)
def_drop_rst(0x38)

drop(leave_vm_imm_op0)
{
    exit_vm(drc, &dri, drip - 1, dr_cycle_counter);
}

drop(leave_vm_imm_op1)
{
    exit_vm(drc, &dri, drip++ - 1, dr_cycle_counter);
}

drop(ld__nn_a)
{
    drvmappn(@@asm("mov [0x12345678],al", [0x78, 0x56, 0x34, 0x12], "MEM_BASE + MEM16(drip)"));
    drip += 2;
}

drop(pop_af)
{
    drvmappn((uint8_t[]){
                 @@asmr("movzx eax,byte [ebp]"),
                 @@asmr("mov ah,[0x12345678+eax]", [0x78, 0x56, 0x34, 0x12], "(uintptr_t)&reverse_flag_table"),
                 @@asmr("mov al,[ebp+1] \n add bp,2"),
                 @@asmr("or eax,0x12345678", [0x78, 0x56, 0x34, 0x12], "MEM_BASE"),
                 @@asmr("sahf")
             });
}

drop(push_af)
{
    // Das ist ganz schön fies, weil wir AF einfach umgekehrt verwendet und
    // zudem ein ganz anderes F-Register haben...
    drvmappn((uint8_t[]){
                 @@asmr("push eax \n lahf \n push eax \n movzx eax,ah"),
                 @@asmr("mov al,[0x12345678+eax]", [0x78, 0x56, 0x34, 0x12], "(uintptr_t)&flag_table"),
                 @@asmr("mov ah,[esp] \n sub bp,2 \n mov [ebp],ax \n pop eax \n sahf \n pop eax")
             });
}

drop(di)
{
    drvmappn(@@asm("mov byte [0x12345678],0", [0x78, 0x56, 0x34, 0x12], "(uintptr_t)&ime"));
}

drop(ei)
{
    drvmappn(@@asm("mov byte [0x12345678],1", [0x78, 0x56, 0x34, 0x12], "(uintptr_t)&ime"));
}

drop(jp_hl)
{
    drvmappn((uint8_t[]){
                 @@asmr("mov [0x12345678],dx", [0x78, 0x56, 0x34, 0x12], "(uintptr_t)&vm_ip"),
                 @@asmr("mov byte [0x12345678],0xff", [0x78, 0x56, 0x34, 0x12], "(uintptr_t)&cycles_gone", [0xff], "dr_cycle_counter")
             });
    size_t here = dri;
    drvmappn(@@asm("jmp dword 0x12345678", [0x78 - 5, 0x56, 0x34, 0x12], "(uint32_t)(CODE_EXITI - (here + 5))"));
}

drop(ld__nn_sp)
{
    drvmappn(@@asm("mov [0x12345678],bp", [0x78, 0x56, 0x34, 0x12], "MEM_BASE + MEM16(drip)"));
    drip += 2;
}

drop(ld_a__nn)
{
    drvmappn(@@asm("mov al,[0x12345678]", [0x78, 0x56, 0x34, 0x12], "MEM_BASE + MEM16(drip)"));
    drip += 2;
}

#ifdef UNSAVE_RAM_MAPPING
drop(ld_a__ffn)
{
    drvmappn(@@asm("mov al,[0x12345678]", [0x78, 0x56, 0x34, 0x12], "MEM_BASE + 0xff00 + MEM8(drip)"));
    drip++;
}

drop(ld_a__ffc)
{
    JUMP_DISTANCE_START();
    uintptr_t target = &drc[dri + 7];
    drvmappn((uint8_t[]){
                 @@asmr("mov [0x12345678],bl", [0x78, 0x56, 0x34, 0x12], "target"),
                 @@asmr("mov al,[0x12345678]", [0x78, 0x56, 0x34, 0x12], "MEM_BASE + 0xFF00")
             });
    ASSERT_JUMP_DISTANCE(0xb);
}
#endif

static void (*const dynarec_table[256])(void) = {
    dran(0x08, ld__nn_sp),
    dran(0x18, jr_n),
    dran(0x20, jrnz_n),
    dran(0x28, jrz_n),
    dran(0x30, jrnc_n),
    dran(0x38, jrc_n),
    dran(0x76, leave_vm_imm_op0), // halt
    dran(0xC0, retnz),
    dran(0xC2, jpnz_nn),
    dran(0xC3, jp_nn),
    dran(0xC4, callnz_nn),
    dran(0xC7, rst_0x00),
    dran(0xC8, retz),
    dran(0xC9, ret),
    dran(0xCA, jpz_nn),
    dran(0xCC, callz_nn),
    dran(0xCD, call_nn),
    dran(0xCF, rst_0x08),
    dran(0xD0, retnc),
    dran(0xD2, jpnc_nn),
    dran(0xD4, callnc_nn),
    dran(0xD7, rst_0x10),
    dran(0xD8, retc),
    dran(0xD9, reti),
    dran(0xDA, jpc_nn),
    dran(0xDC, callc_nn),
    dran(0xDF, rst_0x18),
    dran(0xE0, leave_vm_imm_op1), // ld__ffn_a
    dran(0xE2, leave_vm_imm_op0), // ld__ffc_a
    dran(0xE7, rst_0x20),
    dran(0xE9, jp_hl),
    dran(0xEA, ld__nn_a),
    dran(0xEF, rst_0x28),
#ifndef UNSAVE_RAM_MAPPING
    dran(0xF0, leave_vm_imm_op1), // ld_a__ffn
#else
    dran(0xF0, ld_a__ffn),
#endif
    dran(0xF1, pop_af),
#ifndef UNSAVE_RAM_MAPPING
    dran(0xF2, leave_vm_imm_op0), // ld_a__ffc
#else
    dran(0xF2, ld_a__ffc),
#endif
    dran(0xF3, di),
    dran(0xF5, push_af),
    dran(0xF7, rst_0x30),
    dran(0xFA, ld_a__nn),
    dran(0xFB, ei),
    dran(0xFF, rst_0x38)
};

struct DynarecConstInsn {
    const uint8_t data[28];
    uint8_t length;
    uint8_t load_offset;
    uint8_t load_length;

    uint8_t _padding;
};

#define drci(...) \
    { \
        .data = __VA_ARGS__, \
        .length = sizeof((uint8_t[])__VA_ARGS__), \
        \
        .load_offset = 0, \
        .load_length = 0, \
        \
        ._padding = 0, \
    }

#define drci_l(ll, lo, ...) \
    { \
        .data = __VA_ARGS__, \
        .length = sizeof((uint8_t[])__VA_ARGS__), \
        .load_offset = lo, \
        .load_length = ll, \
        \
        ._padding = 0, \
    }

static const struct DynarecConstInsn dynarec_const_insns[256] = {
    [0x00] = drci(@@asmi("nop")),                       // nop (special-casing this as a zero-length instruction is stupid)
    [0x01] = drci_l(sizeof(uint16_t), @@asmi_with_offset("mov bx,0x1234", [0x34, 0x12])), // ld bc,nn
    [0x02] = drci(@@asmi("mov [ebx],al")),              // ld (bc),a
    [0x03] = drci(@@asmi("lahf \n inc bx \n sahf")),    // inc bc
    [0x04] = drci(@@asmi("inc bh")),                    // inc b
    [0x05] = drci(@@asmi("dec bh")),                    // dec b
    [0x06] = drci_l(sizeof(uint8_t), @@asmi_with_offset("mov bh,0xff", [0xff])), // ld b,n
#ifdef UNSAVE_FLAG_OPTIMIZATIONS
    [0x07] = drci(@@asmi("rol al,1")),                  // rlca
#else
    [0x07] = drci(@@asmi("rol al,1 \n lahf \n and ah,0x2b ; CF \n sahf")), // rlca
#endif
    [0x09] = drci(@@asmi("lahf \n push eax \n mov al,ah \n and al,0x40 ; keep ZF \n add dx,bx \n lahf \n and ah,0xbf ; clear ZF \n or ah,al \n sahf \n pop eax")), // add hl,bc -- ZF must stay unchanged
    [0x0a] = drci(@@asmi("mov al,[ebx]")),              // ld a,(bc)
    [0x0b] = drci(@@asmi("lahf \n dec bx \n sahf")),    // dec bc
    [0x0c] = drci(@@asmi("inc bl")),                    // inc c
    [0x0d] = drci(@@asmi("dec bl")),                    // dec c
    [0x0e] = drci_l(sizeof(uint8_t), @@asmi_with_offset("mov bl,0xff", [0xff])), // ld c,n
#ifdef UNSAVE_FLAG_OPTIMIZATIONS
    [0x0f] = drci(@@asmi("ror al,1")),                  // rrca
#else
    [0x0f] = drci(@@asmi("ror al,1 \n lahf \n and ah,0x2b ; CF \n sahf")), // rrca
#endif
    [0x11] = drci_l(sizeof(uint16_t), @@asmi_with_offset("mov cx,0x1234", [0x34, 0x12])), // ld de,nn
    [0x12] = drci(@@asmi("mov [ecx],al")),              // ld (de),a
    [0x13] = drci(@@asmi("lahf \n inc cx \n sahf")),    // inc de
    [0x14] = drci(@@asmi("inc ch")),                    // inc d
    [0x15] = drci(@@asmi("dec ch")),                    // dec d
    [0x16] = drci_l(sizeof(uint8_t), @@asmi_with_offset("mov ch,0xff", [0xff])), // ld d,n
#ifdef UNSAVE_FLAG_OPTIMIZATIONS
    [0x17] = drci(@@asmi("rcl al,1")),                  // rla
#else
    [0x17] = drci(@@asmi("rcl al,1 \n lahf \n and ah,0x2b ; CF \n sahf")), // rla
#endif
    [0x19] = drci(@@asmi("lahf \n push eax \n mov al,ah \n and al,0x40 ; keep ZF \n add dx,cx \n lahf \n and ah,0xbf ; clear ZF \n or ah,al \n sahf \n pop eax")), // add hl,de -- ZF must stay unchanged
    [0x1a] = drci(@@asmi("mov al,[ecx]")),              // ld a,(de)
    [0x1b] = drci(@@asmi("lahf \n dec cx \n sahf")),    // dec de
    [0x1c] = drci(@@asmi("inc cl")),                    // inc e
    [0x1d] = drci(@@asmi("dec cl")),                    // dec e
    [0x1e] = drci_l(sizeof(uint8_t), @@asmi_with_offset("mov cl,0xff", [0xff])), // ld e,n
#ifdef UNSAVE_FLAG_OPTIMIZATIONS
    [0x1f] = drci(@@asmi("rcr al,1")),                  // rra
#else
    [0x1f] = drci(@@asmi("rcr al,1 \n lahf \n and ah,0x2b ; CF \n sahf")), // rra
#endif
    [0x21] = drci_l(sizeof(uint16_t), @@asmi_with_offset("mov dx,0x1234", [0x34, 0x12])), // ld hl,nn
    [0x22] = drci(@@asmi("mov [edx],al \n lahf \n inc dx \n sahf")), // ldi (hl),a
    [0x23] = drci(@@asmi("lahf \n inc dx \n sahf")),    // inc hl
    [0x24] = drci(@@asmi("inc dh")),                    // inc h
    [0x25] = drci(@@asmi("dec dh")),                    // dec h
    [0x26] = drci_l(sizeof(uint8_t), @@asmi_with_offset("mov dh,0xff", [0xff])), // ld h,n
    [0x27] = drci(@@asmi("daa")),                       // roflol
    [0x29] = drci(@@asmi("lahf \n push eax \n mov al,ah \n and al,0x40 ; keep ZF \n add dx,dx \n lahf \n and ah,0xbf ; clear ZF \n or ah,al \n sahf \n pop eax")), // add hl,hl -- ZF must stay unchanged
    [0x2a] = drci(@@asmi("mov al,[edx] \n lahf \n inc dx \n sahf")), // ldi a,(hl)
    [0x2b] = drci(@@asmi("lahf \n dec dx \n sahf")),    // dec hl
    [0x2c] = drci(@@asmi("inc dl")),                    // inc l
    [0x2d] = drci(@@asmi("dec dl")),                    // dec l
    [0x2e] = drci_l(sizeof(uint8_t), @@asmi_with_offset("mov dl,0xff", [0xff])), // ld l,n
#ifdef UNSAVE_FLAG_OPTIMIZATIONS
    [0x2f] = drci(@@asmi("not al")),                    // cpl a
#else
    [0x2f] = drci(@@asmi("not al \n lahf \n or ah,0x10 ; AF \n sahf")), // cpl a
#endif
    [0x31] = drci_l(sizeof(uint16_t), @@asmi_with_offset("mov bp,0x1234", [0x34, 0x12])), // ld sp,nn
    [0x32] = drci(@@asmi("mov [edx],al \n lahf \n dec dx \n sahf")), // ldd (hl),a
    [0x33] = drci(@@asmi("lahf \n inc bp \n sahf")),    // inc sp
    [0x34] = drci(@@asmi("inc byte [edx]")),            // inc (hl)
    [0x35] = drci(@@asmi("dec byte [edx]")),            // dec (hl)
    [0x36] = drci_l(sizeof(uint8_t), @@asmi_with_offset("mov byte [edx],0xff", [0xff])), // ld (hl),n
#ifdef UNSAVE_FLAG_OPTIMIZATIONS
    [0x37] = drci(@@asmi("stc")),                       // scf (note: this was stc \n nop before, no idea why)
#else
    [0x37] = drci(@@asmi("lahf \n and ah,0x6a ; ZF \n or ah,0x01 ; CF \n sahf")), // scf
#endif
    [0x39] = drci(@@asmi("lahf \n push eax \n mov al,ah \n and al,0x40 ; keep ZF \n add dx,bp \n lahf \n and ah,0xbf ; clear ZF \n or ah,al \n sahf \n pop eax")), // add hl,sp -- ZF must stay unchanged
    [0x3a] = drci(@@asmi("mov al,[edx] \n lahf \n dec dx \n sahf")), // ldd a,(hl)
    [0x3b] = drci(@@asmi("lahf \n dec bp \n sahf")),    // dec sp
    [0x3c] = drci(@@asmi("inc al")),                    // inc a
    [0x3d] = drci(@@asmi("dec al")),                    // dec a
    [0x3e] = drci_l(sizeof(uint8_t), @@asmi_with_offset("mov al,0xff", [0xff])), // ld a,n
#ifdef UNSAVE_FLAG_OPTIMIZATIONS
    [0x3f] = drci(@@asmi("cmc")),                       // ccf (note: this was cmc \n nop before, no idea why)
#else
    [0x3f] = drci(@@asmi("lahf \n and ah,0x6a ; ZF \n xor ah,0x01 ; CF \n sahf")), // ccf
#endif
    [0x40] = drci(@@asmi("mov bh,bh")),                 // ld b,b
    [0x41] = drci(@@asmi("mov bh,bl")),                 // ld b,c
    [0x42] = drci(@@asmi("mov bh,ch")),                 // ld b,d
    [0x43] = drci(@@asmi("mov bh,cl")),                 // ld b,e
    [0x44] = drci(@@asmi("mov bh,dh")),                 // ld b,h
    [0x45] = drci(@@asmi("mov bh,dl")),                 // ld b,l
    [0x46] = drci(@@asmi("mov bh,[edx]")),              // ld b,(hl)
    [0x47] = drci(@@asmi("mov bh,al")),                 // ld b,a
    [0x48] = drci(@@asmi("mov bl,bh")),                 // ld c,b
    [0x49] = drci(@@asmi("mov bl,bl")),                 // ld c,c
    [0x4a] = drci(@@asmi("mov bl,ch")),                 // ld c,d
    [0x4b] = drci(@@asmi("mov bl,cl")),                 // ld c,e
    [0x4c] = drci(@@asmi("mov bl,dh")),                 // ld c,h
    [0x4d] = drci(@@asmi("mov bl,dl")),                 // ld c,l
    [0x4e] = drci(@@asmi("mov bl,[edx]")),              // ld c,(hl)
    [0x4f] = drci(@@asmi("mov bl,al")),                 // ld c,a
    [0x50] = drci(@@asmi("mov ch,bh")),                 // ld d,b
    [0x51] = drci(@@asmi("mov ch,bl")),                 // ld d,c
    [0x52] = drci(@@asmi("mov ch,ch")),                 // ld d,d
    [0x53] = drci(@@asmi("mov ch,cl")),                 // ld d,e
    [0x54] = drci(@@asmi("mov ch,dh")),                 // ld d,h
    [0x55] = drci(@@asmi("mov ch,dl")),                 // ld d,l
    [0x56] = drci(@@asmi("mov ch,[edx]")),              // ld d,(hl)
    [0x57] = drci(@@asmi("mov ch,al")),                 // ld d,a
    [0x58] = drci(@@asmi("mov cl,bh")),                 // ld e,b
    [0x59] = drci(@@asmi("mov cl,bl")),                 // ld e,c
    [0x5a] = drci(@@asmi("mov cl,ch")),                 // ld e,d
    [0x5b] = drci(@@asmi("mov cl,cl")),                 // ld e,e
    [0x5c] = drci(@@asmi("mov cl,dh")),                 // ld e,h
    [0x5d] = drci(@@asmi("mov cl,dl")),                 // ld e,l
    [0x5e] = drci(@@asmi("mov cl,[edx]")),              // ld e,(hl)
    [0x5f] = drci(@@asmi("mov cl,al")),                 // ld e,a
    [0x60] = drci(@@asmi("mov dh,bh")),                 // ld h,b
    [0x61] = drci(@@asmi("mov dh,bl")),                 // ld h,c
    [0x62] = drci(@@asmi("mov dh,ch")),                 // ld h,d
    [0x63] = drci(@@asmi("mov dh,cl")),                 // ld h,e
    [0x64] = drci(@@asmi("mov dh,dh")),                 // ld h,h
    [0x65] = drci(@@asmi("mov dh,dl")),                 // ld h,l
    [0x66] = drci(@@asmi("mov dh,[edx]")),              // ld h,(hl)
    [0x67] = drci(@@asmi("mov dh,al")),                 // ld h,a
    [0x68] = drci(@@asmi("mov dl,bh")),                 // ld l,b
    [0x69] = drci(@@asmi("mov dl,bl")),                 // ld l,c
    [0x6a] = drci(@@asmi("mov dl,ch")),                 // ld l,d
    [0x6b] = drci(@@asmi("mov dl,cl")),                 // ld l,e
    [0x6c] = drci(@@asmi("mov dl,dh")),                 // ld l,h
    [0x6d] = drci(@@asmi("mov dl,dl")),                 // ld l,l
    [0x6e] = drci(@@asmi("mov dl,[edx]")),              // ld l,(hl)
    [0x6f] = drci(@@asmi("mov dl,al")),                 // ld l,a
    [0x70] = drci(@@asmi("mov [edx],bh")),              // ld (hl),b
    [0x71] = drci(@@asmi("mov [edx],bl")),              // ld (hl),c
    [0x72] = drci(@@asmi("mov [edx],ch")),              // ld (hl),d
    [0x73] = drci(@@asmi("mov [edx],cl")),              // ld (hl),e
    [0x74] = drci(@@asmi("mov [edx],dh")),              // ld (hl),h
    [0x75] = drci(@@asmi("mov [edx],dl")),              // ld (hl),l
    [0x77] = drci(@@asmi("mov [edx],al")),              // ld (hl),a
    [0x78] = drci(@@asmi("mov al,bh")),                 // ld a,b
    [0x79] = drci(@@asmi("mov al,bl")),                 // ld a,c
    [0x7a] = drci(@@asmi("mov al,ch")),                 // ld a,d
    [0x7b] = drci(@@asmi("mov al,cl")),                 // ld a,e
    [0x7c] = drci(@@asmi("mov al,dh")),                 // ld a,h
    [0x7d] = drci(@@asmi("mov al,dl")),                 // ld a,l
    [0x7e] = drci(@@asmi("mov al,[edx]")),              // ld a,(hl)
    [0x7f] = drci(@@asmi("mov al,al")),                 // ld a,a
    [0x80] = drci(@@asmi("add al,bh")),                 // add a,b
    [0x81] = drci(@@asmi("add al,bl")),                 // add a,c
    [0x82] = drci(@@asmi("add al,ch")),                 // add a,d
    [0x83] = drci(@@asmi("add al,cl")),                 // add a,e
    [0x84] = drci(@@asmi("add al,dh")),                 // add a,h
    [0x85] = drci(@@asmi("add al,dl")),                 // add a,l
    [0x86] = drci(@@asmi("add al,[edx]")),              // add a,(hl)
    [0x87] = drci(@@asmi("add al,al")),                 // add a,a
    [0x88] = drci(@@asmi("adc al,bh")),                 // adc a,b
    [0x89] = drci(@@asmi("adc al,bl")),                 // adc a,c
    [0x8a] = drci(@@asmi("adc al,ch")),                 // adc a,d
    [0x8b] = drci(@@asmi("adc al,cl")),                 // adc a,e
    [0x8c] = drci(@@asmi("adc al,dh")),                 // adc a,h
    [0x8d] = drci(@@asmi("adc al,dl")),                 // adc a,l
    [0x8e] = drci(@@asmi("adc al,[edx]")),              // adc a,(hl)
    [0x8f] = drci(@@asmi("adc al,al")),                 // adc a,a
    [0x90] = drci(@@asmi("sub al,bh")),                 // sub a,b
    [0x91] = drci(@@asmi("sub al,bl")),                 // sub a,c
    [0x92] = drci(@@asmi("sub al,ch")),                 // sub a,d
    [0x93] = drci(@@asmi("sub al,cl")),                 // sub a,e
    [0x94] = drci(@@asmi("sub al,dh")),                 // sub a,h
    [0x95] = drci(@@asmi("sub al,dl")),                 // sub a,l
    [0x96] = drci(@@asmi("sub al,[edx]")),              // sub a,(hl)
    [0x97] = drci(@@asmi("sub al,al")),                 // sub a,a
    [0x98] = drci(@@asmi("sbb al,bh")),                 // sbc a,b
    [0x99] = drci(@@asmi("sbb al,bl")),                 // sbc a,c
    [0x9a] = drci(@@asmi("sbb al,ch")),                 // sbc a,d
    [0x9b] = drci(@@asmi("sbb al,cl")),                 // sbc a,e
    [0x9c] = drci(@@asmi("sbb al,dh")),                 // sbc a,h
    [0x9d] = drci(@@asmi("sbb al,dl")),                 // sbc a,l
    [0x9e] = drci(@@asmi("sbb al,[edx]")),              // sbc a,(hl)
    [0x9f] = drci(@@asmi("sbb al,al")),                 // sbc a,a
#ifdef UNSAVE_FLAG_OPTIMIZATIONS
    [0xa0] = drci(@@asmi("and al,bh")),                 // and a,b
    [0xa1] = drci(@@asmi("and al,bl")),                 // and a,c
    [0xa2] = drci(@@asmi("and al,ch")),                 // and a,d
    [0xa3] = drci(@@asmi("and al,cl")),                 // and a,e
    [0xa4] = drci(@@asmi("and al,dh")),                 // and a,h
    [0xa5] = drci(@@asmi("and al,dl")),                 // and a,l
    [0xa6] = drci(@@asmi("and al,[edx]")),              // and a,(hl)
    [0xa7] = drci(@@asmi("and al,al")),                 // and a,a
    [0xa8] = drci(@@asmi("xor al,bh")),                 // xor a,b
    [0xa9] = drci(@@asmi("xor al,bl")),                 // xor a,c
    [0xaa] = drci(@@asmi("xor al,ch")),                 // xor a,d
    [0xab] = drci(@@asmi("xor al,cl")),                 // xor a,e
    [0xac] = drci(@@asmi("xor al,dh")),                 // xor a,h
    [0xad] = drci(@@asmi("xor al,dl")),                 // xor a,l
    [0xae] = drci(@@asmi("xor al,[edx]")),              // xor a,(hl)
    [0xaf] = drci(@@asmi("xor al,al")),                 // xor a,a
    [0xb0] = drci(@@asmi("or al,bh")),                  // or a,b
    [0xb1] = drci(@@asmi("or al,bl")),                  // or a,c
    [0xb2] = drci(@@asmi("or al,ch")),                  // or a,d
    [0xb3] = drci(@@asmi("or al,cl")),                  // or a,e
    [0xb4] = drci(@@asmi("or al,dh")),                  // or a,h
    [0xb5] = drci(@@asmi("or al,dl")),                  // or a,l
    [0xb6] = drci(@@asmi("or al,[edx]")),               // or a,(hl)
    [0xb7] = drci(@@asmi("or al,al")),                  // or a,a
#else
    [0xa0] = drci(@@asmi("and al,bh \n lahf \n and ah,0x6a ; ZF \n or ah,0x10 ; AF \n sahf")), // and a,b
    [0xa1] = drci(@@asmi("and al,bl \n lahf \n and ah,0x6a ; ZF \n or ah,0x10 ; AF \n sahf")), // and a,c
    [0xa2] = drci(@@asmi("and al,ch \n lahf \n and ah,0x6a ; ZF \n or ah,0x10 ; AF \n sahf")), // and a,d
    [0xa3] = drci(@@asmi("and al,cl \n lahf \n and ah,0x6a ; ZF \n or ah,0x10 ; AF \n sahf")), // and a,e
    [0xa4] = drci(@@asmi("and al,dh \n lahf \n and ah,0x6a ; ZF \n or ah,0x10 ; AF \n sahf")), // and a,h
    [0xa5] = drci(@@asmi("and al,dl \n lahf \n and ah,0x6a ; ZF \n or ah,0x10 ; AF \n sahf")), // and a,l
    [0xa6] = drci(@@asmi("and al,[edx] \n lahf \n and ah,0x6a ; ZF \n or ah,0x10 ; AF \n sahf")), // and a,(hl)
    [0xa7] = drci(@@asmi("and al,al \n lahf \n and ah,0x6a ; ZF \n or ah,0x10 ; AF \n sahf")), // and a,a
    [0xa8] = drci(@@asmi("xor al,bh \n lahf \n and ah,0x6a ; ZF \n or ah,0x10 ; AF \n sahf")), // xor a,b
    [0xa9] = drci(@@asmi("xor al,bl \n lahf \n and ah,0x6a ; ZF \n or ah,0x10 ; AF \n sahf")), // xor a,c
    [0xaa] = drci(@@asmi("xor al,ch \n lahf \n and ah,0x6a ; ZF \n or ah,0x10 ; AF \n sahf")), // xor a,d
    [0xab] = drci(@@asmi("xor al,cl \n lahf \n and ah,0x6a ; ZF \n or ah,0x10 ; AF \n sahf")), // xor a,e
    [0xac] = drci(@@asmi("xor al,dh \n lahf \n and ah,0x6a ; ZF \n or ah,0x10 ; AF \n sahf")), // xor a,h
    [0xad] = drci(@@asmi("xor al,dl \n lahf \n and ah,0x6a ; ZF \n or ah,0x10 ; AF \n sahf")), // xor a,l
    [0xae] = drci(@@asmi("xor al,[edx] \n lahf \n and ah,0x6a ; ZF \n or ah,0x10 ; AF \n sahf")), // xor a,(hl)
    [0xaf] = drci(@@asmi("xor al,al \n lahf \n and ah,0x6a ; ZF \n or ah,0x10 ; AF \n sahf")), // xor a,a
    [0xb0] = drci(@@asmi("or al,bh \n lahf \n and ah,0x6a ; ZF \n or ah,0x10 ; AF \n sahf")), // or a,b
    [0xb1] = drci(@@asmi("or al,bl \n lahf \n and ah,0x6a ; ZF \n or ah,0x10 ; AF \n sahf")), // or a,c
    [0xb2] = drci(@@asmi("or al,ch \n lahf \n and ah,0x6a ; ZF \n or ah,0x10 ; AF \n sahf")), // or a,d
    [0xb3] = drci(@@asmi("or al,cl \n lahf \n and ah,0x6a ; ZF \n or ah,0x10 ; AF \n sahf")), // or a,e
    [0xb4] = drci(@@asmi("or al,dh \n lahf \n and ah,0x6a ; ZF \n or ah,0x10 ; AF \n sahf")), // or a,h
    [0xb5] = drci(@@asmi("or al,dl \n lahf \n and ah,0x6a ; ZF \n or ah,0x10 ; AF \n sahf")), // or a,l
    [0xb6] = drci(@@asmi("or al,[edx] \n lahf \n and ah,0x6a ; ZF \n or ah,0x10 ; AF \n sahf")), // or a,(hl)
    [0xb7] = drci(@@asmi("or al,al \n lahf \n and ah,0x6a ; ZF \n or ah,0x10 ; AF \n sahf")), // or a,a
#endif
    [0xb8] = drci(@@asmi("cmp al,bh")),                 // cp a,b
    [0xb9] = drci(@@asmi("cmp al,bl")),                 // cp a,c
    [0xba] = drci(@@asmi("cmp al,ch")),                 // cp a,d
    [0xbb] = drci(@@asmi("cmp al,cl")),                 // cp a,e
    [0xbc] = drci(@@asmi("cmp al,dh")),                 // cp a,h
    [0xbd] = drci(@@asmi("cmp al,dl")),                 // cp a,l
    [0xbe] = drci(@@asmi("cmp al,[edx]")),              // cp a,(hl)
    [0xbf] = drci(@@asmi("cmp al,al")),                 // cp a,a
    [0xc1] = drci(@@asmi("mov bx,[ebp] \n lahf \n add bp,2 \n sahf")), // pop bc
    [0xc5] = drci(@@asmi("lahf \n sub bp,2 \n sahf \n mov [ebp],bx")), // push bc
    [0xc6] = drci_l(sizeof(uint8_t), @@asmi_with_offset("add al,0xff", [0xff])), // add al,n
    [0xce] = drci_l(sizeof(uint8_t), @@asmi_with_offset("adc al,0xff", [0xff])), // adc al,n
    [0xd1] = drci(@@asmi("mov cx,[ebp] \n lahf \n add bp,2 \n sahf")), // pop de
    [0xd5] = drci(@@asmi("lahf \n sub bp,2 \n sahf \n mov [ebp],cx")), // push de
    [0xd6] = drci_l(sizeof(uint8_t), @@asmi_with_offset("sub al,0xff", [0xff])), // sub al,n
    [0xde] = drci_l(sizeof(uint8_t), @@asmi_with_offset("sbb al,0xff", [0xff])), // sbc al,n
    [0xe1] = drci(@@asmi("mov dx,[ebp] \n lahf \n add bp,2 \n sahf")), // pop hl
    [0xe5] = drci(@@asmi("lahf \n sub bp,2 \n sahf \n mov [ebp],dx")), // push hl
#ifdef UNSAVE_FLAG_OPTIMIZATIONS
    [0xe6] = drci_l(sizeof(uint8_t), @@asmi_with_offset("and al,0xff", [0xff])), // and al,n
    [0xee] = drci_l(sizeof(uint8_t), @@asmi_with_offset("xor al,0xff", [0xff])), // xor al,n
    [0xf6] = drci_l(sizeof(uint8_t), @@asmi_with_offset("or al,0xff", [0xff])),  // or al,n
    [0xf8] = drci_l(sizeof(uint8_t), @@asmi_with_offset("mov dx,bp \n add dx,-1", [0xff])), // ld hl,sp+n (sign-extended)
#else
    [0xe6] = drci_l(sizeof(uint8_t), @@asmi_with_offset("and al,0xff \n lahf \n and ah,0x6a ; ZF \n or ah,0x10 ; AF \n sahf", [0xff])), // and al,n
    [0xee] = drci_l(sizeof(uint8_t), @@asmi_with_offset("xor al,0xff \n lahf \n and ah,0x6a ; ZF \n or ah,0x10 ; AF \n sahf", [0xff])), // xor al,n
    [0xf6] = drci_l(sizeof(uint8_t), @@asmi_with_offset("or al,0xff \n lahf \n and ah,0x6a ; ZF \n or ah,0x10 ; AF \n sahf", [0xff])),  // or al,n
    [0xf8] = drci_l(sizeof(uint8_t), @@asmi_with_offset("mov dx,bp \n add dx,-1 \n lahf \n and ah,0x3b ; keep AF and CF \n sahf", [0xff])), // ld hl,sp+n (sign-extended)
#endif
    [0xf9] = drci(@@asmi("mov ebp,edx")),               // ld sp,hl
    [0xfe] = drci_l(sizeof(uint8_t), @@asmi_with_offset("cmp al,0xff", [0xff])), // cp al,n
};

static const struct DynarecConstInsn dynarec_const_cb_insns[256] = {
#ifdef UNSAVE_FLAG_OPTIMIZATIONS
    [0x00] = drci(@@asmi("rol bh,1")),           // rlc b
    [0x01] = drci(@@asmi("rol bl,1")),           // rlc c
    [0x02] = drci(@@asmi("rol ch,1")),           // rlc d
    [0x03] = drci(@@asmi("rol cl,1")),           // rlc e
    [0x04] = drci(@@asmi("rol dh,1")),           // rlc h
    [0x05] = drci(@@asmi("rol dl,1")),           // rlc l
    [0x06] = drci(@@asmi("rol byte [edx],1")),   // rlc (hl)
    [0x07] = drci(@@asmi("rol al,1")),           // rlc a
    [0x08] = drci(@@asmi("ror bh,1")),           // rrc b
    [0x09] = drci(@@asmi("ror bl,1")),           // rrc c
    [0x0a] = drci(@@asmi("ror ch,1")),           // rrc d
    [0x0b] = drci(@@asmi("ror cl,1")),           // rrc e
    [0x0c] = drci(@@asmi("ror dh,1")),           // rrc h
    [0x0d] = drci(@@asmi("ror dl,1")),           // rrc l
    [0x0e] = drci(@@asmi("ror byte [edx],1")),   // rrc (hl)
    [0x0f] = drci(@@asmi("ror al,1")),           // rrc a
    [0x10] = drci(@@asmi("rcl bh,1")),           // rl b
    [0x11] = drci(@@asmi("rcl bl,1")),           // rl c
    [0x12] = drci(@@asmi("rcl ch,1")),           // rl d
    [0x13] = drci(@@asmi("rcl cl,1")),           // rl e
    [0x14] = drci(@@asmi("rcl dh,1")),           // rl h
    [0x15] = drci(@@asmi("rcl dl,1")),           // rl l
    [0x16] = drci(@@asmi("rcl byte [edx],1")),   // rl (hl)
    [0x17] = drci(@@asmi("rcl al,1")),           // rl a
    [0x18] = drci(@@asmi("rcr bh,1")),           // rr b
    [0x19] = drci(@@asmi("rcr bl,1")),           // rr c
    [0x1a] = drci(@@asmi("rcr ch,1")),           // rr d
    [0x1b] = drci(@@asmi("rcr cl,1")),           // rr e
    [0x1c] = drci(@@asmi("rcr dh,1")),           // rr h
    [0x1d] = drci(@@asmi("rcr dl,1")),           // rr l
    [0x1e] = drci(@@asmi("rcr byte [edx],1")),   // rr (hl)
    [0x1f] = drci(@@asmi("rcr al,1")),           // rr a
    [0x20] = drci(@@asmi("sal bh,1")),           // sla b
    [0x21] = drci(@@asmi("sal bl,1")),           // sla c
    [0x22] = drci(@@asmi("sal ch,1")),           // sla d
    [0x23] = drci(@@asmi("sal cl,1")),           // sla e
    [0x24] = drci(@@asmi("sal dh,1")),           // sla h
    [0x25] = drci(@@asmi("sal dl,1")),           // sla l
    [0x26] = drci(@@asmi("sal byte [edx],1")),   // sla (hl)
    [0x27] = drci(@@asmi("sal al,1")),           // sla a
    [0x28] = drci(@@asmi("sar bh,1")),           // sra b
    [0x29] = drci(@@asmi("sar bl,1")),           // sra c
    [0x2a] = drci(@@asmi("sar ch,1")),           // sra d
    [0x2b] = drci(@@asmi("sar cl,1")),           // sra e
    [0x2c] = drci(@@asmi("sar dh,1")),           // sra h
    [0x2d] = drci(@@asmi("sar dl,1")),           // sra l
    [0x2e] = drci(@@asmi("sar byte [edx],1")),   // sra (hl)
    [0x2f] = drci(@@asmi("sar al,1")),           // sra a
#else
    [0x00] = drci(@@asmi("rol bh,1 \n push eax \n lahf \n mov al,ah \n and al,0x01 ; CF \n test bh,bh \n lahf \n and ah,0x6a ; ZF \n or ah,al \n sahf \n pop eax")), // rlc b
    [0x01] = drci(@@asmi("rol bl,1 \n push eax \n lahf \n mov al,ah \n and al,0x01 ; CF \n test bl,bl \n lahf \n and ah,0x6a ; ZF \n or ah,al \n sahf \n pop eax")), // rlc c
    [0x02] = drci(@@asmi("rol ch,1 \n push eax \n lahf \n mov al,ah \n and al,0x01 ; CF \n test ch,ch \n lahf \n and ah,0x6a ; ZF \n or ah,al \n sahf \n pop eax")), // rlc d
    [0x03] = drci(@@asmi("rol cl,1 \n push eax \n lahf \n mov al,ah \n and al,0x01 ; CF \n test cl,cl \n lahf \n and ah,0x6a ; ZF \n or ah,al \n sahf \n pop eax")), // rlc e
    [0x04] = drci(@@asmi("rol dh,1 \n push eax \n lahf \n mov al,ah \n and al,0x01 ; CF \n test dh,dh \n lahf \n and ah,0x6a ; ZF \n or ah,al \n sahf \n pop eax")), // rlc h
    [0x05] = drci(@@asmi("rol dl,1 \n push eax \n lahf \n mov al,ah \n and al,0x01 ; CF \n test dl,dl \n lahf \n and ah,0x6a ; ZF \n or ah,al \n sahf \n pop eax")), // rlc l
    [0x06] = drci(@@asmi("rol byte [edx],1 \n push eax \n lahf \n mov al,ah \n and al,0x01 ; CF \n cmp byte [edx],0 \n lahf \n and ah,0x6a ; ZF \n or ah,al \n sahf \n pop eax")), // rlc (hl)
    [0x07] = drci(@@asmi("rol al,1 \n push eax \n lahf \n mov al,ah \n and al,0x01 ; CF \n test al,al \n lahf \n and ah,0x6a ; ZF \n or ah,al \n sahf \n pop eax")), // rlc a
    [0x08] = drci(@@asmi("ror bh,1 \n push eax \n lahf \n mov al,ah \n and al,0x01 ; CF \n test bh,bh \n lahf \n and ah,0x6a ; ZF \n or ah,al \n sahf \n pop eax")), // rrc b
    [0x09] = drci(@@asmi("ror bl,1 \n push eax \n lahf \n mov al,ah \n and al,0x01 ; CF \n test bl,bl \n lahf \n and ah,0x6a ; ZF \n or ah,al \n sahf \n pop eax")), // rrc c
    [0x0a] = drci(@@asmi("ror ch,1 \n push eax \n lahf \n mov al,ah \n and al,0x01 ; CF \n test ch,ch \n lahf \n and ah,0x6a ; ZF \n or ah,al \n sahf \n pop eax")), // rrc d
    [0x0b] = drci(@@asmi("ror cl,1 \n push eax \n lahf \n mov al,ah \n and al,0x01 ; CF \n test cl,cl \n lahf \n and ah,0x6a ; ZF \n or ah,al \n sahf \n pop eax")), // rrc e
    [0x0c] = drci(@@asmi("ror dh,1 \n push eax \n lahf \n mov al,ah \n and al,0x01 ; CF \n test dh,dh \n lahf \n and ah,0x6a ; ZF \n or ah,al \n sahf \n pop eax")), // rrc h
    [0x0d] = drci(@@asmi("ror dl,1 \n push eax \n lahf \n mov al,ah \n and al,0x01 ; CF \n test dl,dl \n lahf \n and ah,0x6a ; ZF \n or ah,al \n sahf \n pop eax")), // rrc l
    [0x0e] = drci(@@asmi("ror byte [edx],1 \n push eax \n lahf \n mov al,ah \n and al,0x01 ; CF \n cmp byte [edx],0 \n lahf \n and ah,0x6a ; ZF \n or ah,al \n sahf \n pop eax")), // rrc (hl)
    [0x0f] = drci(@@asmi("ror al,1 \n push eax \n lahf \n mov al,ah \n and al,0x01 ; CF \n test al,al \n lahf \n and ah,0x6a ; ZF \n or ah,al \n sahf \n pop eax")), // rrc a
    [0x10] = drci(@@asmi("rcl bh,1 \n push eax \n lahf \n mov al,ah \n and al,0x01 ; CF \n test bh,bh \n lahf \n and ah,0x6a ; ZF \n or ah,al \n sahf \n pop eax")), // rl b
    [0x11] = drci(@@asmi("rcl bl,1 \n push eax \n lahf \n mov al,ah \n and al,0x01 ; CF \n test bl,bl \n lahf \n and ah,0x6a ; ZF \n or ah,al \n sahf \n pop eax")), // rl c
    [0x12] = drci(@@asmi("rcl ch,1 \n push eax \n lahf \n mov al,ah \n and al,0x01 ; CF \n test ch,ch \n lahf \n and ah,0x6a ; ZF \n or ah,al \n sahf \n pop eax")), // rl d
    [0x13] = drci(@@asmi("rcl cl,1 \n push eax \n lahf \n mov al,ah \n and al,0x01 ; CF \n test cl,cl \n lahf \n and ah,0x6a ; ZF \n or ah,al \n sahf \n pop eax")), // rl e
    [0x14] = drci(@@asmi("rcl dh,1 \n push eax \n lahf \n mov al,ah \n and al,0x01 ; CF \n test dh,dh \n lahf \n and ah,0x6a ; ZF \n or ah,al \n sahf \n pop eax")), // rl h
    [0x15] = drci(@@asmi("rcl dl,1 \n push eax \n lahf \n mov al,ah \n and al,0x01 ; CF \n test dl,dl \n lahf \n and ah,0x6a ; ZF \n or ah,al \n sahf \n pop eax")), // rl l
    [0x16] = drci(@@asmi("rcl byte [edx],1 \n push eax \n lahf \n mov al,ah \n and al,0x01 ; CF \n cmp byte [edx],0 \n lahf \n and ah,0x6a ; ZF \n or ah,al \n sahf \n pop eax")), // rl (hl)
    [0x17] = drci(@@asmi("rcl al,1 \n push eax \n lahf \n mov al,ah \n and al,0x01 ; CF \n test al,al \n lahf \n and ah,0x6a ; ZF \n or ah,al \n sahf \n pop eax")), // rl a
    [0x18] = drci(@@asmi("rcr bh,1 \n push eax \n lahf \n mov al,ah \n and al,0x01 ; CF \n test bh,bh \n lahf \n and ah,0x6a ; ZF \n or ah,al \n sahf \n pop eax")), // rr b
    [0x19] = drci(@@asmi("rcr bl,1 \n push eax \n lahf \n mov al,ah \n and al,0x01 ; CF \n test bl,bl \n lahf \n and ah,0x6a ; ZF \n or ah,al \n sahf \n pop eax")), // rr c
    [0x1a] = drci(@@asmi("rcr ch,1 \n push eax \n lahf \n mov al,ah \n and al,0x01 ; CF \n test ch,ch \n lahf \n and ah,0x6a ; ZF \n or ah,al \n sahf \n pop eax")), // rr d
    [0x1b] = drci(@@asmi("rcr cl,1 \n push eax \n lahf \n mov al,ah \n and al,0x01 ; CF \n test cl,cl \n lahf \n and ah,0x6a ; ZF \n or ah,al \n sahf \n pop eax")), // rr e
    [0x1c] = drci(@@asmi("rcr dh,1 \n push eax \n lahf \n mov al,ah \n and al,0x01 ; CF \n test dh,dh \n lahf \n and ah,0x6a ; ZF \n or ah,al \n sahf \n pop eax")), // rr h
    [0x1d] = drci(@@asmi("rcr dl,1 \n push eax \n lahf \n mov al,ah \n and al,0x01 ; CF \n test dl,dl \n lahf \n and ah,0x6a ; ZF \n or ah,al \n sahf \n pop eax")), // rr l
    [0x1e] = drci(@@asmi("rcr byte [edx],1 \n push eax \n lahf \n mov al,ah \n and al,0x01 ; CF \n cmp byte [edx],0 \n lahf \n and ah,0x6a ; ZF \n or ah,al \n sahf \n pop eax")), // rr (hl)
    [0x1f] = drci(@@asmi("rcr al,1 \n push eax \n lahf \n mov al,ah \n and al,0x01 ; CF \n test al,al \n lahf \n and ah,0x6a ; ZF \n or ah,al \n sahf \n pop eax")), // rr a
    [0x20] = drci(@@asmi("sal bh,1 \n lahf \n and ah,0xef ; clear AF \n sahf")), // sla b
    [0x21] = drci(@@asmi("sal bl,1 \n lahf \n and ah,0xef ; clear AF \n sahf")), // sla c
    [0x22] = drci(@@asmi("sal ch,1 \n lahf \n and ah,0xef ; clear AF \n sahf")), // sla d
    [0x23] = drci(@@asmi("sal cl,1 \n lahf \n and ah,0xef ; clear AF \n sahf")), // sla e
    [0x24] = drci(@@asmi("sal dh,1 \n lahf \n and ah,0xef ; clear AF \n sahf")), // sla h
    [0x25] = drci(@@asmi("sal dl,1 \n lahf \n and ah,0xef ; clear AF \n sahf")), // sla l
    [0x26] = drci(@@asmi("sal byte [edx],1 \n lahf \n and ah,0xef ; clear AF \n sahf")), // sla (hl)
    [0x27] = drci(@@asmi("sal al,1 \n lahf \n and ah,0xef ; clear AF \n sahf")), // sla a
    [0x28] = drci(@@asmi("sar bh,1 \n lahf \n and ah,0xef ; clear AF \n sahf")), // sra b
    [0x29] = drci(@@asmi("sar bl,1 \n lahf \n and ah,0xef ; clear AF \n sahf")), // sra c
    [0x2a] = drci(@@asmi("sar ch,1 \n lahf \n and ah,0xef ; clear AF \n sahf")), // sra d
    [0x2b] = drci(@@asmi("sar cl,1 \n lahf \n and ah,0xef ; clear AF \n sahf")), // sra e
    [0x2c] = drci(@@asmi("sar dh,1 \n lahf \n and ah,0xef ; clear AF \n sahf")), // sra h
    [0x2d] = drci(@@asmi("sar dl,1 \n lahf \n and ah,0xef ; clear AF \n sahf")), // sra l
    [0x2e] = drci(@@asmi("sar byte [edx],1 \n lahf \n and ah,0xef ; clear AF \n sahf")), // sra (hl)
    [0x2f] = drci(@@asmi("sar al,1 \n lahf \n and ah,0xef ; clear AF \n sahf")), // sra a
#endif
    [0x30] = drci(@@asmi("rol bh,4 \n test bh,bh \n lahf \n and ah,0x40 ; ZF \n sahf")), // swap b
    [0x31] = drci(@@asmi("rol bl,4 \n test bl,bl \n lahf \n and ah,0x40 ; ZF \n sahf")), // swap c
    [0x32] = drci(@@asmi("rol ch,4 \n test ch,ch \n lahf \n and ah,0x40 ; ZF \n sahf")), // swap d
    [0x33] = drci(@@asmi("rol cl,4 \n test cl,cl \n lahf \n and ah,0x40 ; ZF \n sahf")), // swap e
    [0x34] = drci(@@asmi("rol dh,4 \n test dh,dh \n lahf \n and ah,0x40 ; ZF \n sahf")), // swap h
    [0x35] = drci(@@asmi("rol dl,4 \n test dl,dl \n lahf \n and ah,0x40 ; ZF \n sahf")), // swap l
    [0x36] = drci(@@asmi("rol byte [edx],4 \n cmp byte [edx],0 \n lahf \n and ah,0x40 ; ZF \n sahf")), // swap (hl)
    [0x37] = drci(@@asmi("rol al,4 \n test al,al \n lahf \n and ah,0x40 ; ZF \n sahf")), // swap a
#ifdef UNSAVE_FLAG_OPTIMIZATIONS
    [0x38] = drci(@@asmi("shr bh,1")),           // srl b
    [0x39] = drci(@@asmi("shr bl,1")),           // srl c
    [0x3a] = drci(@@asmi("shr ch,1")),           // srl d
    [0x3b] = drci(@@asmi("shr cl,1")),           // srl e
    [0x3c] = drci(@@asmi("shr dh,1")),           // srl h
    [0x3d] = drci(@@asmi("shr dl,1")),           // srl l
    [0x3e] = drci(@@asmi("shr byte [edx],1")),   // srl (hl)
    [0x3f] = drci(@@asmi("shr al,1")),           // srl a
#else
    [0x38] = drci(@@asmi("shr bh,1 \n lahf \n and ah,0xef ; clear AF \n sahf")), // srl b
    [0x39] = drci(@@asmi("shr bl,1 \n lahf \n and ah,0xef ; clear AF \n sahf")), // srl c
    [0x3a] = drci(@@asmi("shr ch,1 \n lahf \n and ah,0xef ; clear AF \n sahf")), // srl d
    [0x3b] = drci(@@asmi("shr cl,1 \n lahf \n and ah,0xef ; clear AF \n sahf")), // srl e
    [0x3c] = drci(@@asmi("shr dh,1 \n lahf \n and ah,0xef ; clear AF \n sahf")), // srl h
    [0x3d] = drci(@@asmi("shr dl,1 \n lahf \n and ah,0xef ; clear AF \n sahf")), // srl l
    [0x3e] = drci(@@asmi("shr byte [edx],1 \n lahf \n and ah,0xef ; clear AF \n sahf")), // srl (hl)
    [0x3f] = drci(@@asmi("shr al,1 \n lahf \n and ah,0xef ; clear AF \n sahf")), // srl a
#endif

    // lol why did i de-generalize this
#ifdef UNSAVE_FLAG_OPTIMIZATIONS
    [0x40] = drci(@@asmi("test bh,0x01")),      // bit 0,b
    [0x41] = drci(@@asmi("test bl,0x01")),      // bit 0,c
    [0x42] = drci(@@asmi("test ch,0x01")),      // bit 0,d
    [0x43] = drci(@@asmi("test cl,0x01")),      // bit 0,e
    [0x44] = drci(@@asmi("test dh,0x01")),      // bit 0,h
    [0x45] = drci(@@asmi("test dl,0x01")),      // bit 0,l
    [0x46] = drci(@@asmi("test byte [edx],0x01")), // bit 0,(hl)
    [0x47] = drci(@@asmi("test al,0x01")),      // bit 0,a
    [0x48] = drci(@@asmi("test bh,0x02")),      // bit 1,b
    [0x49] = drci(@@asmi("test bl,0x02")),      // bit 1,c
    [0x4a] = drci(@@asmi("test ch,0x02")),      // bit 1,d
    [0x4b] = drci(@@asmi("test cl,0x02")),      // bit 1,e
    [0x4c] = drci(@@asmi("test dh,0x02")),      // bit 1,h
    [0x4d] = drci(@@asmi("test dl,0x02")),      // bit 1,l
    [0x4e] = drci(@@asmi("test byte [edx],0x02")), // bit 1,(hl)
    [0x4f] = drci(@@asmi("test al,0x02")),      // bit 1,a
    [0x50] = drci(@@asmi("test bh,0x04")),      // bit 2,b
    [0x51] = drci(@@asmi("test bl,0x04")),      // bit 2,c
    [0x52] = drci(@@asmi("test ch,0x04")),      // bit 2,d
    [0x53] = drci(@@asmi("test cl,0x04")),      // bit 2,e
    [0x54] = drci(@@asmi("test dh,0x04")),      // bit 2,h
    [0x55] = drci(@@asmi("test dl,0x04")),      // bit 2,l
    [0x56] = drci(@@asmi("test byte [edx],0x04")), // bit 2,(hl)
    [0x57] = drci(@@asmi("test al,0x04")),      // bit 2,a
    [0x58] = drci(@@asmi("test bh,0x08")),      // bit 3,b
    [0x59] = drci(@@asmi("test bl,0x08")),      // bit 3,c
    [0x5a] = drci(@@asmi("test ch,0x08")),      // bit 3,d
    [0x5b] = drci(@@asmi("test cl,0x08")),      // bit 3,e
    [0x5c] = drci(@@asmi("test dh,0x08")),      // bit 3,h
    [0x5d] = drci(@@asmi("test dl,0x08")),      // bit 3,l
    [0x5e] = drci(@@asmi("test byte [edx],0x08")), // bit 3,(hl)
    [0x5f] = drci(@@asmi("test al,0x08")),      // bit 3,a
    [0x60] = drci(@@asmi("test bh,0x10")),      // bit 4,b
    [0x61] = drci(@@asmi("test bl,0x10")),      // bit 4,c
    [0x62] = drci(@@asmi("test ch,0x10")),      // bit 4,d
    [0x63] = drci(@@asmi("test cl,0x10")),      // bit 4,e
    [0x64] = drci(@@asmi("test dh,0x10")),      // bit 4,h
    [0x65] = drci(@@asmi("test dl,0x10")),      // bit 4,l
    [0x66] = drci(@@asmi("test byte [edx],0x10")), // bit 4,(hl)
    [0x67] = drci(@@asmi("test al,0x10")),      // bit 4,a
    [0x68] = drci(@@asmi("test bh,0x20")),      // bit 5,b
    [0x69] = drci(@@asmi("test bl,0x20")),      // bit 5,c
    [0x6a] = drci(@@asmi("test ch,0x20")),      // bit 5,d
    [0x6b] = drci(@@asmi("test cl,0x20")),      // bit 5,e
    [0x6c] = drci(@@asmi("test dh,0x20")),      // bit 5,h
    [0x6d] = drci(@@asmi("test dl,0x20")),      // bit 5,l
    [0x6e] = drci(@@asmi("test byte [edx],0x20")), // bit 5,(hl)
    [0x6f] = drci(@@asmi("test al,0x20")),      // bit 5,a
    [0x70] = drci(@@asmi("test bh,0x40")),      // bit 6,b
    [0x71] = drci(@@asmi("test bl,0x40")),      // bit 6,c
    [0x72] = drci(@@asmi("test ch,0x40")),      // bit 6,d
    [0x73] = drci(@@asmi("test cl,0x40")),      // bit 6,e
    [0x74] = drci(@@asmi("test dh,0x40")),      // bit 6,h
    [0x75] = drci(@@asmi("test dl,0x40")),      // bit 6,l
    [0x76] = drci(@@asmi("test byte [edx],0x40")), // bit 6,(hl)
    [0x77] = drci(@@asmi("test al,0x40")),      // bit 6,a
    [0x78] = drci(@@asmi("test bh,0x80")),      // bit 7,b
    [0x79] = drci(@@asmi("test bl,0x80")),      // bit 7,c
    [0x7a] = drci(@@asmi("test ch,0x80")),      // bit 7,d
    [0x7b] = drci(@@asmi("test cl,0x80")),      // bit 7,e
    [0x7c] = drci(@@asmi("test dh,0x80")),      // bit 7,h
    [0x7d] = drci(@@asmi("test dl,0x80")),      // bit 7,l
    [0x7e] = drci(@@asmi("test byte [edx],0x80")), // bit 7,(hl)
    [0x7f] = drci(@@asmi("test al,0x80")),      // bit 7,a
#else
    [0x40] = drci(@@asmi("lahf \n test bh,0x01 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 0,b
    [0x41] = drci(@@asmi("lahf \n test bl,0x01 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 0,c
    [0x42] = drci(@@asmi("lahf \n test ch,0x01 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 0,d
    [0x43] = drci(@@asmi("lahf \n test cl,0x01 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 0,e
    [0x44] = drci(@@asmi("lahf \n test dh,0x01 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 0,h
    [0x45] = drci(@@asmi("lahf \n test dl,0x01 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 0,l
    [0x46] = drci(@@asmi("lahf \n test byte [edx],0x01 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 0,(hl)
    [0x47] = drci(@@asmi("lahf \n test al,0x01 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 0,a
    [0x48] = drci(@@asmi("lahf \n test bh,0x02 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 1,b
    [0x49] = drci(@@asmi("lahf \n test bl,0x02 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 1,c
    [0x4a] = drci(@@asmi("lahf \n test ch,0x02 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 1,d
    [0x4b] = drci(@@asmi("lahf \n test cl,0x02 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 1,e
    [0x4c] = drci(@@asmi("lahf \n test dh,0x02 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 1,h
    [0x4d] = drci(@@asmi("lahf \n test dl,0x02 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 1,l
    [0x4e] = drci(@@asmi("lahf \n test byte [edx],0x02 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 1,(hl)
    [0x4f] = drci(@@asmi("lahf \n test al,0x02 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 1,a
    [0x50] = drci(@@asmi("lahf \n test bh,0x04 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 2,b
    [0x51] = drci(@@asmi("lahf \n test bl,0x04 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 2,c
    [0x52] = drci(@@asmi("lahf \n test ch,0x04 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 2,d
    [0x53] = drci(@@asmi("lahf \n test cl,0x04 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 2,e
    [0x54] = drci(@@asmi("lahf \n test dh,0x04 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 2,h
    [0x55] = drci(@@asmi("lahf \n test dl,0x04 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 2,l
    [0x56] = drci(@@asmi("lahf \n test byte [edx],0x04 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 2,(hl)
    [0x57] = drci(@@asmi("lahf \n test al,0x04 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 2,a
    [0x58] = drci(@@asmi("lahf \n test bh,0x08 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 3,b
    [0x59] = drci(@@asmi("lahf \n test bl,0x08 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 3,c
    [0x5a] = drci(@@asmi("lahf \n test ch,0x08 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 3,d
    [0x5b] = drci(@@asmi("lahf \n test cl,0x08 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 3,e
    [0x5c] = drci(@@asmi("lahf \n test dh,0x08 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 3,h
    [0x5d] = drci(@@asmi("lahf \n test dl,0x08 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 3,l
    [0x5e] = drci(@@asmi("lahf \n test byte [edx],0x08 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 3,(hl)
    [0x5f] = drci(@@asmi("lahf \n test al,0x08 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 3,a
    [0x60] = drci(@@asmi("lahf \n test bh,0x10 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 4,b
    [0x61] = drci(@@asmi("lahf \n test bl,0x10 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 4,c
    [0x62] = drci(@@asmi("lahf \n test ch,0x10 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 4,d
    [0x63] = drci(@@asmi("lahf \n test cl,0x10 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 4,e
    [0x64] = drci(@@asmi("lahf \n test dh,0x10 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 4,h
    [0x65] = drci(@@asmi("lahf \n test dl,0x10 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 4,l
    [0x66] = drci(@@asmi("lahf \n test byte [edx],0x10 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 4,(hl)
    [0x67] = drci(@@asmi("lahf \n test al,0x10 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 4,a
    [0x68] = drci(@@asmi("lahf \n test bh,0x20 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 5,b
    [0x69] = drci(@@asmi("lahf \n test bl,0x20 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 5,c
    [0x6a] = drci(@@asmi("lahf \n test ch,0x20 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 5,d
    [0x6b] = drci(@@asmi("lahf \n test cl,0x20 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 5,e
    [0x6c] = drci(@@asmi("lahf \n test dh,0x20 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 5,h
    [0x6d] = drci(@@asmi("lahf \n test dl,0x20 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 5,l
    [0x6e] = drci(@@asmi("lahf \n test byte [edx],0x20 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 5,(hl)
    [0x6f] = drci(@@asmi("lahf \n test al,0x20 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 5,a
    [0x70] = drci(@@asmi("lahf \n test bh,0x40 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 6,b
    [0x71] = drci(@@asmi("lahf \n test bl,0x40 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 6,c
    [0x72] = drci(@@asmi("lahf \n test ch,0x40 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 6,d
    [0x73] = drci(@@asmi("lahf \n test cl,0x40 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 6,e
    [0x74] = drci(@@asmi("lahf \n test dh,0x40 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 6,h
    [0x75] = drci(@@asmi("lahf \n test dl,0x40 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 6,l
    [0x76] = drci(@@asmi("lahf \n test byte [edx],0x40 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 6,(hl)
    [0x77] = drci(@@asmi("lahf \n test al,0x40 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 6,a
    [0x78] = drci(@@asmi("lahf \n test bh,0x80 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 7,b
    [0x79] = drci(@@asmi("lahf \n test bl,0x80 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 7,c
    [0x7a] = drci(@@asmi("lahf \n test ch,0x80 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 7,d
    [0x7b] = drci(@@asmi("lahf \n test cl,0x80 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 7,e
    [0x7c] = drci(@@asmi("lahf \n test dh,0x80 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 7,h
    [0x7d] = drci(@@asmi("lahf \n test dl,0x80 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 7,l
    [0x7e] = drci(@@asmi("lahf \n test byte [edx],0x80 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 7,(hl)
    [0x7f] = drci(@@asmi("lahf \n test al,0x80 \n push eax \n mov al,ah \n lahf \n and al,0x01 ; keep old CF \n and ah,0xfe ; overriding the new CF \n or al,0x10 ; set AF \n or ah,al \n sahf \n pop eax")), // bit 7,a
#endif

    [0x80] = drci(@@asmi("lahf \n and bh,0xfe \n sahf")), // res 0,b
    [0x81] = drci(@@asmi("lahf \n and bl,0xfe \n sahf")), // res 0,c
    [0x82] = drci(@@asmi("lahf \n and ch,0xfe \n sahf")), // res 0,d
    [0x83] = drci(@@asmi("lahf \n and cl,0xfe \n sahf")), // res 0,e
    [0x84] = drci(@@asmi("lahf \n and dh,0xfe \n sahf")), // res 0,h
    [0x85] = drci(@@asmi("lahf \n and dl,0xfe \n sahf")), // res 0,l
    [0x86] = drci(@@asmi("lahf \n and byte [edx],0xfe \n sahf")), // res 0,(hl)
    [0x87] = drci(@@asmi("lahf \n and al,0xfe \n sahf")), // res 0,a
    [0x88] = drci(@@asmi("lahf \n and bh,0xfd \n sahf")), // res 1,b
    [0x89] = drci(@@asmi("lahf \n and bl,0xfd \n sahf")), // res 1,c
    [0x8a] = drci(@@asmi("lahf \n and ch,0xfd \n sahf")), // res 1,d
    [0x8b] = drci(@@asmi("lahf \n and cl,0xfd \n sahf")), // res 1,e
    [0x8c] = drci(@@asmi("lahf \n and dh,0xfd \n sahf")), // res 1,h
    [0x8d] = drci(@@asmi("lahf \n and dl,0xfd \n sahf")), // res 1,l
    [0x8e] = drci(@@asmi("lahf \n and byte [edx],0xfd \n sahf")), // res 1,(hl)
    [0x8f] = drci(@@asmi("lahf \n and al,0xfd \n sahf")), // res 1,a
    [0x90] = drci(@@asmi("lahf \n and bh,0xfb \n sahf")), // res 2,b
    [0x91] = drci(@@asmi("lahf \n and bl,0xfb \n sahf")), // res 2,c
    [0x92] = drci(@@asmi("lahf \n and ch,0xfb \n sahf")), // res 2,d
    [0x93] = drci(@@asmi("lahf \n and cl,0xfb \n sahf")), // res 2,e
    [0x94] = drci(@@asmi("lahf \n and dh,0xfb \n sahf")), // res 2,h
    [0x95] = drci(@@asmi("lahf \n and dl,0xfb \n sahf")), // res 2,l
    [0x96] = drci(@@asmi("lahf \n and byte [edx],0xfb \n sahf")), // res 2,(hl)
    [0x97] = drci(@@asmi("lahf \n and al,0xfb \n sahf")), // res 2,a
    [0x98] = drci(@@asmi("lahf \n and bh,0xf7 \n sahf")), // res 3,b
    [0x99] = drci(@@asmi("lahf \n and bl,0xf7 \n sahf")), // res 3,c
    [0x9a] = drci(@@asmi("lahf \n and ch,0xf7 \n sahf")), // res 3,d
    [0x9b] = drci(@@asmi("lahf \n and cl,0xf7 \n sahf")), // res 3,e
    [0x9c] = drci(@@asmi("lahf \n and dh,0xf7 \n sahf")), // res 3,h
    [0x9d] = drci(@@asmi("lahf \n and dl,0xf7 \n sahf")), // res 3,l
    [0x9e] = drci(@@asmi("lahf \n and byte [edx],0xf7 \n sahf")), // res 3,(hl)
    [0x9f] = drci(@@asmi("lahf \n and al,0xf7 \n sahf")), // res 3,a
    [0xa0] = drci(@@asmi("lahf \n and bh,0xef \n sahf")), // res 4,b
    [0xa1] = drci(@@asmi("lahf \n and bl,0xef \n sahf")), // res 4,c
    [0xa2] = drci(@@asmi("lahf \n and ch,0xef \n sahf")), // res 4,d
    [0xa3] = drci(@@asmi("lahf \n and cl,0xef \n sahf")), // res 4,e
    [0xa4] = drci(@@asmi("lahf \n and dh,0xef \n sahf")), // res 4,h
    [0xa5] = drci(@@asmi("lahf \n and dl,0xef \n sahf")), // res 4,l
    [0xa6] = drci(@@asmi("lahf \n and byte [edx],0xef \n sahf")), // res 4,(hl)
    [0xa7] = drci(@@asmi("lahf \n and al,0xef \n sahf")), // res 4,a
    [0xa8] = drci(@@asmi("lahf \n and bh,0xdf \n sahf")), // res 5,b
    [0xa9] = drci(@@asmi("lahf \n and bl,0xdf \n sahf")), // res 5,c
    [0xaa] = drci(@@asmi("lahf \n and ch,0xdf \n sahf")), // res 5,d
    [0xab] = drci(@@asmi("lahf \n and cl,0xdf \n sahf")), // res 5,e
    [0xac] = drci(@@asmi("lahf \n and dh,0xdf \n sahf")), // res 5,h
    [0xad] = drci(@@asmi("lahf \n and dl,0xdf \n sahf")), // res 5,l
    [0xae] = drci(@@asmi("lahf \n and byte [edx],0xdf \n sahf")), // res 5,(hl)
    [0xaf] = drci(@@asmi("lahf \n and al,0xdf \n sahf")), // res 5,a
    [0xb0] = drci(@@asmi("lahf \n and bh,0xbf \n sahf")), // res 6,b
    [0xb1] = drci(@@asmi("lahf \n and bl,0xbf \n sahf")), // res 6,c
    [0xb2] = drci(@@asmi("lahf \n and ch,0xbf \n sahf")), // res 6,d
    [0xb3] = drci(@@asmi("lahf \n and cl,0xbf \n sahf")), // res 6,e
    [0xb4] = drci(@@asmi("lahf \n and dh,0xbf \n sahf")), // res 6,h
    [0xb5] = drci(@@asmi("lahf \n and dl,0xbf \n sahf")), // res 6,l
    [0xb6] = drci(@@asmi("lahf \n and byte [edx],0xbf \n sahf")), // res 6,(hl)
    [0xb7] = drci(@@asmi("lahf \n and al,0xbf \n sahf")), // res 6,a
    [0xb8] = drci(@@asmi("lahf \n and bh,0x7f \n sahf")), // res 7,b
    [0xb9] = drci(@@asmi("lahf \n and bl,0x7f \n sahf")), // res 7,c
    [0xba] = drci(@@asmi("lahf \n and ch,0x7f \n sahf")), // res 7,d
    [0xbb] = drci(@@asmi("lahf \n and cl,0x7f \n sahf")), // res 7,e
    [0xbc] = drci(@@asmi("lahf \n and dh,0x7f \n sahf")), // res 7,h
    [0xbd] = drci(@@asmi("lahf \n and dl,0x7f \n sahf")), // res 7,l
    [0xbe] = drci(@@asmi("lahf \n and byte [edx],0x7f \n sahf")), // res 7,(hl)
    [0xbf] = drci(@@asmi("lahf \n and al,0x7f \n sahf")), // res 7,a

    [0xc0] = drci(@@asmi("lahf \n or bh,0x01 \n sahf")), // set 0,b
    [0xc1] = drci(@@asmi("lahf \n or bl,0x01 \n sahf")), // set 0,c
    [0xc2] = drci(@@asmi("lahf \n or ch,0x01 \n sahf")), // set 0,d
    [0xc3] = drci(@@asmi("lahf \n or cl,0x01 \n sahf")), // set 0,e
    [0xc4] = drci(@@asmi("lahf \n or dh,0x01 \n sahf")), // set 0,h
    [0xc5] = drci(@@asmi("lahf \n or dl,0x01 \n sahf")), // set 0,l
    [0xc6] = drci(@@asmi("lahf \n or byte [edx],0x01 \n sahf")), // set 0,(hl)
    [0xc7] = drci(@@asmi("lahf \n or al,0x01 \n sahf")), // set 0,a
    [0xc8] = drci(@@asmi("lahf \n or bh,0x02 \n sahf")), // set 1,b
    [0xc9] = drci(@@asmi("lahf \n or bl,0x02 \n sahf")), // set 1,c
    [0xca] = drci(@@asmi("lahf \n or ch,0x02 \n sahf")), // set 1,d
    [0xcb] = drci(@@asmi("lahf \n or cl,0x02 \n sahf")), // set 1,e
    [0xcc] = drci(@@asmi("lahf \n or dh,0x02 \n sahf")), // set 1,h
    [0xcd] = drci(@@asmi("lahf \n or dl,0x02 \n sahf")), // set 1,l
    [0xce] = drci(@@asmi("lahf \n or byte [edx],0x02 \n sahf")), // set 1,(hl)
    [0xcf] = drci(@@asmi("lahf \n or al,0x02 \n sahf")), // set 1,a
    [0xd0] = drci(@@asmi("lahf \n or bh,0x04 \n sahf")), // set 2,b
    [0xd1] = drci(@@asmi("lahf \n or bl,0x04 \n sahf")), // set 2,c
    [0xd2] = drci(@@asmi("lahf \n or ch,0x04 \n sahf")), // set 2,d
    [0xd3] = drci(@@asmi("lahf \n or cl,0x04 \n sahf")), // set 2,e
    [0xd4] = drci(@@asmi("lahf \n or dh,0x04 \n sahf")), // set 2,h
    [0xd5] = drci(@@asmi("lahf \n or dl,0x04 \n sahf")), // set 2,l
    [0xd6] = drci(@@asmi("lahf \n or byte [edx],0x04 \n sahf")), // set 2,(hl)
    [0xd7] = drci(@@asmi("lahf \n or al,0x04 \n sahf")), // set 2,a
    [0xd8] = drci(@@asmi("lahf \n or bh,0x08 \n sahf")), // set 3,b
    [0xd9] = drci(@@asmi("lahf \n or bl,0x08 \n sahf")), // set 3,c
    [0xda] = drci(@@asmi("lahf \n or ch,0x08 \n sahf")), // set 3,d
    [0xdb] = drci(@@asmi("lahf \n or cl,0x08 \n sahf")), // set 3,e
    [0xdc] = drci(@@asmi("lahf \n or dh,0x08 \n sahf")), // set 3,h
    [0xdd] = drci(@@asmi("lahf \n or dl,0x08 \n sahf")), // set 3,l
    [0xde] = drci(@@asmi("lahf \n or byte [edx],0x08 \n sahf")), // set 3,(hl)
    [0xdf] = drci(@@asmi("lahf \n or al,0x08 \n sahf")), // set 3,a
    [0xe0] = drci(@@asmi("lahf \n or bh,0x10 \n sahf")), // set 4,b
    [0xe1] = drci(@@asmi("lahf \n or bl,0x10 \n sahf")), // set 4,c
    [0xe2] = drci(@@asmi("lahf \n or ch,0x10 \n sahf")), // set 4,d
    [0xe3] = drci(@@asmi("lahf \n or cl,0x10 \n sahf")), // set 4,e
    [0xe4] = drci(@@asmi("lahf \n or dh,0x10 \n sahf")), // set 4,h
    [0xe5] = drci(@@asmi("lahf \n or dl,0x10 \n sahf")), // set 4,l
    [0xe6] = drci(@@asmi("lahf \n or byte [edx],0x10 \n sahf")), // set 4,(hl)
    [0xe7] = drci(@@asmi("lahf \n or al,0x10 \n sahf")), // set 4,a
    [0xe8] = drci(@@asmi("lahf \n or bh,0x20 \n sahf")), // set 5,b
    [0xe9] = drci(@@asmi("lahf \n or bl,0x20 \n sahf")), // set 5,c
    [0xea] = drci(@@asmi("lahf \n or ch,0x20 \n sahf")), // set 5,d
    [0xeb] = drci(@@asmi("lahf \n or cl,0x20 \n sahf")), // set 5,e
    [0xec] = drci(@@asmi("lahf \n or dh,0x20 \n sahf")), // set 5,h
    [0xed] = drci(@@asmi("lahf \n or dl,0x20 \n sahf")), // set 5,l
    [0xee] = drci(@@asmi("lahf \n or byte [edx],0x20 \n sahf")), // set 5,(hl)
    [0xef] = drci(@@asmi("lahf \n or al,0x20 \n sahf")), // set 5,a
    [0xf0] = drci(@@asmi("lahf \n or bh,0x40 \n sahf")), // set 6,b
    [0xf1] = drci(@@asmi("lahf \n or bl,0x40 \n sahf")), // set 6,c
    [0xf2] = drci(@@asmi("lahf \n or ch,0x40 \n sahf")), // set 6,d
    [0xf3] = drci(@@asmi("lahf \n or cl,0x40 \n sahf")), // set 6,e
    [0xf4] = drci(@@asmi("lahf \n or dh,0x40 \n sahf")), // set 6,h
    [0xf5] = drci(@@asmi("lahf \n or dl,0x40 \n sahf")), // set 6,l
    [0xf6] = drci(@@asmi("lahf \n or byte [edx],0x40 \n sahf")), // set 6,(hl)
    [0xf7] = drci(@@asmi("lahf \n or al,0x40 \n sahf")), // set 6,a
    [0xf8] = drci(@@asmi("lahf \n or bh,0x80 \n sahf")), // set 7,b
    [0xf9] = drci(@@asmi("lahf \n or bl,0x80 \n sahf")), // set 7,c
    [0xfa] = drci(@@asmi("lahf \n or ch,0x80 \n sahf")), // set 7,d
    [0xfb] = drci(@@asmi("lahf \n or cl,0x80 \n sahf")), // set 7,e
    [0xfc] = drci(@@asmi("lahf \n or dh,0x80 \n sahf")), // set 7,h
    [0xfd] = drci(@@asmi("lahf \n or dl,0x80 \n sahf")), // set 7,l
    [0xfe] = drci(@@asmi("lahf \n or byte [edx],0x80 \n sahf")), // set 7,(hl)
    [0xff] = drci(@@asmi("lahf \n or al,0x80 \n sahf")), // set 7,a
};
