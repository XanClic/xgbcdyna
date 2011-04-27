#define DYNAREC_LOAD 1
#define DYNAREC_CNST 0

#define drop(n) static void _dynarec_##n(void)
#define dran(o, n) [o] = &_dynarec_##n

static uint8_t *drc;
static size_t dri;
static uint16_t drip;
static unsigned dr_cycle_counter;

#define drvmapp1(v) vmapp1(drc, dri, v)
#define drvmapp2(v) vmapp2(drc, dri, v)
#define drvmapp4(v) vmapp4(drc, dri, v)

drop(nop)
{
}

drop(jr_n)
{
    drip += (int8_t)MEM8(drip) + 1;
}

drop(jrnz_n)
{
    // jz byte +0x15
    drvmapp2(0x1574);
    exit_vm(drc, &dri, drip + (int8_t)MEM8(drip) + 1, dr_cycle_counter);
    drip++;
}

drop(jrz_n)
{
    // jnz byte +0x15
    drvmapp2(0x1575);
    exit_vm(drc, &dri, drip + (int8_t)MEM8(drip) + 1, dr_cycle_counter);
    drip++;
}

drop(jrnc_n)
{
    // jc byte +0x15
    drvmapp2(0x1572);
    exit_vm(drc, &dri, drip + (int8_t)MEM8(drip) + 1, dr_cycle_counter);
    drip++;
}

drop(jrc_n)
{
    // jnc byte +0x15
    drvmapp2(0x1573);
    exit_vm(drc, &dri, drip + (int8_t)MEM8(drip) + 1, dr_cycle_counter);
    drip++;
}

drop(retnz)
{
    // jz byte +0x1E
    drvmapp2(0x1E74);
    exit_vm_by_ret(drc, &dri, dr_cycle_counter);
}

drop(retz)
{
    // jnz byte +0x1E
    drvmapp2(0x1E75);
    exit_vm_by_ret(drc, &dri, dr_cycle_counter);
}

drop(retnc)
{
    // jc byte +0x1E
    drvmapp2(0x1E72);
    exit_vm_by_ret(drc, &dri, dr_cycle_counter);
}

drop(retc)
{
    // jnc byte +0x1E
    drvmapp2(0x1E73);
    exit_vm_by_ret(drc, &dri, dr_cycle_counter);
}

drop(ldi__hl_a)
{
    // mov [edx],al
    drvmapp2(0x0288);
    // lahf; inc dx; sahf
    drvmapp4(0x9E42669F);
}

drop(ldi_a__hl)
{
    // mov al,[edx]
    drvmapp2(0x028A);
    // lahf; inc dx; sahf
    drvmapp4(0x9E42669F);
}

drop(ldd__hl_a)
{
    // mov [edx],al
    drvmapp2(0x0288);
    // lahf; dec dx; sahf
    drvmapp4(0x9E4A669F);
}

drop(ldd_a__hl)
{
    // mov al,[edx]
    drvmapp2(0x028A);
    // lahf; dec dx; sahf
    drvmapp4(0x9E4A669F);
}

drop(ld__hl_nn)
{
    // mov byte [edx],nn
    drvmapp2(0x02C6);
    drvmapp1(MEM8(drip++));
}

#ifndef UNSAVE_FLAG_OPTIMIZATIONS
drop(and_a_n)
{
    // and al,n
    drvmapp1(0x24);
    drvmapp1(MEM8(drip++));
    // lahf; and ah,0x6A (ZF)
    drvmapp4(0x6AE4809F);
    // or ah,0x10 (AF); sahf
    drvmapp4(0x9E10CC80);
}

#define def_drop_and_a_r(r, and) \
    drop(and_a_##r) \
    { \
        /* and al,r */ \
        drvmapp2(and); \
        /* lahf; and ah,0x6A (ZF) */ \
        drvmapp4(0x6AE4809F); \
        /* or ah,0x10 (AF); sahf */ \
        drvmapp4(0x9E10CC80); \
    }

def_drop_and_a_r(b, 0xF820) // and al,bh
def_drop_and_a_r(c, 0xD820) // and al,bl
def_drop_and_a_r(d, 0xE820) // and al,ch
def_drop_and_a_r(e, 0xC820) // and al,cl
def_drop_and_a_r(h, 0xF020) // and al,dh
def_drop_and_a_r(l, 0xD020) // and al,dl
def_drop_and_a_r(_hl, 0x0222) // and al,[edx]
def_drop_and_a_r(a, 0xC020) // and al,al

drop(xor_a_n)
{
    // xor al,n
    drvmapp1(0x34);
    drvmapp1(MEM8(drip++));
    // lahf
    drvmapp1(0x9F);
    // and ah,0x6A (ZF); sahf
    drvmapp4(0x9E6AE480);
}

#define def_drop_xor_a_r(r, xor) \
    drop(xor_a_##r) \
    { \
        drvmapp2(xor); \
        /* lahf */ \
        drvmapp1(0x9F); \
        /* and ah,0x6A (ZF); sahf */ \
        drvmapp4(0x9E6AE480); \
    }

def_drop_xor_a_r(b, 0xF830) // xor al,bh
def_drop_xor_a_r(c, 0xD830) // xor al,bl
def_drop_xor_a_r(d, 0xE830) // xor al,ch
def_drop_xor_a_r(e, 0xC830) // xor al,cl
def_drop_xor_a_r(h, 0xF030) // xor al,dh
def_drop_xor_a_r(l, 0xD030) // xor al,dl
def_drop_xor_a_r(_hl, 0x0232) // xor al,[edx]
def_drop_xor_a_r(a, 0xC030) // xor al,al

drop(or_a_n)
{
    // or al,n
    drvmapp1(0x0C);
    drvmapp1(MEM8(drip++));
    // lahf; and ah,0x6A (ZF)
    drvmapp4(0x6AE4809F);
    // sahf
    drvmapp1(0x9E);
}

#define def_drop_or_a_r(r, or) \
    drop(or_a_##r) \
    { \
        /* or al,r */ \
        drvmapp2(or); \
        /* lahf */ \
        drvmapp1(0x9F); \
        /* and ah,0x6A (ZF); sahf */ \
        drvmapp4(0x9E6AE480); \
    } \

def_drop_or_a_r(b, 0xF808) // or al,bh
def_drop_or_a_r(c, 0xD808) // or al,bl
def_drop_or_a_r(d, 0xE808) // or al,ch
def_drop_or_a_r(e, 0xC808) // or al,cl
def_drop_or_a_r(h, 0xF008) // or al,dh
def_drop_or_a_r(l, 0xD008) // or al,dl
def_drop_or_a_r(_hl, 0x020A) // or al,[edx]
def_drop_or_a_r(a, 0xC008) // or al,al
#endif

drop(jp_nn)
{
    drip = MEM16(drip);
}

drop(jpnz_nn)
{
    // jz byte +0x15
    drvmapp2(0x1574);
    exit_vm(drc, &dri, MEM16(drip), dr_cycle_counter);
    drip += 2;
}

drop(jpz_nn)
{
    // jnz byte +0x15
    drvmapp2(0x1575);
    exit_vm(drc, &dri, MEM16(drip), dr_cycle_counter);
    drip += 2;
}

drop(jpnc_nn)
{
    // jc byte +0x15
    drvmapp2(0x1572);
    exit_vm(drc, &dri, MEM16(drip), dr_cycle_counter);
    drip += 2;
}

drop(jpc_nn)
{
    // jnc byte +0x15
    drvmapp2(0x1573);
    exit_vm(drc, &dri, MEM16(drip), dr_cycle_counter);
    drip += 2;
}

drop(ret)
{
    exit_vm_by_ret(drc, &dri, dr_cycle_counter);
}

drop(reti)
{
    // mov byte [ime],1
    drvmapp2(0x05C6);
    drvmapp4((uintptr_t)&ime);
    drvmapp1(0x01);
    // Und dann ein normales RET
    exit_vm_by_ret(drc, &dri, dr_cycle_counter);
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

drop(callnz_nn)
{
    // jz byte +0x21
    drvmapp2(0x2174);
    // lahf; sub bp,2
    drvmapp4(0xED83669F);
    // ...; sahf; mov word [ebp],drip+2
    drvmapp4(0xC7669E02);
    drvmapp2(0x0045);
    drvmapp2(drip + 2);
    exit_vm(drc, &dri, MEM16(drip), dr_cycle_counter);
    drip += 2;
}

drop(callz_nn)
{
    // jnz byte +0x21
    drvmapp2(0x2175);
    // lahf; sub bp,2
    drvmapp4(0xED83669F);
    // ...; sahf; mov word [ebp],drip+2
    drvmapp4(0xC7669E02);
    drvmapp2(0x0045);
    drvmapp2(drip + 2);
    exit_vm(drc, &dri, MEM16(drip), dr_cycle_counter);
    drip += 2;
}

drop(callnc_nn)
{
    // jc byte +0x21
    drvmapp2(0x2172);
    // lahf; sub bp,2
    drvmapp4(0xED83669F);
    // ...; sahf; mov word [ebp],drip+2
    drvmapp4(0xC7669E02);
    drvmapp2(0x0045);
    drvmapp2(drip + 2);
    exit_vm(drc, &dri, MEM16(drip), dr_cycle_counter);
    drip += 2;
}

drop(callc_nn)
{
    // jnc byte +0x21
    drvmapp2(0x2173);
    // lahf; sub bp,2
    drvmapp4(0xED83669F);
    // ...; sahf; mov word [ebp],drip+2
    drvmapp4(0xC7669E02);
    drvmapp2(0x0045);
    drvmapp2(drip + 2);
    exit_vm(drc, &dri, MEM16(drip), dr_cycle_counter);
    drip += 2;
}

#define def_drop_rst(num) \
    drop(rst_##num) \
    { \
        /* lahf; sub bp,2 */ \
        drvmapp4(0xED83669F); \
        /* ...; sahf; mov word [ebp],drip */ \
        drvmapp4(0xC7669E02); \
        drvmapp2(0x0045); \
        drvmapp2(drip); \
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
    // mov byte [0x40000000+nn],al
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

drop(pop_bc)
{
    // mov bx,[ebp]
    drvmapp4(0x005D8B66);
    // lahf; add bp,2
    drvmapp4(0xC583669F);
    // ...; sahf
    drvmapp2(0x9E02);
}

drop(pop_de)
{
    // mov cx,[ebp]
    drvmapp4(0x004D8B66);
    // lahf; add bp,2
    drvmapp4(0xC583669F);
    // ...; sahf
    drvmapp2(0x9E02);
}

drop(pop_hl)
{
    // mov dx,[ebp]
    drvmapp4(0x00558B66);
    // lahf; add bp,2
    drvmapp4(0xC583669F);
    // ...; sahf
    drvmapp2(0x9E02);
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

drop(push_bc)
{
    // lahf; sub bp,2
    drvmapp2(0x669F);
    // ...; sahf
    drvmapp4(0x9E02ED83);
    // mov [ebp],bx
    drvmapp4(0x005D8966);
}

drop(push_de)
{
    // lahf; sub bp,2
    drvmapp2(0x669F);
    // ...; sahf
    drvmapp4(0x9E02ED83);
    // mov [ebp],cx
    drvmapp4(0x004D8966);
}

drop(push_hl)
{
    // lahf; sub bp,2
    drvmapp2(0x669F);
    // ...; sahf
    drvmapp4(0x9E02ED83);
    // mov [ebp],dx
    drvmapp4(0x00558966);
}

drop(di)
{
    // mov byte [ime],0
    drvmapp2(0x05C6);
    drvmapp4((uintptr_t)&ime);
    drvmapp1(0x00);
}

drop(ei)
{
    // mov byte [ime],1
    drvmapp2(0x05C6);
    drvmapp4((uintptr_t)&ime);
    drvmapp1(0x01);
}

drop(add_hl_bc)
{
    // Tolle Sache, das ZF muss unverändert bleiben... -.-
    // lahf; push eax; mov al,ah
    drvmapp4(0xE088509F);
    // and al,0x40 (ZF behalten); add dx,bx
    drvmapp4(0x01664024);
    // ...; lahf; and ah,0xBF (ZF löschen)
    drvmapp4(0xE4809FDA);
    // ...; or ah,al; sahf
    drvmapp4(0x9EC408BF);
    // pop eax
    drvmapp1(0x58);
}

drop(add_hl_de)
{
    // lahf; push eax; mov al,ah
    drvmapp4(0xE088509F);
    // and al,0x40 (ZF behalten); add dx,cx
    drvmapp4(0x01664024);
    // ...; lahf; and ah,0xBF (ZF löschen)
    drvmapp4(0xE4809FCA);
    // ...; or ah,al; sahf
    drvmapp4(0x9EC408BF);
    // pop eax
    drvmapp1(0x58);
}
drop(add_hl_hl)
{
    // lahf; push eax; mov al,ah
    drvmapp4(0xE088509F);
    // and al,0x40 (ZF behalten); add dx,dx
    drvmapp4(0x01664024);
    // ...; lahf; and ah,0xBF (ZF löschen)
    drvmapp4(0xE4809FD2);
    // ...; or ah,al; sahf
    drvmapp4(0x9EC408BF);
    // pop eax
    drvmapp1(0x58);
}

drop(add_hl_sp)
{
    // lahf; push eax; mov al,ah
    drvmapp4(0xE088509F);
    // and al,0x40 (ZF behalten); add dx,bp
    drvmapp4(0x01664024);
    // ...; lahf; and ah,0xBF (ZF löschen)
    drvmapp4(0xE4809FEA);
    // ...; or ah,al; sahf
    drvmapp4(0x9EC408BF);
    // pop eax
    drvmapp1(0x58);
}

drop(jp_hl)
{
    // mov [vm_ip],dx
    drvmapp2(0x8966);
    drvmapp1(0x15);
    drvmapp4((uintptr_t)&vm_ip);
    // mov byte [cycles_gone],dr_cycle_counter
    drvmapp2(0x05C6);
    drvmapp4((uintptr_t)&cycles_gone);
    // ...; jmp dword ei
    drvmapp2(dr_cycle_counter | 0xE900);
    drvmapp4((uint32_t)(CODE_EXITI - (dri + 4)));
}

#ifndef UNSAVE_FLAG_OPTIMIZATIONS
drop(rlca) // rotate left NOT through carry (sic)
{
    // rol al,1
    drvmapp2(0xC0D0);
    // lahf
    drvmapp1(0x9F);
    // and ah,0x2B (CF); sahf
    drvmapp4(0x9E2BE480);
}

drop(rla) // rotate left through carry (sic)
{
    // rcl al,1
    drvmapp2(0xD0D0);
    // lahf
    drvmapp1(0x9F);
    // and ah,0x2B (CF); sahf
    drvmapp4(0x9E2BE480);
}

drop(rrca) // rotate right NOT through carry (sic)
{
    // ror al,1
    drvmapp2(0xC8D0);
    // lahf
    drvmapp1(0x9F);
    // and ah,0x2B (CF); sahf
    drvmapp4(0x9E2BE480);
}

drop(rra) // rotate right through carry (sic)
{
    // rcr al,1
    drvmapp2(0xD8D0);
    // lahf
    drvmapp1(0x9F);
    // and ah,0x2B (CF); sahf
    drvmapp4(0x9E2BE480);
}
#endif

drop(ld__nn_sp)
{
    // mov [0x40000000+nn],bp
    drvmapp2(0x8966);
    drvmapp1(0x2D);
    drvmapp4(0x40000000 + MEM16(drip));
    drip += 2;
}

#ifndef UNSAVE_FLAG_OPTIMIZATIONS
drop(cpl_a)
{
    // not al
    drvmapp2(0xD0F6);
    // lahf
    drvmapp1(0x9F);
    // or ah,0x10 (AF); sahf
    drvmapp4(0x9E10CC80);
}

drop(scf)
{
    // lahf; and ah,0x6A (ZF)
    drvmapp4(0x6AE4809F);
    // or ah,0x01 (CF); sahf
    drvmapp4(0x9E01CC80);
}

drop(ccf)
{
    // lahf; and ah,0x6B (ZF)
    drvmapp4(0x6BE4809F);
    // xor ah,0x01 (CF); sahf
    drvmapp4(0x9E01F480);
}
#endif

drop(ld_a__nn)
{
    // mov al,[0x40000000+nn]
    drvmapp1(0xA0);
    drvmapp4(0x40000000 + MEM16(drip));
    drip += 2;
}

static void (*const dynarec_table[256])(void) = {
    dran(0x00, nop),
#ifndef UNSAVE_FLAG_OPTIMIZATIONS
    dran(0x07, rlca),
#endif
    dran(0x08, ld__nn_sp),
    dran(0x09, add_hl_bc),
#ifndef UNSAVE_FLAG_OPTIMIZATIONS
    dran(0x0F, rrca),
    dran(0x17, rla),
#endif
    dran(0x18, jr_n),
    dran(0x19, add_hl_de),
#ifndef UNSAVE_FLAG_OPTIMIZATIONS
    dran(0x1F, rra),
#endif
    dran(0x20, jrnz_n),
    dran(0x22, ldi__hl_a),
    dran(0x28, jrz_n),
    dran(0x29, add_hl_hl),
    dran(0x2A, ldi_a__hl),
#ifndef UNSAVE_FLAG_OPTIMIZATIONS
    dran(0x2F, cpl_a),
#endif
    dran(0x30, jrnc_n),
    dran(0x32, ldd__hl_a),
    dran(0x36, ld__hl_nn),
#ifndef UNSAVE_FLAG_OPTIMIZATIONS
    dran(0x37, scf),
#endif
    dran(0x38, jrc_n),
    dran(0x39, add_hl_sp),
    dran(0x3A, ldd_a__hl),
#ifndef UNSAVE_FLAG_OPTIMIZATIONS
    dran(0x3F, ccf),
#endif
    dran(0x76, leave_vm_imm_op0), // halt
#ifndef UNSAVE_FLAG_OPTIMIZATIONS
    dran(0xA0, and_a_b),
    dran(0xA1, and_a_c),
    dran(0xA2, and_a_d),
    dran(0xA3, and_a_e),
    dran(0xA4, and_a_h),
    dran(0xA5, and_a_l),
    dran(0xA6, and_a__hl),
    dran(0xA7, and_a_a),
    dran(0xA8, xor_a_b),
    dran(0xA9, xor_a_c),
    dran(0xAA, xor_a_d),
    dran(0xAB, xor_a_e),
    dran(0xAC, xor_a_h),
    dran(0xAD, xor_a_l),
    dran(0xAE, xor_a__hl),
    dran(0xAF, xor_a_a),
    dran(0xB0, or_a_b),
    dran(0xB1, or_a_c),
    dran(0xB2, or_a_d),
    dran(0xB3, or_a_e),
    dran(0xB4, or_a_h),
    dran(0xB5, or_a_l),
    dran(0xB6, or_a__hl),
    dran(0xB7, or_a_a),
#endif
    dran(0xC0, retnz),
    dran(0xC1, pop_bc),
    dran(0xC2, jpnz_nn),
    dran(0xC3, jp_nn),
    dran(0xC4, callnz_nn),
    dran(0xC5, push_bc),
    dran(0xC7, rst_0x00),
    dran(0xC8, retz),
    dran(0xC9, ret),
    dran(0xCA, jpz_nn),
    dran(0xCC, callz_nn),
    dran(0xCD, call_nn),
    dran(0xCF, rst_0x08),
    dran(0xD0, retnc),
    dran(0xD1, pop_de),
    dran(0xD2, jpnc_nn),
    dran(0xD4, callnc_nn),
    dran(0xD5, push_de),
    dran(0xD7, rst_0x10),
    dran(0xD8, retc),
    dran(0xD9, reti),
    dran(0xDA, jpc_nn),
    dran(0xDC, callc_nn),
    dran(0xDF, rst_0x18),
    dran(0xE0, leave_vm_imm_op1), // ld__ffn_a
    dran(0xE1, pop_hl),
    dran(0xE2, leave_vm_imm_op0), // ld__ffc_a
    dran(0xE5, push_hl),
#ifndef UNSAVE_FLAG_OPTIMIZATIONS
    dran(0xE6, and_a_n),
#endif
    dran(0xE7, rst_0x20),
    dran(0xE9, jp_hl),
    dran(0xEA, ld__nn_a),
#ifndef UNSAVE_FLAG_OPTIMIZATIONS
    dran(0xEE, xor_a_n),
#endif
    dran(0xEF, rst_0x28),
    dran(0xF0, leave_vm_imm_op1), // ld_a__ffn
    dran(0xF1, pop_af),
    dran(0xF2, leave_vm_imm_op0), // ld_a__ffc
    dran(0xF3, di),
    dran(0xF5, push_af),
#ifndef UNSAVE_FLAG_OPTIMIZATIONS
    dran(0xF6, or_a_n),
#endif
    dran(0xF7, rst_0x30),
    dran(0xFA, ld_a__nn),
    dran(0xFB, ei),
    dran(0xFF, rst_0x38)
};

static const size_t dynarec_const_length[256] = {
    [0x01] = 4 | DYNAREC_LOAD,
    [0x02] = 2 | DYNAREC_CNST,
    [0x03] = 4 | DYNAREC_CNST,
    [0x04] = 2 | DYNAREC_CNST,
    [0x05] = 2 | DYNAREC_CNST,
    [0x06] = 2 | DYNAREC_LOAD,
#ifdef UNSAVE_FLAG_OPTIMIZATIONS
    [0x07] = 2 | DYNAREC_CNST,
#endif
    [0x0A] = 2 | DYNAREC_CNST,
    [0x0B] = 4 | DYNAREC_CNST,
    [0x0C] = 2 | DYNAREC_CNST,
    [0x0D] = 2 | DYNAREC_CNST,
    [0x0E] = 2 | DYNAREC_LOAD,
#ifdef UNSAVE_FLAG_OPTIMIZATIONS
    [0x0F] = 2 | DYNAREC_CNST,
#endif
    [0x11] = 4 | DYNAREC_LOAD,
    [0x12] = 2 | DYNAREC_CNST,
    [0x13] = 4 | DYNAREC_CNST,
    [0x14] = 2 | DYNAREC_CNST,
    [0x15] = 2 | DYNAREC_CNST,
    [0x16] = 2 | DYNAREC_LOAD,
#ifdef UNSAVE_FLAG_OPTIMIZATIONS
    [0x17] = 2 | DYNAREC_CNST,
#endif
    [0x1A] = 2 | DYNAREC_CNST,
    [0x1B] = 4 | DYNAREC_CNST,
    [0x1C] = 2 | DYNAREC_CNST,
    [0x1D] = 2 | DYNAREC_CNST,
    [0x1E] = 2 | DYNAREC_LOAD,
#ifdef UNSAVE_FLAG_OPTIMIZATIONS
    [0x1F] = 2 | DYNAREC_CNST,
#endif
    [0x21] = 4 | DYNAREC_LOAD,
    [0x23] = 4 | DYNAREC_CNST,
    [0x24] = 2 | DYNAREC_CNST,
    [0x25] = 2 | DYNAREC_CNST,
    [0x26] = 2 | DYNAREC_LOAD,
    [0x2B] = 4 | DYNAREC_CNST,
    [0x2C] = 2 | DYNAREC_CNST,
    [0x2D] = 2 | DYNAREC_CNST,
    [0x2E] = 2 | DYNAREC_LOAD,
#ifdef UNSAVE_FLAG_OPTIMIZATIONS
    [0x2F] = 2 | DYNAREC_CNST,
#endif
    [0x31] = 4 | DYNAREC_LOAD,
    [0x33] = 4 | DYNAREC_CNST,
    [0x34] = 2 | DYNAREC_CNST,
    [0x35] = 2 | DYNAREC_CNST,
#ifdef UNSAVE_FLAG_OPTIMIZATIONS
    [0x37] = 2 | DYNAREC_CNST,
#endif
    [0x3B] = 4 | DYNAREC_CNST,
    [0x3C] = 2 | DYNAREC_CNST,
    [0x3D] = 2 | DYNAREC_CNST,
    [0x3E] = 2 | DYNAREC_LOAD,
#ifdef UNSAVE_FLAG_OPTIMIZATIONS
    [0x3F] = 2 | DYNAREC_CNST,
#endif
    [0x40] = 2 | DYNAREC_CNST,
    [0x41] = 2 | DYNAREC_CNST,
    [0x42] = 2 | DYNAREC_CNST,
    [0x43] = 2 | DYNAREC_CNST,
    [0x44] = 2 | DYNAREC_CNST,
    [0x45] = 2 | DYNAREC_CNST,
    [0x46] = 2 | DYNAREC_CNST,
    [0x47] = 2 | DYNAREC_CNST,
    [0x48] = 2 | DYNAREC_CNST,
    [0x49] = 2 | DYNAREC_CNST,
    [0x4A] = 2 | DYNAREC_CNST,
    [0x4B] = 2 | DYNAREC_CNST,
    [0x4C] = 2 | DYNAREC_CNST,
    [0x4D] = 2 | DYNAREC_CNST,
    [0x4E] = 2 | DYNAREC_CNST,
    [0x4F] = 2 | DYNAREC_CNST,
    [0x50] = 2 | DYNAREC_CNST,
    [0x51] = 2 | DYNAREC_CNST,
    [0x52] = 2 | DYNAREC_CNST,
    [0x53] = 2 | DYNAREC_CNST,
    [0x54] = 2 | DYNAREC_CNST,
    [0x55] = 2 | DYNAREC_CNST,
    [0x56] = 2 | DYNAREC_CNST,
    [0x57] = 2 | DYNAREC_CNST,
    [0x58] = 2 | DYNAREC_CNST,
    [0x59] = 2 | DYNAREC_CNST,
    [0x5A] = 2 | DYNAREC_CNST,
    [0x5B] = 2 | DYNAREC_CNST,
    [0x5C] = 2 | DYNAREC_CNST,
    [0x5D] = 2 | DYNAREC_CNST,
    [0x5E] = 2 | DYNAREC_CNST,
    [0x5F] = 2 | DYNAREC_CNST,
    [0x60] = 2 | DYNAREC_CNST,
    [0x61] = 2 | DYNAREC_CNST,
    [0x62] = 2 | DYNAREC_CNST,
    [0x63] = 2 | DYNAREC_CNST,
    [0x64] = 2 | DYNAREC_CNST,
    [0x65] = 2 | DYNAREC_CNST,
    [0x66] = 2 | DYNAREC_CNST,
    [0x67] = 2 | DYNAREC_CNST,
    [0x68] = 2 | DYNAREC_CNST,
    [0x69] = 2 | DYNAREC_CNST,
    [0x6A] = 2 | DYNAREC_CNST,
    [0x6B] = 2 | DYNAREC_CNST,
    [0x6C] = 2 | DYNAREC_CNST,
    [0x6D] = 2 | DYNAREC_CNST,
    [0x6E] = 2 | DYNAREC_CNST,
    [0x6F] = 2 | DYNAREC_CNST,
    [0x70] = 2 | DYNAREC_CNST,
    [0x71] = 2 | DYNAREC_CNST,
    [0x72] = 2 | DYNAREC_CNST,
    [0x73] = 2 | DYNAREC_CNST,
    [0x74] = 2 | DYNAREC_CNST,
    [0x75] = 2 | DYNAREC_CNST,
    [0x77] = 2 | DYNAREC_CNST,
    [0x78] = 2 | DYNAREC_CNST,
    [0x79] = 2 | DYNAREC_CNST,
    [0x7A] = 2 | DYNAREC_CNST,
    [0x7B] = 2 | DYNAREC_CNST,
    [0x7C] = 2 | DYNAREC_CNST,
    [0x7D] = 2 | DYNAREC_CNST,
    [0x7E] = 2 | DYNAREC_CNST,
    [0x7F] = 2 | DYNAREC_CNST,
    [0x80] = 2 | DYNAREC_CNST,
    [0x81] = 2 | DYNAREC_CNST,
    [0x82] = 2 | DYNAREC_CNST,
    [0x83] = 2 | DYNAREC_CNST,
    [0x84] = 2 | DYNAREC_CNST,
    [0x85] = 2 | DYNAREC_CNST,
    [0x86] = 2 | DYNAREC_CNST,
    [0x87] = 2 | DYNAREC_CNST,
    [0x88] = 2 | DYNAREC_CNST,
    [0x89] = 2 | DYNAREC_CNST,
    [0x8A] = 2 | DYNAREC_CNST,
    [0x8B] = 2 | DYNAREC_CNST,
    [0x8C] = 2 | DYNAREC_CNST,
    [0x8D] = 2 | DYNAREC_CNST,
    [0x8E] = 2 | DYNAREC_CNST,
    [0x8F] = 2 | DYNAREC_CNST,
    [0x90] = 2 | DYNAREC_CNST,
    [0x91] = 2 | DYNAREC_CNST,
    [0x92] = 2 | DYNAREC_CNST,
    [0x93] = 2 | DYNAREC_CNST,
    [0x94] = 2 | DYNAREC_CNST,
    [0x95] = 2 | DYNAREC_CNST,
    [0x96] = 2 | DYNAREC_CNST,
    [0x97] = 2 | DYNAREC_CNST,
    [0x98] = 2 | DYNAREC_CNST,
    [0x99] = 2 | DYNAREC_CNST,
    [0x9A] = 2 | DYNAREC_CNST,
    [0x9B] = 2 | DYNAREC_CNST,
    [0x9C] = 2 | DYNAREC_CNST,
    [0x9D] = 2 | DYNAREC_CNST,
    [0x9E] = 2 | DYNAREC_CNST,
    [0x9F] = 2 | DYNAREC_CNST,
#ifdef UNSAVE_FLAG_OPTIMIZATIONS
    [0xA0] = 2 | DYNAREC_CNST,
    [0xA1] = 2 | DYNAREC_CNST,
    [0xA2] = 2 | DYNAREC_CNST,
    [0xA3] = 2 | DYNAREC_CNST,
    [0xA4] = 2 | DYNAREC_CNST,
    [0xA5] = 2 | DYNAREC_CNST,
    [0xA6] = 2 | DYNAREC_CNST,
    [0xA7] = 2 | DYNAREC_CNST,
    [0xA8] = 2 | DYNAREC_CNST,
    [0xA9] = 2 | DYNAREC_CNST,
    [0xAA] = 2 | DYNAREC_CNST,
    [0xAB] = 2 | DYNAREC_CNST,
    [0xAC] = 2 | DYNAREC_CNST,
    [0xAD] = 2 | DYNAREC_CNST,
    [0xAE] = 2 | DYNAREC_CNST,
    [0xAF] = 2 | DYNAREC_CNST,
    [0xB0] = 2 | DYNAREC_CNST,
    [0xB1] = 2 | DYNAREC_CNST,
    [0xB2] = 2 | DYNAREC_CNST,
    [0xB3] = 2 | DYNAREC_CNST,
    [0xB4] = 2 | DYNAREC_CNST,
    [0xB5] = 2 | DYNAREC_CNST,
    [0xB6] = 2 | DYNAREC_CNST,
    [0xB7] = 2 | DYNAREC_CNST,
#endif
    [0xB8] = 2 | DYNAREC_CNST,
    [0xB9] = 2 | DYNAREC_CNST,
    [0xBA] = 2 | DYNAREC_CNST,
    [0xBB] = 2 | DYNAREC_CNST,
    [0xBC] = 2 | DYNAREC_CNST,
    [0xBD] = 2 | DYNAREC_CNST,
    [0xBE] = 2 | DYNAREC_CNST,
    [0xBF] = 2 | DYNAREC_CNST,
    [0xC6] = 2 | DYNAREC_LOAD,
    [0xCE] = 2 | DYNAREC_LOAD,
    [0xD6] = 2 | DYNAREC_LOAD,
    [0xDE] = 2 | DYNAREC_LOAD,
#ifdef UNSAVE_FLAG_OPTIMIZATIONS
    [0xE6] = 2 | DYNAREC_LOAD,
    [0xEE] = 2 | DYNAREC_LOAD,
    [0xF6] = 2 | DYNAREC_LOAD,
#endif
    [0xF9] = 2 | DYNAREC_CNST,
    [0xFE] = 2 | DYNAREC_LOAD
};

static const uint_fast32_t dynarec_const[256] = {
    [0x01] = 0x0000BB66,    // ld bc,nn = mov bx,nn
    [0x02] = 0x0388,        // ld (bc),a = mov [ebx],al
    [0x03] = 0x9E43669F,    // inc bc = lahf; inc bx; sahf
    [0x04] = 0xC7FE,        // inc b = inc bh
    [0x05] = 0xCFFE,        // dec b = dec bh
    [0x06] = 0x00B7,        // ld b,n = mov bh,n
#ifdef UNSAVE_FLAG_OPTIMIZATIONS
    [0x07] = 0xC0D0,        // rlca = rol al,1
#endif
    [0x0A] = 0x038A,        // ld a,(bc) = mov al,[ebx]
    [0x0B] = 0x9E4B669F,    // dec bc = lahf; dec bx; sahf
    [0x0C] = 0xC3FE,        // inc c = inc bl
    [0x0D] = 0xCBFE,        // dec c = dec bl
    [0x0E] = 0x00B3,        // ld c,n = mov bl,n
#ifdef UNSAVE_FLAG_OPTIMIZATIONS
    [0x0F] = 0xC8D0,        // rrca = ror al,1
#endif
    [0x11] = 0x0000B966,    // ld de,nn = mov cx,nn
    [0x12] = 0x0188,        // ld (de),a = mov [ecx],al
    [0x13] = 0x9E41669F,    // inc de = lahf; inc cx; sahf
    [0x14] = 0xC5FE,        // inc d = inc ch
    [0x15] = 0xCDFE,        // dec d = dec ch
    [0x16] = 0x00B5,        // ld d,n = mov ch,n
#ifdef UNSAVE_FLAG_OPTIMIZATIONS
    [0x17] = 0xD0D0,        // rla = rcl al,1
#endif
    [0x1A] = 0x018A,        // ld a,(de) = mov al,[ecx]
    [0x1B] = 0x9E49669F,    // dec de = lahf; dec cx; sahf
    [0x1C] = 0xC1FE,        // inc e = inc cl
    [0x1D] = 0xC9FE,        // dec e = dec cl
    [0x1E] = 0x00B1,        // ld e,n = mov cl,n
#ifdef UNSAVE_FLAG_OPTIMIZATIONS
    [0x1F] = 0xD8D0,        // rra = rcr al,1
#endif
    [0x21] = 0x0000BA66,    // ld hl,nn = mov dx,nn
    [0x23] = 0x9E42669F,    // inc hl = lahf; inc dx; sahf
    [0x24] = 0xC6FE,        // inc h = inc dh
    [0x25] = 0xCEFE,        // dec h = dec dh
    [0x26] = 0x00B6,        // ld h,n = mov dh,n
    [0x2B] = 0x9E4A669F,    // dec hl = lahf; dec dx; sahf
    [0x2C] = 0xC2FE,        // inc l = inc dl
    [0x2D] = 0xCAFE,        // dec l = dec dl
    [0x2E] = 0x00B2,        // ld l,n = mov dl,n
#ifdef UNSAVE_FLAG_OPTIMIZATIONS
    [0x2F] = 0xD0F6,        // cpl a = not al
#endif
    [0x31] = 0x0000BD66,    // ld sp,nn = mov bp,nn
    [0x33] = 0x9E45669F,    // inc sp = lahf; inc bp; sahf
    [0x34] = 0x02FE,        // inc (hl) = inc byte [edx]
    [0x35] = 0x0AFE,        // dec (hl) = dec byte [edx]
#ifdef UNSAVE_FLAG_OPTIMIZATIONS
    [0x37] = 0x90F9,        // scf = stc; nop
#endif
    [0x3B] = 0x9E4D669F,    // dec sp = lahf; dec bp; sahf
    [0x3C] = 0xC0FE,        // inc a = inc al
    [0x3D] = 0xC8FE,        // dec a = dec al
    [0x3E] = 0x00B0,        // ld a,n = mov al,n
#ifdef UNSAVE_FLAG_OPTIMIZATIONS
    [0x3F] = 0x90F5,        // ccf = cmc; nop
#endif
    [0x40] = 0xFF88,        // ld b,b = mov bh,bh
    [0x41] = 0xDF88,        // ld b,c = mov bh,bl
    [0x42] = 0xEF88,        // ld b,d = mov bh,ch
    [0x43] = 0xCF88,        // ld b,e = mov bh,cl
    [0x44] = 0xF788,        // ld b,h = mov bh,dh
    [0x45] = 0xD788,        // ld b,l = mov bh,dl
    [0x46] = 0x3A8A,        // ld b,(hl) = mov bh,[edx]
    [0x47] = 0xC788,        // ld b,a = mov bh,al
    [0x48] = 0xFB88,        // ld c,b = mov bl,bh
    [0x49] = 0xDB88,        // ld c,c = mov bl,bl
    [0x4A] = 0xEB88,        // ld c,d = mov bl,ch
    [0x4B] = 0xCB88,        // ld c,e = mov bl,cl
    [0x4C] = 0xF388,        // ld c,h = mov bl,dh
    [0x4D] = 0xD388,        // ld c,l = mov bl,dl
    [0x4E] = 0x1A8A,        // ld c,(hl) = mov bl,[edx]
    [0x4F] = 0xC388,        // ld c,a = mov bl,al
    [0x50] = 0xFD88,        // ld d,b = mov ch,bh
    [0x51] = 0xDD88,        // ld d,c = mov ch,bl
    [0x52] = 0xED88,        // ld d,d = mov ch,ch
    [0x53] = 0xCD88,        // ld d,e = mov ch,cl
    [0x54] = 0xF588,        // ld d,h = mov ch,dh
    [0x55] = 0xD588,        // ld d,l = mov ch,dl
    [0x56] = 0x2A8A,        // ld d,(hl) = mov ch,[edx]
    [0x57] = 0xC588,        // ld d,a = mov ch,al
    [0x58] = 0xF988,        // ld e,b = mov cl,bh
    [0x59] = 0xD988,        // ld e,c = mov cl,bl
    [0x5A] = 0xE988,        // ld e,d = mov cl,ch
    [0x5B] = 0xC988,        // ld e,e = mov cl,cl
    [0x5C] = 0xF188,        // ld e,h = mov cl,dh
    [0x5D] = 0xD188,        // ld e,l = mov cl,dl
    [0x5E] = 0x0A8A,        // ld e,(hl) = mov cl,[edx]
    [0x5F] = 0xC188,        // ld e,a = mov cl,al
    [0x60] = 0xFE88,        // ld h,b = mov dh,bh
    [0x61] = 0xDE88,        // ld h,c = mov dh,bl
    [0x62] = 0xEE88,        // ld h,d = mov dh,ch
    [0x63] = 0xCE88,        // ld h,e = mov dh,cl
    [0x64] = 0xF688,        // ld h,h = mov dh,dh
    [0x65] = 0xD688,        // ld h,l = mov dh,dl
    [0x66] = 0x328A,        // ld h,(hl) = mov dh,[edx]
    [0x67] = 0xC688,        // ld h,a = mov dh,al
    [0x68] = 0xFA88,        // ld l,b = mov dl,bh
    [0x69] = 0xDA88,        // ld l,c = mov dl,bl
    [0x6A] = 0xEA88,        // ld l,d = mov dl,ch
    [0x6B] = 0xCA88,        // ld l,e = mov dl,cl
    [0x6C] = 0xF288,        // ld l,h = mov dl,dh
    [0x6D] = 0xD288,        // ld l,l = mov dl,dl
    [0x6E] = 0x128A,        // ld l,(hl) = mov dl,[edx]
    [0x6F] = 0xC288,        // ld l,a = mov dl,al
    [0x70] = 0x3A88,        // ld (hl),b = mov [edx],bh
    [0x71] = 0x1A88,        // ld (hl),c = mov [edx],bl
    [0x72] = 0x2A88,        // ld (hl),d = mov [edx],ch
    [0x73] = 0x0A88,        // ld (hl),e = mov [edx],cl
    [0x74] = 0x3288,        // ld (hl),h = mov [edx],dh
    [0x75] = 0x1288,        // ld (hl),l = mov [edx],dl
    [0x77] = 0x0288,        // ld (Hl),a = mov [edx],al
    [0x78] = 0xF888,        // ld a,b = mov al,bh
    [0x79] = 0xD888,        // ld a,c = mov al,bl
    [0x7A] = 0xE888,        // ld a,d = mov al,ch
    [0x7B] = 0xC888,        // ld a,e = mov al,cl
    [0x7C] = 0xF088,        // ld a,h = mov al,dh
    [0x7D] = 0xD088,        // ld a,l = mov al,dl
    [0x7E] = 0x028A,        // ld a,(hl) = mov al,[edx]
    [0x7F] = 0xC088,        // ld a,a = mov al,al
    [0x80] = 0xF800,        // add a,b = add al,bh
    [0x81] = 0xD800,        // add a,c = add al,bl
    [0x82] = 0xE800,        // add a,d = add al,ch
    [0x83] = 0xC800,        // add a,e = add al,cl
    [0x84] = 0xF000,        // add a,h = add al,dh
    [0x85] = 0xD000,        // add a,l = add al,dl
    [0x86] = 0x0202,        // add a,(hl) = add al,[edx]
    [0x87] = 0xC000,        // add a,a = add al,al
    [0x88] = 0xF810,        // adc a,b = adc al,bh
    [0x89] = 0xD810,        // adc a,c = adc al,bl
    [0x8A] = 0xE810,        // adc a,d = adc al,ch
    [0x8B] = 0xC810,        // adc a,e = adc al,cl
    [0x8C] = 0xF010,        // adc a,h = adc al,dh
    [0x8D] = 0xD010,        // adc a,l = adc al,dl
    [0x8E] = 0x0212,        // adc a,(hl) = add al,[edx]
    [0x8F] = 0xC010,        // adc a,a = adc al,al
    [0x90] = 0xF828,        // sub a,b = sub al,bh
    [0x91] = 0xD828,        // sub a,c = sub al,bl
    [0x92] = 0xE828,        // sub a,d = sub al,ch
    [0x93] = 0xC828,        // sub a,e = sub al,cl
    [0x94] = 0xF028,        // sub a,h = sub al,dh
    [0x95] = 0xD028,        // sub a,l = sub al,dl
    [0x96] = 0x022A,        // sub a,(hl) = sub al,[edx]
    [0x97] = 0xC028,        // sub a,a = sub al,al
    [0x98] = 0xF818,        // sbc a,b = sbb al,bh
    [0x99] = 0xD818,        // sbc a,c = sbb al,bl
    [0x9A] = 0xE818,        // sbc a,d = sbb al,ch
    [0x9B] = 0xC818,        // sbc a,e = sbb al,cl
    [0x9C] = 0xF018,        // sbc a,h = sbb al,dh
    [0x9D] = 0xD018,        // sbc a,l = sbb al,dl
    [0x9E] = 0x021A,        // sbc a,(hl) = sbb al,[edx]
    [0x9F] = 0xC018,        // sbc a,a = sbb al,al
#ifdef UNSAVE_FLAG_OPTIMIZATIONS
    [0xA0] = 0xF820,        // and a,b = and al,bh
    [0xA1] = 0xD820,        // and a,c = and al,bl
    [0xA2] = 0xE820,        // and a,d = and al,ch
    [0xA3] = 0xC820,        // and a,e = and al,cl
    [0xA4] = 0xF020,        // and a,h = and al,dh
    [0xA5] = 0xD020,        // and a,l = and al,dl
    [0xA6] = 0x0222,        // and a,(hl) = and al,[edx]
    [0xA7] = 0xC020,        // and a,a = and al,al
    [0xA8] = 0xF830,        // xor a,b = xor al,bh
    [0xA9] = 0xD830,        // xor a,c = xor al,bl
    [0xAA] = 0xE830,        // xor a,d = xor al,ch
    [0xAB] = 0xC830,        // xor a,e = xor al,cl
    [0xAC] = 0xF030,        // xor a,h = xor al,dh
    [0xAD] = 0xD030,        // xor a,l = xor al,dl
    [0xAE] = 0x0232,        // xor a,(hl) = xor al,[edx]
    [0xAF] = 0xC030,        // xor a,a = xor al,al
    [0xB0] = 0xF808,        // or a,b = or al,bh
    [0xB1] = 0xD808,        // or a,c = or al,bl
    [0xB2] = 0xE808,        // or a,d = or al,ch
    [0xB3] = 0xC808,        // or a,e = or al,cl
    [0xB4] = 0xF008,        // or a,h = or al,dh
    [0xB5] = 0xD008,        // or a,l = or al,dl
    [0xB6] = 0x020A,        // or a,(hl) = or al,[edx]
    [0xB7] = 0xC008,        // or a,a = or al,al
#endif
    [0xB8] = 0xF838,        // cp a,b = cmp al,bh
    [0xB9] = 0xD838,        // cp a,c = cmp al,bl
    [0xBA] = 0xE838,        // cp a,d = cmp al,ch
    [0xBB] = 0xC838,        // cp a,e = cmp al,cl
    [0xBC] = 0xF038,        // cp a,h = cmp al,dh
    [0xBD] = 0xD038,        // cp a,l = cmp al,dl
    [0xBE] = 0x023A,        // cp a,(hl) = cmp al,[edx]
    [0xBF] = 0xC038,        // cp a,a = cmp al,al
    [0xC6] = 0x0004,        // add a,n = add al,n
    [0xCE] = 0x0014,        // adc a,n = adc al,n
    [0xD6] = 0x002C,        // sub a,n = sub al,n
    [0xDE] = 0x001C,        // sbc a,n = sbb al,n
#ifdef UNSAVE_FLAG_OPTIMIZATIONS
    [0xE6] = 0x0024,        // and a,n = and al,n
    [0xEE] = 0x0034,        // xor a,n = xor al,n
    [0xF6] = 0x000C,        // or a,n = or al,n
#endif
    [0xF9] = 0xD589,        // ld sp,hl = mov ebp,edx
    [0xFE] = 0x003C         // cp a,n = cmp al,n
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

def_drop_sXX_r(la, b, 0xE7D0) // shl bh,1
def_drop_sXX_r(la, c, 0xE3D0) // shl bl,1
def_drop_sXX_r(la, d, 0xE5D0) // shl ch,1
def_drop_sXX_r(la, e, 0xE1D0) // shl cl,1
def_drop_sXX_r(la, h, 0xE6D0) // shl dh,1
def_drop_sXX_r(la, l, 0xE2D0) // shl dl,1
def_drop_sXX_r(la, a, 0xE0D0) // shl al,1

def_drop_sXX_r(ra, b, 0xFFD0) // sar bh,1
def_drop_sXX_r(ra, c, 0xFBD0) // sar bl,1
def_drop_sXX_r(ra, d, 0xFDD0) // sar ch,1
def_drop_sXX_r(ra, e, 0xF9D0) // sar cl,1
def_drop_sXX_r(ra, h, 0xFED0) // sar dh,1
def_drop_sXX_r(ra, l, 0xFAD0) // sar dl,1
def_drop_sXX_r(ra, a, 0xF8D0) // sar al,1

def_drop_sXX_r(rl, b, 0xEFD0) // shr bh,1
def_drop_sXX_r(rl, c, 0xEBD0) // shr bl,1
def_drop_sXX_r(rl, d, 0xEDD0) // shr ch,1
def_drop_sXX_r(rl, e, 0xE9D0) // shr cl,1
def_drop_sXX_r(rl, h, 0xEED0) // shr dh,1
def_drop_sXX_r(rl, l, 0xEAD0) // shr dl,1
def_drop_sXX_r(rl, a, 0xE8D0) // shr al,1
#endif

static void (*const dynarec_table_CB[64])(void) = {
#ifndef UNSAVE_FLAG_OPTIMIZATIONS
    dran(0x10, rl_b),
    dran(0x11, rl_c),
    dran(0x12, rl_d),
    dran(0x13, rl_e),
    dran(0x14, rl_h),
    dran(0x15, rl_l),
    dran(0x17, rl_a),
    dran(0x18, rr_b),
    dran(0x19, rr_c),
    dran(0x1A, rr_d),
    dran(0x1B, rr_e),
    dran(0x1C, rr_h),
    dran(0x1D, rr_l),
    dran(0x1F, rr_a),
    dran(0x20, sla_b),
    dran(0x21, sla_c),
    dran(0x22, sla_d),
    dran(0x23, sla_e),
    dran(0x24, sla_h),
    dran(0x25, sla_l),
    dran(0x27, sla_a),
    dran(0x28, sra_b),
    dran(0x29, sra_c),
    dran(0x2A, sra_d),
    dran(0x2B, sra_e),
    dran(0x2C, sra_h),
    dran(0x2D, sra_l),
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
    dran(0x3F, srl_a)
#endif
};

#ifdef UNSAVE_FLAG_OPTIMIZATIONS
static uint16_t dynarec_const_CB[64] = {
    [0x10] = 0xD7D0,        // rl b = rcl bh,1
    [0x11] = 0xD3D0,        // rl c = rcl bl,1
    [0x12] = 0xD5D0,        // rl d = rcl ch,1
    [0x13] = 0xD1D0,        // rl e = rcl cl,1
    [0x14] = 0xD6D0,        // rl h = rcl dh,1
    [0x15] = 0xD2D0,        // rl l = rcl dl,1
    [0x17] = 0xD0D0,        // rl a = rcl al,1
    [0x18] = 0xDFD0,        // rr b = rcr bh,1
    [0x19] = 0xDBD0,        // rr c = rcr bl,1
    [0x1A] = 0xDDD0,        // rr d = rcr ch,1
    [0x1B] = 0xD9D0,        // rr e = rcr cl,1
    [0x1C] = 0xDED0,        // rr h = rcr dh,1
    [0x1D] = 0xDAD0,        // rr l = rcr dl,1
    [0x1F] = 0xD8D0,        // rr a = rcr al,1
    [0x20] = 0xE7D0,        // sla b = shl bh,1
    [0x21] = 0xE3D0,        // sla c = shl bl,1
    [0x22] = 0xE5D0,        // sla d = shl ch,1
    [0x23] = 0xE1D0,        // sla e = shl cl,1
    [0x24] = 0xE6D0,        // sla h = shl dh,1
    [0x25] = 0xE2D0,        // sla l = shl dl,1
    [0x27] = 0xE0D0,        // sla a = shl al,1
    [0x28] = 0xFFD0,        // sra b = sar bh,1
    [0x29] = 0xFBD0,        // sra c = sar bl,1
    [0x2A] = 0xFDD0,        // sra d = sar ch,1
    [0x2B] = 0xF9D0,        // sra e = sar cl,1
    [0x2C] = 0xFED0,        // sra h = sar dh,1
    [0x2D] = 0xFAD0,        // sra l = sar dl,1
    [0x2F] = 0xF8D0,        // sra a = sar al,1
    [0x38] = 0xEFD0,        // srl b = shr bh,1
    [0x39] = 0xEBD0,        // srl c = shr bl,1
    [0x3A] = 0xEDD0,        // srl d = shr ch,1
    [0x3B] = 0xE9D0,        // srl e = shr cl,1
    [0x3C] = 0xEED0,        // srl h = shr dh,1
    [0x3D] = 0xEAD0,        // srl l = shr dl,1
    [0x3F] = 0xE8D0,        // srl a = shr al,1
};
#endif
