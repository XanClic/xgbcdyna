#define DYNAREC_LOAD 1
#define DYNAREC_CNST 0

#define drop(n) void _dynarec_##n(void)
#define dran(o, n) [o] = &_dynarec_##n

static uint8_t *drc;
static size_t dri, drei;
static uint16_t drip;
static unsigned dr_cycle_counter;

#define drvmapp1(v) vmapp1(drc, dri, v)
#define drvmapp2(v) vmapp2(drc, dri, v)
#define drvmapp4(v) vmapp4(drc, dri, v)

drop(nop)
{
}

drop(ld_bc_nn)
{
    // mov bx,nn
    drvmapp2(0xBB66);
    drvmapp2(MEM16(drip));
    drip += 2;
}

drop(jr_n)
{
    drip += (int8_t)MEM8(drip) + 1;
}

drop(jrnz_n)
{
    // jz byte
    drvmapp2(0x1574);
    exit_vm(drc, &dri, drei, drip + (int8_t)MEM8(drip) + 1, dr_cycle_counter);
    drip++;
}

drop(ldi__hl_a)
{
    // mov [edx],al
    drvmapp2(0x0288);
    // lahf; inc dx; sahf
    drvmapp4(0x9E42669F);
}

drop(jrz_n)
{
    // jnz byte
    drvmapp2(0x1575);
    exit_vm(drc, &dri, drei, drip + (int8_t)MEM8(drip) + 1, dr_cycle_counter);
    drip++;
}

drop(jrnc_n)
{
    // jc byte
    drvmapp2(0x1572);
    exit_vm(drc, &dri, drei, drip + (int8_t)MEM8(drip) + 1, dr_cycle_counter);
    drip++;
}

drop(ld__hl_nn)
{
    // mov byte [edx],nn
    drvmapp2(0x02C6);
    drvmapp1(MEM8(drip++));
}

drop(jrc_n)
{
    // jnc byte
    drvmapp2(0x1573);
    exit_vm(drc, &dri, drei, drip + (int8_t)MEM8(drip) + 1, dr_cycle_counter);
    drip++;
}

drop(xor_a_a)
{
    // xor al,al; mov ah 0x42 (ZF)
    drvmapp4(0x42B4C030);
    // sahf
    drvmapp1(0x9E);
}

drop(or_a_c)
{
    // or al,bl
    drvmapp2(0xD808);
    // lahf
    drvmapp1(0x9F);
    // and ah, 0x6A (ZF); sahf
    drvmapp4(0x9E6AE480);
}

drop(jp_nn)
{
    drip = MEM16(drip);
}

drop(ret)
{
    exit_vm_by_ret(drc, &dri, drei, dr_cycle_counter);
}

drop(call_nn)
{
    // lahf; sub bp,2
    drvmapp4(0xED83669F);
    // ...; sahf; mov word [ebp],drip+2
    drvmapp4(0xC7669E02);
    drvmapp2(0x0045);
    drvmapp2(drip + 2);
    drip = MEM16(drip);
}

drop(leave_vm_imm)
{
    exit_vm(drc, &dri, drei, drip - 1, dr_cycle_counter);
}

drop(ld__nn_a)
{
    // mov byte [nnnn],al
    drvmapp1(0xA2);
    drvmapp4(0x40000000 + MEM16(drip));
    drip += 2;
}

drop(pop_af)
{
    // movzx eax,byte [ebp]
    drvmapp4(0x0045B60F);
    // mov ah,[reverse_flag_table+eax]
    drvmapp2(0xA08A);
    drvmapp4((uintptr_t)&reverse_flag_table);
    // mov al,[ebp+1]; add bp,2
    drvmapp4(0x6601458A);
    // ...; or eax,0x40000000
    drvmapp4(0x0D02C583);
    drvmapp4(0x40000000);
    // sahf
    drvmapp1(0x9E);
}

drop(di)
{
    // mov byte [ime],0
    drvmapp2(0x05C6);
    drvmapp4((uintptr_t)&ime);
    drvmapp1(0x00);
}

drop(push_af)
{
    // Das ist ganz schön fies, weil wir AF einfach umgekehrt verwendet und
    // zudem ein ganz anderes F-Register haben...
    // push eax; lahf; push eax; movzx eax,ah
    drvmapp4(0x0F509F50);
    // ...; mov al,[flag_table+eax]
    drvmapp4(0x808AC4B6);
    drvmapp4((uintptr_t)&flag_table);
    // mov ah,[esp]; sub bp,2
    drvmapp4(0x6624248A);
    // ...; mov [ebp],ax
    drvmapp4(0x6602ED83);
    // ...; pop eax
    drvmapp4(0x58004589);
    // sahf; pop eax
    drvmapp2(0x589F);
}

static void (*const dynarec_table[256])(void) = {
    dran(0x00, nop),
    dran(0x18, jr_n),
    dran(0x20, jrnz_n),
    dran(0x22, ldi__hl_a),
    dran(0x28, jrz_n),
    dran(0x30, jrnc_n),
    dran(0x36, ld__hl_nn),
    dran(0x38, jrc_n),
    dran(0xAF, xor_a_a),
    dran(0xB1, or_a_c),
    dran(0xC3, jp_nn),
    dran(0xC9, ret),
    dran(0xCD, call_nn),
    dran(0xE0, leave_vm_imm), // ld__ffn_a
    dran(0xEA, ld__nn_a),
    dran(0xF0, leave_vm_imm), // ld_a__ffn
    dran(0xF1, pop_af),
    dran(0xF3, di),
    dran(0xF5, push_af),
};

static const size_t dynarec_const_length[256] = {
    [0x01] = 4 | DYNAREC_LOAD,
    [0x04] = 2 | DYNAREC_CNST,
    [0x05] = 2 | DYNAREC_CNST,
    [0x06] = 2 | DYNAREC_LOAD,
    [0x0B] = 4 | DYNAREC_CNST,
    [0x0C] = 2 | DYNAREC_CNST,
    [0x0D] = 2 | DYNAREC_CNST,
    [0x21] = 4 | DYNAREC_LOAD,
    [0x23] = 4 | DYNAREC_CNST,
    [0x31] = 4 | DYNAREC_LOAD,
    [0x3C] = 2 | DYNAREC_CNST,
    [0x3D] = 2 | DYNAREC_CNST,
    [0x3E] = 2 | DYNAREC_LOAD,
    [0x78] = 2 | DYNAREC_CNST,
    [0xFE] = 2 | DYNAREC_LOAD,
};

static const uint_fast32_t dynarec_const[256] = {
    [0x01] = 0x0000BB66,    // ld bc,nn = mov bx,nn
    [0x04] = 0xC7FE,        // inc b = inc bh
    [0x05] = 0xCFFE,        // dec b = dec bh
    [0x06] = 0x00B7,        // ld b,n = mov bh,n
    [0x0B] = 0x9E4B669F,    // dec bc = lahf; dec bx; sahf
    [0x0C] = 0xC3FE,        // inc c = inc bl
    [0x0D] = 0xCBFE,        // dec c = dec bl
    [0x21] = 0x0000BA66,    // ld hl,nn = mov dx,nn
    [0x23] = 0x9E42669F,    // inc hl = lahf; inc dx; sahf
    [0x31] = 0x0000BD66,    // ld sp,nn = mov bp,nn
    [0x3C] = 0xC0FE,        // inc a = inc al
    [0x3D] = 0xC8FE,        // dec a = dec al
    [0x3E] = 0x00B0,        // ld a,n = mov al,n
    [0x78] = 0xF888,        // ld a,b = mov al,bh
    [0xFE] = 0x003C,        // cp a,n = cmp al,n
};
