/*	Copyright  (c)	Günter Woigk 2020 - 2021
					mailto:kio@little-bat.de

	This file is free software.

	Permission to use, copy, modify, distribute, and sell this software
	and its documentation for any purpose is hereby granted without fee,
	provided that the above copyright notice appears in all copies and
	that both that copyright notice, this permission notice and the
	following disclaimer appear in supporting documentation.

	THIS SOFTWARE IS PROVIDED "AS IS", WITHOUT ANY WARRANTY,
	NOT EVEN THE IMPLIED WARRANTY OF MERCHANTABILITY OR FITNESS FOR
	A PARTICULAR PURPOSE, AND IN NO EVENT SHALL THE COPYRIGHT HOLDER
	BE LIABLE FOR ANY DAMAGES ARISING FROM THE USE OF THIS SOFTWARE,
	TO THE EXTENT PERMITTED BY APPLICABLE LAW.
*/

#pragma once
#include "Z80/goodies/z80_goodies.h"
#include "Z80Registers.h"
#include <functional>

namespace zasm
{

class Z80
{
public:
	using CoreByte		= uint8;
	using CpuCycle		= int32; // cpu clock cycle
	using Address		= uint16;
	using Byte			= uint8;
	using Word			= uint16;
	using InputHandler	= std::function<uint8(CpuCycle, uint16)>;
	using OutputHandler = std::function<void(CpuCycle, uint16, uint8)>;

	static const uint8 zlog_flags[256];

	Z80Registers  registers;
	CoreByte*	  core = nullptr;
	InputHandler  input;
	OutputHandler output;

	CpuID cpu_type;
	bool  ixcbr2_enabled = no; // Z80
	bool  ixcbxh_enabled = no; // Z80

	CpuCycle cc			  = 0;
	uint	 int_ack_byte = 255; // RST 0x38
	bool	 halt		  = no;
	bool	 int_off	  = yes; // interrupt was automatically switched off in int ack cycle
	CpuCycle int_start	  = 0;
	CpuCycle int_end	  = 0; // interrupt duration: -1 = no interrupts, 0 =
							   // automatic switch-off mode, else cc
	Address breakpoint = 0;

	enum RVal { TimeOut = 0, BreakPoint, IllegalInstruction, UnsupportedIntAckByte };

	Z80(CpuID, CoreByte[0x10000], InputHandler, OutputHandler);

	~Z80() {}

	void reset() noexcept;
	RVal run(CpuCycle cc_exit);
	RVal runZ180(CpuCycle);

	Byte peek(Address a) const noexcept { return core[a]; }

	void poke(Address a, Byte c) noexcept { core[a] = c; }
};

} // namespace zasm
