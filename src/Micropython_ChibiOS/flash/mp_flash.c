/** @addtogroup flash_file
 *
 */

/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2010 Thomas Otto <tommi@viadmin.org>
 * Copyright (C) 2010 Mark Butler <mbutler@physics.otago.ac.nz>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

/**@{*/
#include "hal.h"
#include "mp_flash.h"

/* Generic memory-mapped I/O accessor functions */
#define MMIO8(addr)			(*(volatile uint8_t *)(addr))
#define MMIO16(addr)		(*(volatile uint16_t *)(addr))
#define MMIO32(addr)		(*(volatile uint32_t *)(addr))
#define MMIO64(addr)		(*(volatile uint64_t *)(addr))


/* --- FLASH_CR values ----------------------------------------------------- */

#define FLASH_CR_SNB_SHIFT			3
#define FLASH_CR_SNB_MASK			0x1f
#define FLASH_CR_PROGRAM_MASK		0x3
#define FLASH_CR_PROGRAM_SHIFT		8

#define FLASH_CR_PROGRAM_X8			0
#define FLASH_CR_PROGRAM_X16		1
#define FLASH_CR_PROGRAM_X32		2
#define FLASH_CR_PROGRAM_X64		3

/* --- FLASH Keys -----------------------------------------------------------*/

#define FLASH_OPTKEYR_KEY1		((uint32_t)0x08192a3b)
#define FLASH_OPTKEYR_KEY2		((uint32_t)0x4c5d6e7f)

/* --- FLASH registers ----------------------------------------------------- */

#define FLASH_ACR			MMIO32(FLASH_R_BASE + 0x00)
#define FLASH_KEYR			MMIO32(FLASH_R_BASE + 0x04)
#define FLASH_SR			MMIO32(FLASH_R_BASE + 0x0C)
#define FLASH_CR			MMIO32(FLASH_R_BASE + 0x10)


/* --- FLASH_CR values ----------------------------------------------------- */

/* --- FLASH Keys -----------------------------------------------------------*/

#define FLASH_KEYR_KEY1			((uint32_t)0x45670123)
#define FLASH_KEYR_KEY2			((uint32_t)0xcdef89ab)

/** @brief Set the Program Parallelism Size

Set the programming word width. Note carefully the power supply voltage
restrictions under which the different word sizes may be used. See the
programming manual for more information.
@param[in] psize The programming word width one of: @ref flash_cr_program_width
*/

static inline void flash_set_program_size(uint32_t psize)
{
	FLASH_CR &= ~(FLASH_CR_PROGRAM_MASK << FLASH_CR_PROGRAM_SHIFT);
	FLASH_CR |= psize << FLASH_CR_PROGRAM_SHIFT;
}

static void flash_wait_for_last_operation(void)
{
	while ((FLASH_SR & FLASH_SR_BSY) == FLASH_SR_BSY);
}

/*---------------------------------------------------------------------------*/
/** @brief Unlock the Flash Program and Erase Controller

This enables write access to the Flash memory. It is locked by default on
reset.
*/

void mp_flash_unlock(void)
{
	/* Clear the unlock sequence state. */
	FLASH_CR |= FLASH_CR_LOCK;

	/* Authorize the FPEC access. */
	FLASH_KEYR = FLASH_KEYR_KEY1;
	FLASH_KEYR = FLASH_KEYR_KEY2;
}

/*---------------------------------------------------------------------------*/
/** @brief Lock the Flash Program and Erase Controller

Used to prevent spurious writes to FLASH.
*/

void mp_flash_lock(void)
{
	FLASH_CR |= FLASH_CR_LOCK;
}

/*---------------------------------------------------------------------------*/
/** @brief Program an 8 bit Byte to FLASH

This performs all operations necessary to program an 8 bit byte to FLASH memory.
The program error flag should be checked separately for the event that memory
was not properly erased.

@param[in] address Starting address in Flash.
@param[in] data byte to write
*/

void mp_flash_program_byte(uint32_t address, uint8_t data)
{
	flash_wait_for_last_operation();
	flash_set_program_size(FLASH_CR_PROGRAM_X8);

	FLASH_CR |= FLASH_CR_PG;

	MMIO8(address) = data;

	flash_wait_for_last_operation();

	FLASH_CR &= ~FLASH_CR_PG;		/* Disable the PG bit. */
}

/*---------------------------------------------------------------------------*/
/** @brief Erase a Sector of FLASH

This performs all operations necessary to erase a sector in FLASH memory.
The page should be checked to ensure that it was properly erased. A sector must
first be fully erased before attempting to program it.

See the reference manual or the FLASH programming manual for details.

@param[in] sector (0 - 11 for some parts, 0-23 on others)
*/

void mp_flash_erase_sector(uint8_t sector)
{
	flash_wait_for_last_operation();
	flash_set_program_size(FLASH_CR_PROGRAM_X32);

	FLASH_CR &= ~(FLASH_CR_SNB_MASK << FLASH_CR_SNB_SHIFT);
	FLASH_CR |= (sector & FLASH_CR_SNB_MASK) << FLASH_CR_SNB_SHIFT;
	FLASH_CR |= FLASH_CR_SER;
	FLASH_CR |= FLASH_CR_STRT;

	flash_wait_for_last_operation();
	FLASH_CR &= ~FLASH_CR_SER;
	FLASH_CR &= ~(FLASH_CR_SNB_MASK << FLASH_CR_SNB_SHIFT);
}

