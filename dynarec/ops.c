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

// Grundwerte für die bitweisen Funktionen test (v | 0x00), and (v | 0x20) und or (v | 0x08)
static uint8_t dynarec_table_CBbit[7] = {
    0xC7, // bit bh,n
    0xC3, // bit bl,n
    0xC5, // bit ch,n
    0xC1, // bit cl,n
    0xC6, // bit dh,n
    0xC2, // bit dl,n
    0x02  // bit byte [edx],n
};

#define def_drop_swap_r(r, rol, test) \
    drop(swap_##r) \
    { \
        /* rol r,4 */ \
        drvmapp2(rol); \
        /* ...; test r,r; lahf */ \
        drvmapp4(0x9F000004 | (test << 8)); \
        /* and ah,0x40 (ZF); sahf */ \
        drvmapp4(0x9E40E480); \
    }

def_drop_swap_r(b, 0xC7C0, 0xFF84)
def_drop_swap_r(c, 0xC3C0, 0xDB84)
def_drop_swap_r(d, 0xC5C0, 0xED84)
def_drop_swap_r(e, 0xC1C0, 0xC984)
def_drop_swap_r(h, 0xC6C0, 0xF684)
def_drop_swap_r(l, 0xC2C0, 0xD284)
def_drop_swap_r(a, 0xC0C0, 0xC084)

drop(swap__hl)
{
    // rol byte [edx],4
    drvmapp2(0x02C0);
    // ...; cmp byte [edx],0
    drvmapp4(0x003A8004);
    // lahf; and ah,0x40
    drvmapp4(0x40E4809F);
    // sahf
    drvmapp1(0x9E);
}

#ifndef UNSAVE_FLAG_OPTIMIZATIONS
#define def_drop_rX_r(d, reg, rcX, test) \
    drop(r##d##_##reg) \
    { \
        /* rcX r,1; push eax; lahf */ \
        drvmapp4(0x9F500000 | rcX); \
        /* mov al,ah; and al,0x01 (CF) */ \
        drvmapp4(0x0124E088); \
        /* test r,r; lahf; and ah,0x6A (ZF) */ \
        drvmapp4(0x809F0000 | test); \
        /* ...; or ah,al */ \
        drvmapp4(0xC4086AE4); \
        /* sahf; pop eax */ \
        drvmapp2(0x589E); \
    }

def_drop_rX_r(lc, b, 0xC7D0, 0xFF84)
def_drop_rX_r(lc, c, 0xC3D0, 0xDB84)
def_drop_rX_r(lc, d, 0xC5D0, 0xED84)
def_drop_rX_r(lc, e, 0xC1D0, 0xC984)
def_drop_rX_r(lc, h, 0xC6D0, 0xF684)
def_drop_rX_r(lc, l, 0xC2D0, 0xD284)
def_drop_rX_r(lc, a, 0xC0D0, 0xC084)

def_drop_rX_r(rc, b, 0xCFD0, 0xFF84)
def_drop_rX_r(rc, c, 0xCBD0, 0xDB84)
def_drop_rX_r(rc, d, 0xCDD0, 0xED84)
def_drop_rX_r(rc, e, 0xC9D0, 0xC984)
def_drop_rX_r(rc, h, 0xCED0, 0xF684)
def_drop_rX_r(rc, l, 0xCAD0, 0xD284)
def_drop_rX_r(rc, a, 0xC8D0, 0xC084)

def_drop_rX_r(l, b, 0xD7D0, 0xFF84)
def_drop_rX_r(l, c, 0xD3D0, 0xDB84)
def_drop_rX_r(l, d, 0xD5D0, 0xED84)
def_drop_rX_r(l, e, 0xD1D0, 0xC984)
def_drop_rX_r(l, h, 0xD6D0, 0xF684)
def_drop_rX_r(l, l, 0xD2D0, 0xD284)
def_drop_rX_r(l, a, 0xD0D0, 0xC084)

def_drop_rX_r(r, b, 0xDFD0, 0xFF84)
def_drop_rX_r(r, c, 0xDBD0, 0xDB84)
def_drop_rX_r(r, d, 0xDDD0, 0xED84)
def_drop_rX_r(r, e, 0xD9D0, 0xC984)
def_drop_rX_r(r, h, 0xDED0, 0xF684)
def_drop_rX_r(r, l, 0xDAD0, 0xD284)
def_drop_rX_r(r, a, 0xD8D0, 0xC084)

drop(rlc__hl)
{
    // rol byte [edx],1; push eax; lahf
    drvmapp4(0x9F5002D0);
    // mov al,ah; and al,0x01 (CF)
    drvmapp4(0x0124E088);
    // cmp byte [edx],0; lahf
    drvmapp4(0x9F003A80);
    // and ah,0x6A (ZF); or ah,al
    drvmapp4(0x086AE480);
    // ...; sahf
    drvmapp2(0x9EC4);
    // pop eax
    drvmapp1(0x58);
}

drop(rrc__hl)
{
    // ror byte [edx],1; push eax; lahf
    drvmapp4(0x9F500AD0);
    // mov al,ah; and al,0x01 (CF)
    drvmapp4(0x0124E088);
    // cmp byte [edx],0; lahf
    drvmapp4(0x9F003A80);
    // and ah,0x6A (ZF); or ah,al
    drvmapp4(0x086AE480);
    // ...; sahf
    drvmapp2(0x9EC4);
    // pop eax
    drvmapp1(0x58);
}

drop(rl__hl)
{
    // rcl byte [edx],1; push eax; lahf
    drvmapp4(0x9F5012D0);
    // mov al,ah; and al,0x01 (CF)
    drvmapp4(0x0124E088);
    // cmp byte [edx],0; lahf
    drvmapp4(0x9F003A80);
    // and ah,0x6A (ZF); or ah,al
    drvmapp4(0x086AE480);
    // ...; sahf
    drvmapp2(0x9EC4);
    // pop eax
    drvmapp1(0x58);
}

drop(rr__hl)
{
    // rcr byte [edx],1; push eax; lahf
    drvmapp4(0x9F501AD0);
    // mov al,ah; and al,0x01 (CF)
    drvmapp4(0x0124E088);
    // cmp byte [edx],0; lahf
    drvmapp4(0x9F003A80);
    // and ah,0x6A (ZF); or ah,al
    drvmapp4(0x086AE480);
    // ...; sahf
    drvmapp2(0x9EC4);
    // pop eax
    drvmapp1(0x58);
}

#define def_drop_sXX_r(da, r, sXX) \
    drop(s##da##_##r) \
    { \
        /* sXX r,1 */ \
        drvmapp2(sXX); \
        /* lahf */ \
        drvmapp1(0x9F); \
        /* and ah,0xEF (AF löschen); sahf */ \
        drvmapp4(0x9EEFE480); \
    }

def_drop_sXX_r(la,   b, 0xE7D0) // shl bh,1
def_drop_sXX_r(la,   c, 0xE3D0) // shl bl,1
def_drop_sXX_r(la,   d, 0xE5D0) // shl ch,1
def_drop_sXX_r(la,   e, 0xE1D0) // shl cl,1
def_drop_sXX_r(la,   h, 0xE6D0) // shl dh,1
def_drop_sXX_r(la,   l, 0xE2D0) // shl dl,1
def_drop_sXX_r(la, _hl, 0x22D0) // shl byte [edx],1
def_drop_sXX_r(la,   a, 0xE0D0) // shl al,1

def_drop_sXX_r(ra,   b, 0xFFD0) // sar bh,1
def_drop_sXX_r(ra,   c, 0xFBD0) // sar bl,1
def_drop_sXX_r(ra,   d, 0xFDD0) // sar ch,1
def_drop_sXX_r(ra,   e, 0xF9D0) // sar cl,1
def_drop_sXX_r(ra,   h, 0xFED0) // sar dh,1
def_drop_sXX_r(ra,   l, 0xFAD0) // sar dl,1
def_drop_sXX_r(ra, _hl, 0x3AD0) // sar byte [edx],1
def_drop_sXX_r(ra,   a, 0xF8D0) // sar al,1

def_drop_sXX_r(rl,   b, 0xEFD0) // shr bh,1
def_drop_sXX_r(rl,   c, 0xEBD0) // shr bl,1
def_drop_sXX_r(rl,   d, 0xEDD0) // shr ch,1
def_drop_sXX_r(rl,   e, 0xE9D0) // shr cl,1
def_drop_sXX_r(rl,   h, 0xEED0) // shr dh,1
def_drop_sXX_r(rl,   l, 0xEAD0) // shr dl,1
def_drop_sXX_r(rl, _hl, 0x2AD0) // shr byte [edx],1
def_drop_sXX_r(rl,   a, 0xE8D0) // shr al,1
#endif

static void (*const dynarec_table_CB[64])(void) = {
#ifndef UNSAVE_FLAG_OPTIMIZATIONS
    dran(0x00, rlc_b),
    dran(0x01, rlc_c),
    dran(0x02, rlc_d),
    dran(0x03, rlc_e),
    dran(0x04, rlc_h),
    dran(0x05, rlc_l),
    dran(0x06, rlc__hl),
    dran(0x07, rlc_a),
    dran(0x08, rrc_b),
    dran(0x09, rrc_c),
    dran(0x0A, rrc_d),
    dran(0x0B, rrc_e),
    dran(0x0C, rrc_h),
    dran(0x0D, rrc_l),
    dran(0x0E, rrc__hl),
    dran(0x0F, rrc_a),
    dran(0x10, rl_b),
    dran(0x11, rl_c),
    dran(0x12, rl_d),
    dran(0x13, rl_e),
    dran(0x14, rl_h),
    dran(0x15, rl_l),
    dran(0x16, rl__hl),
    dran(0x17, rl_a),
    dran(0x18, rr_b),
    dran(0x19, rr_c),
    dran(0x1A, rr_d),
    dran(0x1B, rr_e),
    dran(0x1C, rr_h),
    dran(0x1D, rr_l),
    dran(0x1E, rr__hl),
    dran(0x1F, rr_a),
    dran(0x20, sla_b),
    dran(0x21, sla_c),
    dran(0x22, sla_d),
    dran(0x23, sla_e),
    dran(0x24, sla_h),
    dran(0x25, sla_l),
    dran(0x26, sla__hl),
    dran(0x27, sla_a),
    dran(0x28, sra_b),
    dran(0x29, sra_c),
    dran(0x2A, sra_d),
    dran(0x2B, sra_e),
    dran(0x2C, sra_h),
    dran(0x2D, sra_l),
    dran(0x2E, sra__hl),
    dran(0x2F, sra_a),
#endif
    dran(0x30, swap_b),
    dran(0x31, swap_c),
    dran(0x32, swap_d),
    dran(0x33, swap_e),
    dran(0x34, swap_h),
    dran(0x35, swap_l),
    dran(0x36, swap__hl),
    dran(0x37, swap_a),
#ifndef UNSAVE_FLAG_OPTIMIZATIONS
    dran(0x38, srl_b),
    dran(0x39, srl_c),
    dran(0x3A, srl_d),
    dran(0x3B, srl_e),
    dran(0x3C, srl_h),
    dran(0x3D, srl_l),
    dran(0x3E, srl__hl),
    dran(0x3F, srl_a)
#endif
};

#ifdef UNSAVE_FLAG_OPTIMIZATIONS
static uint16_t dynarec_const_CB[64] = {
    [0x00] = 0xC7D0,        // rlc b = rol bh,1
    [0x01] = 0xC3D0,        // rlc c = rol bl,1
    [0x02] = 0xC5D0,        // rlc d = rol ch,1
    [0x03] = 0xC1D0,        // rlc e = rol cl,1
    [0x04] = 0xC6D0,        // rlc h = rol dh,1
    [0x05] = 0xC2D0,        // rlc l = rol dl,1
    [0x06] = 0x02D0,        // rlc (hl) = rol byte [edx],1
    [0x07] = 0xC0D0,        // rlc a = rol al,1
    [0x08] = 0xCFD0,        // rrc b = ror bh,1
    [0x09] = 0xCBD0,        // rrc c = ror bl,1
    [0x0A] = 0xCDD0,        // rrc d = ror ch,1
    [0x0B] = 0xC9D0,        // rrc e = ror cl,1
    [0x0C] = 0xCED0,        // rrc h = ror dh,1
    [0x0D] = 0xCAD0,        // rrc l = ror dl,1
    [0x0E] = 0x0AD0,        // rrc (hl) = ror byte [edx],1
    [0x0F] = 0xC8D0,        // rrc a = ror al,1
    [0x10] = 0xD7D0,        // rl b = rcl bh,1
    [0x11] = 0xD3D0,        // rl c = rcl bl,1
    [0x12] = 0xD5D0,        // rl d = rcl ch,1
    [0x13] = 0xD1D0,        // rl e = rcl cl,1
    [0x14] = 0xD6D0,        // rl h = rcl dh,1
    [0x15] = 0xD2D0,        // rl l = rcl dl,1
    [0x16] = 0x12D0,        // rl (hl) = rcl byte [edx],1
    [0x17] = 0xD0D0,        // rl a = rcl al,1
    [0x18] = 0xDFD0,        // rr b = rcr bh,1
    [0x19] = 0xDBD0,        // rr c = rcr bl,1
    [0x1A] = 0xDDD0,        // rr d = rcr ch,1
    [0x1B] = 0xD9D0,        // rr e = rcr cl,1
    [0x1C] = 0xDED0,        // rr h = rcr dh,1
    [0x1D] = 0xDAD0,        // rr l = rcr dl,1
    [0x1E] = 0x1AD0,        // rr (hl) = rcr byte [edx],1
    [0x1F] = 0xD8D0,        // rr a = rcr al,1
    [0x20] = 0xE7D0,        // sla b = shl bh,1
    [0x21] = 0xE3D0,        // sla c = shl bl,1
    [0x22] = 0xE5D0,        // sla d = shl ch,1
    [0x23] = 0xE1D0,        // sla e = shl cl,1
    [0x24] = 0xE6D0,        // sla h = shl dh,1
    [0x25] = 0xE2D0,        // sla l = shl dl,1
    [0x26] = 0x22D0,        // sla (hl) = shl byte [edx],1
    [0x27] = 0xE0D0,        // sla a = shl al,1
    [0x28] = 0xFFD0,        // sra b = sar bh,1
    [0x29] = 0xFBD0,        // sra c = sar bl,1
    [0x2A] = 0xFDD0,        // sra d = sar ch,1
    [0x2B] = 0xF9D0,        // sra e = sar cl,1
    [0x2C] = 0xFED0,        // sra h = sar dh,1
    [0x2D] = 0xFAD0,        // sra l = sar dl,1
    [0x2E] = 0x3AD0,        // sra (hl) = sar byte [edx],1
    [0x2F] = 0xF8D0,        // sra a = sar al,1
    [0x38] = 0xEFD0,        // srl b = shr bh,1
    [0x39] = 0xEBD0,        // srl c = shr bl,1
    [0x3A] = 0xEDD0,        // srl d = shr ch,1
    [0x3B] = 0xE9D0,        // srl e = shr cl,1
    [0x3C] = 0xEED0,        // srl h = shr dh,1
    [0x3D] = 0xEAD0,        // srl l = shr dl,1
    [0x3E] = 0x2AD0,        // srl (hl) = shr byte [edx],1
    [0x3F] = 0xE8D0,        // srl a = shr al,1
};
#endif
