/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - cached_interp.c                                         *
 *   Mupen64Plus homepage: https://mupen64plus.org/                        *
 *   Copyright (C) 2002 Hacktarux                                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "cached_interp.h"

#include <stdint.h>
#include <stdlib.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <string.h>

#include "api/callbacks.h"
#include "api/debugger.h"
#include "api/m64p_types.h"
#include "device/r4300/r4300_core.h"
#include "device/r4300/recomp.h"
#include "main/main.h"
#include "osal/preproc.h"

#ifdef DBG
#include "debugger/dbg_debugger.h"
#endif

// -----------------------------------------------------------
// Cached interpreter functions (and fallback for dynarec).
// -----------------------------------------------------------
#ifdef DBG
#define UPDATE_DEBUGGER() if (g_DebuggerActive) update_debugger(*r4300_pc(r4300))
#else
#define UPDATE_DEBUGGER() do { } while(0)
#endif

#define DECLARE_R4300 struct r4300_core* r4300 = &g_dev.r4300;
#define PCADDR *r4300_pc(r4300)
#define ADD_TO_PC(x) (*r4300_pc_struct(r4300)) += x;
#define DECLARE_INSTRUCTION(name) void cached_interp_##name(void)

#define DECLARE_JUMP(name, destination, condition, link, likely, cop1) \
void cached_interp_##name(void) \
{ \
    DECLARE_R4300 \
    const int take_jump = (condition); \
    const uint32_t jump_target = (destination); \
    int64_t *link_register = (link); \
    if (cop1 && check_cop1_unusable(r4300)) return; \
    if (link_register != &r4300_regs(r4300)[0]) \
    { \
        *link_register = SE32(*r4300_pc(r4300) + 8); \
    } \
    if (!likely || take_jump) \
    { \
        (*r4300_pc_struct(r4300))++; \
        r4300->delay_slot=1; \
        UPDATE_DEBUGGER(); \
        (*r4300_pc_struct(r4300))->ops(); \
        cp0_update_count(r4300); \
        r4300->delay_slot=0; \
        if (take_jump && !r4300->skip_jump) \
        { \
            (*r4300_pc_struct(r4300))=r4300->cached_interp.actual->block+((jump_target-r4300->cached_interp.actual->start)>>2); \
        } \
    } \
    else \
    { \
        (*r4300_pc_struct(r4300)) += 2; \
        cp0_update_count(r4300); \
    } \
    r4300->cp0.last_addr = *r4300_pc(r4300); \
    if (*r4300_cp0_next_interrupt(&r4300->cp0) <= r4300_cp0_regs(&r4300->cp0)[CP0_COUNT_REG]) gen_interrupt(r4300); \
} \
 \
void cached_interp_##name##_OUT(void) \
{ \
    DECLARE_R4300 \
    const int take_jump = (condition); \
    const uint32_t jump_target = (destination); \
    int64_t *link_register = (link); \
    if (cop1 && check_cop1_unusable(r4300)) return; \
    if (link_register != &r4300_regs(r4300)[0]) \
    { \
        *link_register = SE32(*r4300_pc(r4300) + 8); \
    } \
    if (!likely || take_jump) \
    { \
        (*r4300_pc_struct(r4300))++; \
        r4300->delay_slot=1; \
        UPDATE_DEBUGGER(); \
        (*r4300_pc_struct(r4300))->ops(); \
        cp0_update_count(r4300); \
        r4300->delay_slot=0; \
        if (take_jump && !r4300->skip_jump) \
        { \
            generic_jump_to(r4300, jump_target); \
        } \
    } \
    else \
    { \
        (*r4300_pc_struct(r4300)) += 2; \
        cp0_update_count(r4300); \
    } \
    r4300->cp0.last_addr = *r4300_pc(r4300); \
    if (*r4300_cp0_next_interrupt(&r4300->cp0) <= r4300_cp0_regs(&r4300->cp0)[CP0_COUNT_REG]) gen_interrupt(r4300); \
} \
  \
void cached_interp_##name##_IDLE(void) \
{ \
    DECLARE_R4300 \
    uint32_t* cp0_regs = r4300_cp0_regs(&r4300->cp0); \
    const int take_jump = (condition); \
    int skip; \
    if (cop1 && check_cop1_unusable(r4300)) return; \
    if (take_jump) \
    { \
        cp0_update_count(r4300); \
        skip = *r4300_cp0_next_interrupt(&r4300->cp0) - cp0_regs[CP0_COUNT_REG]; \
        if (skip > 3) cp0_regs[CP0_COUNT_REG] += (skip & UINT32_C(0xFFFFFFFC)); \
        else cached_interp_##name(); \
    } \
    else cached_interp_##name(); \
}

/* These macros allow direct access to parsed opcode fields. */
#define rrt *(*r4300_pc_struct(r4300))->f.r.rt
#define rrd *(*r4300_pc_struct(r4300))->f.r.rd
#define rfs (*r4300_pc_struct(r4300))->f.r.nrd
#define rrs *(*r4300_pc_struct(r4300))->f.r.rs
#define rsa (*r4300_pc_struct(r4300))->f.r.sa
#define irt *(*r4300_pc_struct(r4300))->f.i.rt
#define ioffset (*r4300_pc_struct(r4300))->f.i.immediate
#define iimmediate (*r4300_pc_struct(r4300))->f.i.immediate
#define irs *(*r4300_pc_struct(r4300))->f.i.rs
#define ibase *(*r4300_pc_struct(r4300))->f.i.rs
#define jinst_index (*r4300_pc_struct(r4300))->f.j.inst_index
#define lfbase (*r4300_pc_struct(r4300))->f.lf.base
#define lfft (*r4300_pc_struct(r4300))->f.lf.ft
#define lfoffset (*r4300_pc_struct(r4300))->f.lf.offset
#define cfft (*r4300_pc_struct(r4300))->f.cf.ft
#define cffs (*r4300_pc_struct(r4300))->f.cf.fs
#define cffd (*r4300_pc_struct(r4300))->f.cf.fd

/* 32 bits macros */
#ifndef M64P_BIG_ENDIAN
#define rrt32 *((int32_t*) (*r4300_pc_struct(r4300))->f.r.rt)
#define rrd32 *((int32_t*) (*r4300_pc_struct(r4300))->f.r.rd)
#define rrs32 *((int32_t*) (*r4300_pc_struct(r4300))->f.r.rs)
#define irs32 *((int32_t*) (*r4300_pc_struct(r4300))->f.i.rs)
#define irt32 *((int32_t*) (*r4300_pc_struct(r4300))->f.i.rt)
#else
#define rrt32 *((int32_t*) (*r4300_pc_struct(r4300))->f.r.rt + 1)
#define rrd32 *((int32_t*) (*r4300_pc_struct(r4300))->f.r.rd + 1)
#define rrs32 *((int32_t*) (*r4300_pc_struct(r4300))->f.r.rs + 1)
#define irs32 *((int32_t*) (*r4300_pc_struct(r4300))->f.i.rs + 1)
#define irt32 *((int32_t*) (*r4300_pc_struct(r4300))->f.i.rt + 1)
#endif

#include "mips_instructions.def"

// -----------------------------------------------------------
// Flow control 'fake' instructions
// -----------------------------------------------------------
void cached_interp_FIN_BLOCK(void)
{
    DECLARE_R4300
    if (!r4300->delay_slot)
    {
        generic_jump_to(r4300, ((*r4300_pc_struct(r4300))-1)->addr+4);
/*
#ifdef DBG
      if (g_DebuggerActive) update_debugger(*r4300_pc(r4300));
#endif
Used by dynarec only, check should be unnecessary
*/
        (*r4300_pc_struct(r4300))->ops();
    }
    else
    {
        struct precomp_block *blk = r4300->cached_interp.actual;
        struct precomp_instr *inst = (*r4300_pc_struct(r4300));
        generic_jump_to(r4300, ((*r4300_pc_struct(r4300))-1)->addr+4);

/*
#ifdef DBG
          if (g_DebuggerActive) update_debugger(*r4300_pc(r4300));
#endif
Used by dynarec only, check should be unnecessary
*/
        if (!r4300->skip_jump)
        {
            (*r4300_pc_struct(r4300))->ops();
            r4300->cached_interp.actual = blk;
            (*r4300_pc_struct(r4300)) = inst+1;
        }
        else
            (*r4300_pc_struct(r4300))->ops();
    }
}

void cached_interp_NOTCOMPILED(void)
{
    DECLARE_R4300
    uint32_t *mem = fast_mem_access(r4300, r4300->cached_interp.blocks[*r4300_pc(r4300)>>12]->start);
#ifdef DBG
    DebugMessage(M64MSG_INFO, "NOTCOMPILED: addr = %x ops = %lx", *r4300_pc(r4300), (long) (*r4300_pc_struct(r4300))->ops);
#endif

    if (mem == NULL) {
        DebugMessage(M64MSG_ERROR, "not compiled exception");
    }
    else {
        r4300->cached_interp.recompile_block(r4300, mem, r4300->cached_interp.blocks[*r4300_pc(r4300) >> 12], *r4300_pc(r4300));
    }

/*
#ifdef DBG
      if (g_DebuggerActive) update_debugger(*r4300_pc(r4300));
#endif
The preceeding update_debugger SHOULD be unnecessary since it should have been
called before NOTCOMPILED would have been executed
*/
    (*r4300_pc_struct(r4300))->ops();
}

void cached_interp_NOTCOMPILED2(void)
{
    cached_interp_NOTCOMPILED();
}

static uint32_t update_invalid_addr(struct r4300_core* r4300, uint32_t addr)
{
    if (addr >= 0x80000000 && addr < 0xc0000000)
    {
        if (r4300->cached_interp.invalid_code[addr>>12]) {
            r4300->cached_interp.invalid_code[(addr^0x20000000)>>12] = 1;
        }
        if (r4300->cached_interp.invalid_code[(addr^0x20000000)>>12]) {
            r4300->cached_interp.invalid_code[addr>>12] = 1;
        }
        return addr;
    }
    else
    {
        uint32_t paddr = virtual_to_physical_address(r4300, addr, 2);
        if (paddr)
        {
            uint32_t beg_paddr = paddr - (addr - (addr & ~0xfff));

            update_invalid_addr(r4300, paddr);

            if (r4300->cached_interp.invalid_code[(beg_paddr+0x000)>>12]) {
                r4300->cached_interp.invalid_code[addr>>12] = 1;
            }
            if (r4300->cached_interp.invalid_code[(beg_paddr+0xffc)>>12]) {
                r4300->cached_interp.invalid_code[addr>>12] = 1;
            }
            if (r4300->cached_interp.invalid_code[addr>>12]) {
                r4300->cached_interp.invalid_code[(beg_paddr+0x000)>>12] = 1;
            }
            if (r4300->cached_interp.invalid_code[addr>>12]) {
                r4300->cached_interp.invalid_code[(beg_paddr+0xffc)>>12] = 1;
            }
        }
        return paddr;
    }
}

int get_block_length(const struct precomp_block *block)
{
    return (block->end-block->start)/4;
}

size_t get_block_memsize(const struct precomp_block *block)
{
    int length = get_block_length(block);
    return ((length+1)+(length>>2)) * sizeof(struct precomp_instr);
}

void cached_interp_init_block(struct r4300_core* r4300, uint32_t address)
{
    int i, length;

    struct precomp_block** block = &r4300->cached_interp.blocks[address >> 12];

    /* allocate block */
    if (*block == NULL) {
        *block = malloc(sizeof(struct precomp_block));
        (*block)->block = NULL;
        (*block)->start = address & ~UINT32_C(0xfff);
        (*block)->end = (address & ~UINT32_C(0xfff)) + 0x1000;
    }

    struct precomp_block* b = *block;

    length = get_block_length(b);

#ifdef DBG
    DebugMessage(M64MSG_INFO, "init block %" PRIX32 " - %" PRIX32, b->start, b->end);
#endif

    /* allocate block instructions */
    if (!b->block)
    {
        size_t memsize = get_block_memsize(b);
        b->block = (struct precomp_instr*)malloc(memsize);
        if (!b->block) {
            DebugMessage(M64MSG_ERROR, "Memory error: couldn't allocate memory for cached interpreter.");
            return;
        }

        memset(b->block, 0, memsize);
    }

    /* reset block instructions (addr + ops) */
    for (i = 0; i < length; ++i)
    {
        b->block[i].addr = b->start + 4*i;
        b->block[i].ops = cached_interp_NOTCOMPILED;
    }

    /* here we're marking the block as a valid code even if it's not compiled
     * yet as the game should have already set up the code correctly.
     */
    r4300->cached_interp.invalid_code[b->start>>12] = 0;


    if (b->end < UINT32_C(0x80000000) || b->start >= UINT32_C(0xc0000000))
    {
        uint32_t paddr = virtual_to_physical_address(r4300, b->start, 2);

        r4300->cached_interp.invalid_code[paddr>>12] = 0;
        cached_interp_init_block(r4300, paddr);

        paddr += b->end - b->start - 4;

        r4300->cached_interp.invalid_code[paddr>>12] = 0;
        cached_interp_init_block(r4300, paddr);
    }
    else
    {
        uint32_t alt_addr = b->start ^ UINT32_C(0x20000000);

        if (r4300->cached_interp.invalid_code[alt_addr>>12])
        {
            cached_interp_init_block(r4300, alt_addr);
        }
    }
}

void cached_interp_free_block(struct precomp_block* block)
{
    if (block->block) {
        free(block->block);
        block->block = NULL;
    }
}


void cached_interp_recompile_block(struct r4300_core* r4300, const uint32_t* source, struct precomp_block* block, uint32_t func)
{
    int i;
    int length, finished = 0;

    length = (block->end-block->start)/4;
    r4300->cached_interp.dst_block = block;

    block->xxhash = 0;

    for (i = (func & 0xFFF) / 4; finished != 2; i++)
    {
        if (block->start < UINT32_C(0x80000000) || UINT32_C(block->start >= 0xc0000000))
        {
            uint32_t address2 = virtual_to_physical_address(r4300, block->start + i*4, 0);
            if (r4300->cached_interp.blocks[address2>>12]->block[(address2&UINT32_C(0xFFF))/4].ops == cached_interp_NOTCOMPILED) {
                r4300->cached_interp.blocks[address2>>12]->block[(address2&UINT32_C(0xFFF))/4].ops = cached_interp_NOTCOMPILED2;
            }
        }

        r4300->cached_interp.SRC = source + i;
        r4300->cached_interp.src = source[i];
        r4300->cached_interp.check_nop = source[i+1] == 0;
        r4300->cached_interp.dst = block->block + i;
        r4300->cached_interp.dst->addr = block->start + i*4;
        recomp_ops[((r4300->cached_interp.src >> 26) & 0x3F)](r4300);
        r4300->cached_interp.dst = block->block + i;

        if (r4300->cached_interp.delay_slot_compiled)
        {
            r4300->cached_interp.delay_slot_compiled--;
        }

        if (i >= length-2+(length>>2)) { finished = 2; }
        if (i >= (length-1) && (block->start == UINT32_C(0xa4000000) ||
                    block->start >= UINT32_C(0xc0000000) ||
                    block->end   <  UINT32_C(0x80000000))) { finished = 2; }
        if (r4300->cached_interp.dst->ops == cached_interp_ERET || finished == 1) { finished = 2; }
        if (/*i >= length && */
                (r4300->cached_interp.dst->ops == cached_interp_J ||
                 r4300->cached_interp.dst->ops == cached_interp_J_OUT ||
                 r4300->cached_interp.dst->ops == cached_interp_JR ||
                 r4300->cached_interp.dst->ops == cached_interp_JR_OUT) &&
                !(i >= (length-1) && (block->start >= UINT32_C(0xc0000000) ||
                        block->end   <  UINT32_C(0x80000000)))) {
            finished = 1;
        }
    }

    if (i >= length)
    {
        r4300->cached_interp.dst = block->block + i;
        r4300->cached_interp.dst->addr = block->start + i*4;
        r4300->cached_interp.dst->ops = cached_interp_FIN_BLOCK;
        i++;
        if (i < length-1+(length>>2)) // useful when last opcode is a jump
        {
            r4300->cached_interp.dst = block->block + i;
            r4300->cached_interp.dst->addr = block->start + i*4;
            r4300->cached_interp.dst->ops = cached_interp_FIN_BLOCK;
            i++;
        }
    }

#ifdef DBG
    DebugMessage(M64MSG_INFO, "block recompiled (%" PRIX32 "-%" PRIX32 ")", func, block->start+i*4);
#endif
}

void cached_interpreter_jump_to(struct r4300_core* r4300, uint32_t address)
{
    struct cached_interp* const cinterp = &r4300->cached_interp;

    if (r4300->skip_jump) {
        return;
    }

    if (!update_invalid_addr(r4300, address)) {
        return;
    }

    /* setup new block if invalid */
    if (cinterp->invalid_code[address >> 12]) {
        r4300->cached_interp.init_block(r4300, address);
    }

    /* set new PC */
    cinterp->actual = cinterp->blocks[address >> 12];
    (*r4300_pc_struct(r4300)) = cinterp->actual->block + ((address - cinterp->actual->start) >> 2);
}


void init_blocks(struct r4300_core* r4300)
{
    size_t i;
    struct cached_interp* cinterp = &r4300->cached_interp;

    for (i = 0; i < 0x100000; ++i)
    {
        cinterp->invalid_code[i] = 1;
        cinterp->blocks[i] = NULL;
    }
}

void free_blocks(struct r4300_core* r4300)
{
    size_t i;
    struct cached_interp* cinterp = &r4300->cached_interp;

    for (i = 0; i < 0x100000; ++i)
    {
        if (cinterp->blocks[i])
        {
            r4300->cached_interp.free_block(cinterp->blocks[i]);
            free(cinterp->blocks[i]);
            cinterp->blocks[i] = NULL;
        }
    }
}

void invalidate_cached_code_hacktarux(struct r4300_core* r4300, uint32_t address, size_t size)
{
    size_t i;
    uint32_t addr;
    uint32_t addr_max;

    if (size == 0)
    {
        /* invalidate everthing */
        memset(r4300->cached_interp.invalid_code, 1, 0x100000);
    }
    else
    {
        /* invalidate blocks (if necessary) */
        addr_max = address+size;

        for(addr = address; addr < addr_max; addr += 4)
        {
            i = (addr >> 12);

            if (r4300->cached_interp.invalid_code[i] == 0)
            {
                if (r4300->cached_interp.blocks[i] == NULL
                 || r4300->cached_interp.blocks[i]->block[(addr & 0xfff) / 4].ops != r4300->cached_interp.not_compiled)
                {
                    r4300->cached_interp.invalid_code[i] = 1;
                    /* go directly to next i */
                    addr &= ~0xfff;
                    addr |= 0xffc;
                }
            }
            else
            {
                /* go directly to next i */
                addr &= ~0xfff;
                addr |= 0xffc;
            }
        }
    }
}

void run_cached_interpreter(struct r4300_core* r4300)
{
    while (!*r4300_stop(r4300))
    {
#ifdef COMPARE_CORE
        if ((*r4300_pc_struct(r4300))->ops == cached_interp_FIN_BLOCK && ((*r4300_pc_struct(r4300))->addr < 0x80000000 || (*r4300_pc_struct(r4300))->addr >= 0xc0000000))
            virtual_to_physical_address(r4300, (*r4300_pc_struct(r4300))->addr, 2);
        CoreCompareCallback();
#endif
#ifdef DBG
        if (g_DebuggerActive) update_debugger((*r4300_pc_struct(r4300))->addr);
#endif
        (*r4300_pc_struct(r4300))->ops();
    }
}
