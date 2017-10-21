/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - cart_rom.h                                              *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2014 Bobby Smiles                                       *
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

#ifndef M64P_DEVICE_PI_CART_ROM_H
#define M64P_DEVICE_PI_CART_ROM_H

#include <stddef.h>
#include <stdint.h>

struct r4300_core;
struct cic;
struct rdram;

struct cart_rom
{
    uint8_t* rom;
    size_t rom_size;

    uint32_t last_write;
    uint32_t rom_written;

    struct r4300_core* r4300;
    struct rdram* rdram;
    const struct cic* cic;
};

static uint32_t rom_address(uint32_t address)
{
    return (address & 0x03fffffc);
}

void init_cart_rom(struct cart_rom* cart_rom,
                   uint8_t* rom, size_t rom_size,
                   struct r4300_core* r4300,
                   struct rdram* rdram, const struct cic* cic);

void poweron_cart_rom(struct cart_rom* cart_rom);

void read_cart_rom(void* opaque, uint32_t address, uint32_t* value);
void write_cart_rom(void* opaque, uint32_t address, uint32_t value, uint32_t mask);

unsigned int cart_rom_dma_read(void* opaque, const uint8_t* dram, uint32_t dram_addr, uint32_t cart_addr, uint32_t length);
unsigned int cart_rom_dma_write(void* opaque, uint8_t* dram, uint32_t dram_addr, uint32_t cart_addr, uint32_t length);

#endif
