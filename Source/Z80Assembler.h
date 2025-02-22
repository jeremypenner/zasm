#pragma once
/*	Copyright  (c)	Günter Woigk 1994 - 2021
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

#include "Error.h"
#include "Label.h"
#include "Macro.h"
#include "Segment.h"
#include "Source.h"
#include "SyntaxError.h"
#include "Templates/Array.h"
#include "Z80/goodies/z80_goodies.h"
#include "kio/kio.h"


// hints may be set by caller:
//
extern cstr sdcc_compiler_path;
extern cstr sdcc_include_path;
extern cstr sdcc_library_path;
extern cstr vcc_compiler_path;
extern cstr vcc_include_path;
extern cstr vcc_library_path;


extern const char DEFAULT_CODE_SEGMENT[];

class CharMap;
namespace zasm
{
class Z80;
}

enum Target {
	TARGET_UNSET,
	ROM,   // generic binary, for eprom burner, hex file addresses start at 0
	BIN,   // generic binary, for ram loader, hex file addresses are based to .org
	Z80,   // ZX Spectrum snapshot
	SNA,   // ZX Spectrum snapshot
	TAP,   // ZX Spectrum tape file
	ZX80,  // ZX 80 snapshot / tape file
	ZX81,  // ZX 81 snapshot / tape file
	ZX81P, // ZX 81 snapshot / tape file
	ACE,   // Jupiter Ace snapshot
	TZX	   // universal ZX Spectrum etc. tape file
};

class Z80Assembler
{
public:
	int	   verbose		  = 1;
	uint   max_errors	  = 30;
	double timestamp	  = now(); // for __date__ and __time__
	CpuID  cpu			  = CpuDefault;
	Target default_target = ROM;	 // if target == UNSET
	bool   ixcbr2_enabled = no;		 // enable ixcb illegals: e.g. set b,(ix+d),r2
	bool   ixcbxh_enabled = no;		 // enable ixcb illegals: e.g. bit b,xh
	bool   syntax_8080	  = no;		 // use 8080 assembler syntax
	bool   convert_8080	  = no;		 // convert 8080 mnemonics to Z80 style
	bool   allow_dotnames = no;		 // allow label names starting with a dot '.'
	bool   require_colon  = no;		 // program labels must be followed by a colon ':'
	bool   casefold		  = no;		 // label names are not case sensitive
	bool   flat_operators = no;		 // no operator precedence: evaluate strictly from left to right
	bool   cgi_mode		  = no;		 // disallow escaping from sourcefile's directory
	bool   compare_to_old = no;		 // compare own output file to existing reference file
	cstr   c_compiler	  = nullptr; // -c: fqn to sdcc or vcc or NULL
	cstr   c_includes	  = nullptr; // -I: fqn to custom include dir or NULL
	cstr   stdlib_dir	  = nullptr; // -L: fqn to custom library dir or NULL (not only c but any .globl)
	Errors errors;

private:
	// set by checkCpuOptions():
	bool target_z180; // Z180 / HD64180
	bool target_8080; // I8080
	bool target_z80;  // Z80
	bool target_z80_or_z180;

	// performance:
	double starttime; // of assembly

	// files and paths:
	cstr   source_directory; // top-level source
	cstr   source_filename;
	cstr   temp_directory;
	cstr   target_ext;
	cstr   target_filepath;
	Target target = TARGET_UNSET; // target file format as set by #target directive
	Target effective_target() const noexcept { return target != TARGET_UNSET ? target : default_target; }

	// source:
	Source		source; // SourceLine[] accumulating total source
	uint		current_sourceline_index;
	SourceLine& current_sourceline() { return source[current_sourceline_index]; }

	// code:
	Segments segments; // code and data segments
	Segment* current_segment_ptr;
	Segment& current_segment() { return *current_segment_ptr; }

	// Labels:
	Array<Labels> labels;
	uint		  local_labels_index;
	Labels&		  global_labels() { return labels[0]; }
	Labels&		  local_labels() { return labels[local_labels_index]; }
	uint		  local_blocks_count;
	cstr		  reusable_label_basename; // name of last normal label

	// Macros:
	Macros macros;

	// cond. assembly:
	uint32 cond_off; // effective final on/off state of conditions nested:
					 // 0 = assemble; !0 => assembly off
	uint8 cond[32];	 // cond. state for up to 32 nested conditional blocks
	enum {
		no_cond = 0, // no conditional assembly
		cond_if,	 // #if or #elif pending and no path 'on' up to now
		cond_if_dis, // #if or #elif pending and 'on' path currently or already processed
		cond_else	 // #else pending
	};
	bool		if_pending; // flag to modify label search in value()
	Array<bool> if_values;
	uint		if_values_idx;

	// values set and modified during assembly:
	CharMap* charset;  // characterset conversion (if any)
	Value	 cmd_dpos; // start of current opcode in segment

	// Errors:
	uint	 pass;
	bool	 end;
	Validity validity;		  // validity of generated code
	int		 labels_changed;  // count value changes of (preliminary) labels
	int		 labels_resolved; // count labels which became valid

	// c compiler:
	bool		is_sdcc;
	bool		is_vcc;
	cstr		c_tempdir; // fqn of sub directory in temp_directory acc. to c_flags for .s files
	Array<cstr> c_flags;
	int			c_qi; // index for source file in c_flags[] or -1
	int			c_zi; // index for output file in c_flags[] or -1

private:
	void	   replaceCurlyBraces(SourceLine&);
	uint	   skipMacroBlock(uint i, cstr a, cstr e);
	void	   parseBytes(SourceLine&, Array<uint8>&);
	IoSequence parseIoSequence(SourceLine&);
	Value	   value(SourceLine&, int prio = 0);
	void	   skip_expression(SourceLine&, int prio);
	void	   asmLabel(SourceLine&);
	void	   asmDirect(SourceLine&); // #directives
	void	   asmIf(SourceLine&);
	void	   asmElif(SourceLine&);
	void	   asmElse(SourceLine&);
	void	   asmEndif(SourceLine&);
	void	   asmTarget(SourceLine&);
	void	   asmInclude(SourceLine&);
	void	   asmInsert(SourceLine&);
	void	   asmTzx(SourceLine&);
	void	   asmSegment(SourceLine&, SegmentType);
	void	   asmCFlags(SourceLine&);
	void	   asmCPath(SourceLine&);
	void	   asmLocal(SourceLine&);
	void	   asmEndLocal(SourceLine&);
	void	   asmEnd(SourceLine&);
	void	   asmAssert(SourceLine&);
	void	   asmDefine(SourceLine&);
	void	   asmCompress(SourceLine&);
	void	   asmCharset(SourceLine&);
	void	   asmFirstOrg(SourceLine&);
	void	   asmRept(SourceLine&, cstr rept, cstr endm);
	void	   asmMacro(SourceLine&, cstr macro, cstr name, char tag);
	void	   asmMacroCall(SourceLine&, Macro&);
	void	   asmShebang(SourceLine&);
	cstr	   compileFile(cstr);
	void	   compressSegments();

	void (Z80Assembler::*asmInstr)(SourceLine&, cstr);
	void asmPseudoInstr(SourceLine&, cstr);
	void asmZ80Instr(SourceLine&, cstr);
	void asm8080Instr(SourceLine&, cstr);
	void asmNoSegmentInstr(SourceLine&, cstr);
	void asmRawDataInstr(SourceLine&, cstr);
	void asmTzxPulses(SourceLine&, cstr);
	void asmTzxHardwareInfo(SourceLine&, cstr);
	void asmTzxArchiveInfo(SourceLine&, cstr);

	void store(int n) { current_segment().store(n); }
	void store(int n, int m) { current_segment().store(n, m); }
	void store(int n, int m, int u) { current_segment().store(n, m, u); }
	void store(int a, int b, int c, int d) { current_segment().store(a, b, c, d); }
	void storeIXopcode(int n);
	void storeEDopcode(int n);
	void storeIYopcode(int n);

	void storeWord(cValue& n) { current_segment().storeWord(n); }
	void storeBlock(cptr blk, uint n) { current_segment().storeBlock(blk, n); }
	void storeHexbytes(cptr hex, uint n) { current_segment().storeHexBytes(hex, n); }
	void storeByte(cValue& n) { current_segment().storeByte(n); }
	void storeOffset(cValue& n) { current_segment().storeOffset(n); }

	cValue& currentPosition() { return current_segment_ptr->dpos; }
	Value	dollar()
	{
		if (DataSegment* seg = dynamic_cast<DataSegment*>(current_segment_ptr))
			return seg->lpos - seg->dpos + cmd_dpos;
		else
			throw SyntaxError(
				current_segment_ptr ? "current segment does not provide a '$' address" :
									  "not in any segment -- org not yet set?");
	}
	Value dollarDollar()
	{
		if (DataSegment* seg = dynamic_cast<DataSegment*>(current_segment_ptr))
			return seg->getAddress() + cmd_dpos;
		else
			throw SyntaxError("current segment does not provide a '$$' address");
	}

	int getCondition(SourceLine&, bool expect_comma);
	int getRegister(SourceLine&, Value&, bool quad_reg = no);
	int get8080Register(SourceLine& q);
	int get8080WordRegister(SourceLine& q, uint);

	void setLabelValue(Label*, int32, Validity);
	void setLabelValue(Label*, cValue&);

	void setError(const AnyError&);									 // set error for current file, line & column
	void setError(cstr format, ...) __printflike(2, 3);				 // set error for current file, line & column
	void setError(SourceLine*, cstr format, ...) __printflike(3, 4); // set error for file, line & column
	void init_c_flags();
	void init_c_tempdir();
	void init_c_compiler(cstr cc = nullptr);

	bool  is_name(cstr w) { return is_letter(*w) || *w == '_' || (allow_dotnames && *w == '.'); }
	cstr  unquotedstr(cstr);
	cstr  get_filename(SourceLine&, bool dir = no);
	cstr  get_directory(SourceLine& q) { return get_filename(q, yes); }
	uint8 charcode_from_utf8(cptr& s);

public:
	Z80Assembler();
	~Z80Assembler();

	void assembleFile(
		cstr sourcepath,		  // source file must exist
		cstr destpath  = nullptr, // dflt = source directory, may be dir or filename
		cstr listpath  = nullptr, // dflt = dest direcory, may be dir or filename
		cstr temppath  = nullptr, // dflt = dest dir, must be dir
		int	 liststyle = 1,		  // 0=none, 1=plain, 2=w/ocode, 4=w/labels, 8=w/clkcycles
		int	 deststyle = 'b',	  // 0=none, 'b'=bin, 'x'=intel hex, 's'=motorola s19
		bool clean	   = no) noexcept;
	void assemble(StrArray& sourcelines, cstr sourcepath) noexcept;
	void assembleOnePass(uint pass) noexcept;
	void assembleLine(SourceLine&);
	uint assembleSingleLine(uint address, cstr z80_instruction, char buffer[]);
	void convert8080toZ80(cstr source, cstr dest);
	void runTestcode();
	void runTestcode(TestSegment&, zasm::Z80&);

	void checkTargetfile();
	void writeListfile(cstr filepath, int style);
	void writeTargetfile(cstr& filepath, int style);
	void writeBinFile(FD&);
	void writeHexFile(FD&);
	void writeS19File(FD&);
	void writeTapFile(FD&);
	void writeTzxFile(FD&);
	void writeZ80File(FD&);
	void writeSnaFile(FD&);
	void writeAceFile(FD&);
	void writeZX80File(FD&);
	void writeZX81File(FD&);
	void checkBinFile();
	void checkTapFile();
	void checkTzxFile();
	void checkZ80File();
	void checkSnaFile();
	void checkAceFile();
	void checkZX80File();
	void checkZX81File();

	void checkCpuOptions();
	uint numErrors() const noexcept { return errors.count(); }
	cstr targetFilepath() const noexcept { return target_filepath; }
	uint numPasses() const noexcept { return pass; }
	uint numSourcelines() const noexcept { return source.count(); }
};
