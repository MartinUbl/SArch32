#pragma once

#include <string>
#include <array>
#include <vector>
#include <memory>
#include <unordered_map>
#include <stdexcept>
#include <bit>
#include <variant>

/*
 * Enumerator of all existing registers
 */
enum class NRegister
{
	// general purpose registers
	R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11,
	// stack pointer
	SP,
	// return address
	RA,
	// flags
	FLG,
	// program counter
	PC,

	count, // 16 (to fit into 4 bits)

	// special mark
	ignored = R0,
};

// convenience constant for register count
constexpr size_t Register_Count = static_cast<size_t>(NRegister::count);

// retrieve a register name using its enum value
std::string Get_Register_Name(NRegister reg);
// retrieve a register name using its index
std::string Get_Register_Name(size_t regIdx);

/*
 * Existing processor state flags (FLG register)
 */
enum class NFlags
{
	Sign = 1 << 0,		// N
	Zero = 1 << 1,		// Z
	Overflow = 1 << 2,	// V
	Carry = 1 << 3,		// C

	Interrupt_Enable = 1 << 12,		// are interrupts enabled?
};

/*
 * Instruction condition flags (encoded in the highest 3 bits of MSB)
 */
enum class NCondition
{
	always         = 0b000,	// any state
	equal          = 0b001,	// Z == 1
	not_equal      = 0b010,	// Z == 0
	greater        = 0b011, // Z == 0 && N == V	(signed)
	greater_equal  = 0b100, // N == V (signed)
	less           = 0b101, // N != V (signed)
	less_equal     = 0b110,	// Z == 1 || N != V (signed)

	unspecified    = 0b111,	// undefined condition
};

/*
 * Entries in interrupt vector table (IVT)
 */
enum class NIVT_Entry
{
	Reset,				// called upon warm reset
	Abort,				// called upon memory abort event (e.g., unmapped memory access)
	Undefined,			// called upon decoding an undefined instruction
	IRQ,				// called upon external interrupt request
	SupervisorCall,		// software interrupt - call the supervisor using an interrupt
};

/*
 * All existing operation codes
 */
enum class NOpcode
{
	nop,  // no operation (nop)

	mov,  // move register <- register (mov reg1, reg2) (reg1 <-- reg2)
	movi, // move register <- immediate (mov reg, #imm) (reg <-- #imm)
	add,  // add register <- register (add reg1, reg2) (reg1 <-- reg1 + reg2)
	addi, // add register <- immediate (add reg, #imm) (reg1 <-- reg1 + #imm)
	sub,  // sub register <- register (sub reg1, reg2) (reg1 <-- reg1 - reg2)
	subi, // sub register <- immediate (sub reg, #imm) (reg1 <-- reg1 - #imm)
	mul,  // mul register <- register (mul reg1, reg2) (reg1 <-- reg1 * reg2)
	muli, // mul register <- immediate (mul reg, #imm) (reg1 <-- reg1 * #imm)
	div,  // div register <- register (div reg1, reg2) (reg1u <-- reg1 / reg2, reg1l <-- reg1 % reg2)
	divi, // div register <- immediate (div reg, #imm) (reg1u <-- reg1 / #imm, reg1l <-- reg1 % #imm)

	and_,  // and register <- register (and reg1, reg2) (reg1 <-- reg1 & reg2)
	andi, // and reg�ster <- immediate (and reg, #imm) (reg1 <-- reg1 # #imm)
	or_,   // or register <- register (or reg1, reg2) (reg1 <-- reg1 | reg2)
	ori,  // or register <- immediate (or reg1, #imm) (reg1 <-- reg1 | #imm)
	slr,  // shift left register <- register (slr reg1, reg2) (reg1 <-- reg1 << reg2)
	sli,  // shift left register <- immediate (slr reg1, #imm) (reg1 <-- reg1 << #imm)
	srr,  // shift right register <- register (srr reg1, reg2) (reg1 <-- reg1 >> reg2)
	sri,  // shift right register <- immediate (sri reg1, #imm) (reg1 <-- reg1 >> #imm)

	lw,   // load word register <- register (lw reg1, reg2) (reg1 <-- memory[reg2])
	li,   // load word register <- immediate address (li reg1, #imm) (reg1 <-- memory[#imm])
	sw,   // store word register <- register (sw reg1, reg2) (memory[reg2] <-- reg1)
	si,   // store word register <- immediate address (si reg1, #imm) (memory[#imm] <-- reg1)

	cmpr, // compare registers (cmp reg1, reg2) (flags <-- compare result of reg1 and reg2)
	cmpi, // compare register with immediate (cmp reg1, #imm) (flags <-- compare result of reg1 and #imm)

	br,   // branch to register (br reg1) (pc <-- reg1)
	bi,   // branch to immediate (bi #imm) (pc <-- #imm)

	push, // push register to stack (push reg1) (memory[sp-4] <-- reg1, sp <-- sp - 4)
	pop,  // pop register from stack (pop reg1) (reg1 <-- memory[sp], sp <-- sp + 4)

	fw,   // fetch 24bit word (fw #imm) (r0 <-- #imm(24bit))
	svc,  // supervisor call (svc #imm) (ra <-- pc, pc <-- ivt[x])

	__unassigned, // has no defined operation

	count, // 32 --> 5 bits for instruction
};

// convenience constant for opcode count
constexpr size_t Opcode_Count = static_cast<size_t>(NOpcode::count);

/*
 * Exception generated by instruction parser
 */
class sarch32_parser_exception : public std::exception {
	private:
		const std::string mWhat;

	public:
		sarch32_parser_exception(const std::string& what) : mWhat{ what } {
			//
		}

		[[nodiscard]] char const* what() const override
		{
			return mWhat.c_str();
		}
};

/*
 * Exception generated by instruction generator
 */
class sarch32_generator_exception : public std::exception {
	private:
		const std::string mWhat;

	public:
		sarch32_generator_exception(const std::string& what) : mWhat{ what } {
			//
		}

		[[nodiscard]] char const* what() const override
		{
			return mWhat.c_str();
		}
};

/*
 * Memory bus interface
 */
class IBus
{
	public:
		// reads from given address, stores the read bytes of given amount into target pointer
		virtual void Read(uint32_t address, void* target, uint32_t size) const = 0;
		// writes to a given address, places the bytes from source pointer to the memory
		virtual void Write(uint32_t address, const void* source, uint32_t size) = 0;
};

/*
 * Class encapsulation CPU context
 */
class CCPU_Context
{
	protected:
		// registers and their content
		std::array<uint32_t, Register_Count> mRegister_Content{};
		// memory bus
		IBus& mBus;

	public:
		CCPU_Context(IBus& bus) : mBus(bus) {
			std::fill(mRegister_Content.begin(), mRegister_Content.end(), 0xFFFFFFFF);
		}
		virtual ~CCPU_Context() = default;

		// retrieve a reference to register content
		uint32_t& Reg(NRegister regist) {
			return mRegister_Content[static_cast<size_t>(regist)];
		}

		// retrieve a reference to register content, constant context
		const uint32_t& Reg(NRegister regist) const {
			return mRegister_Content[static_cast<size_t>(regist)];
		}

		// does the CPU has given flag?
		bool Has_Flag(NFlags flag) const {
			return (Reg(NRegister::FLG) & static_cast<uint32_t>(flag)) != 0;
		}

		// read a scalar from the memory bus (helper method)
		template<typename T>
		T Mem_Read_Scalar(uint32_t address) {
			T t;
			mBus.Read(address, &t, sizeof(T));
			return t;
		}

		// write a scalar to the memory bus (helper method)
		template<typename T>
		void Mem_Write_Scalar(uint32_t address, const T& t) {
			mBus.Write(address, &t, sizeof(T));
		}
};

/*
 * Type of an instruction operand
 */
enum class NOperand_Type {
	None,

	Register,  // operand is register
	Immediate, // operand is an immediate value

	Immediate_Symbolic, // operand is an immediate value, that is unknown at the moment (will be resolved during linkage)
};

/*
 * Class encapsulating instruction operand
 */
class COperand {
	protected:
		NOperand_Type mType = NOperand_Type::None;
		std::variant<int32_t, NRegister, std::string> mValue;

	public:
		COperand() : mType{ NOperand_Type::None }, mValue{ 0 } {
			//
		}

		COperand(int32_t value) : mType{ NOperand_Type::Immediate }, mValue{ value } {
			//
		}

		COperand(NRegister value) : mType{ NOperand_Type::Register }, mValue{ value } {
			//
		}

		COperand(const std::string& symbol) : mType{ NOperand_Type::Immediate_Symbolic }, mValue{ symbol } {
			//
		}

		// retrieves operand type
		NOperand_Type Get_Type() const {
			return mType;
		}

		// is the operand a register?
		inline bool Is_Register() const {
			return Get_Type() == NOperand_Type::Register;
		}

		// is the operand an immediate value?
		inline bool Is_Immediate() const {
			return Get_Type() == NOperand_Type::Immediate;
		}

		// is the operand a symbolic reference?
		inline bool Is_Symbolic() const {
			return Get_Type() == NOperand_Type::Immediate_Symbolic;
		}

		// retrieves register (given that the type matches)
		const NRegister Get_Register() const {
			return std::get<NRegister>(mValue);
		}

		// retrieves immediate value (given that the type matches)
		const int32_t Get_Immediate() const {
			return std::get<int32_t>(mValue);
		}

		// retrieves symbolic reference (given that the type matches)
		const std::string& Get_Symbol() const {
			return std::get<std::string>(mValue);
		}

		// resolves symbol
		void Resolve_Symbol(int32_t immediate) {
			if (Is_Symbolic()) {
				mType = NOperand_Type::Immediate;
				mValue = immediate;
			}
		}
};

/*
 * Instruction base class
 */
class CInstruction
{
	protected:
		// under what condition the instruction should execute
		NCondition mCondition = NCondition::always;
		// operation code
		NOpcode mOpcode = NOpcode::nop;

	protected:
		// encodes the most significant byte using condition and opcode
		uint8_t Encode_MSB() const {
			return (static_cast<uint8_t>(mCondition) << 5) | (static_cast<uint8_t>(mOpcode));
		}

		// encodes register pair into a single byte
		uint8_t Encode_Register_Pair(NRegister reg1, NRegister reg2) const {
			return static_cast<uint8_t>(reg1) << 4 | static_cast<uint8_t>(reg2);
		}

		// decodes register pair from a single byte
		std::pair<NRegister, NRegister> Decode_Register_Pair(uint8_t val) const {
			return std::make_pair( static_cast<NRegister>(val >> 4), static_cast<NRegister>(val & 0b1111) );
		}

		// encodes whole instruction from 4 bytes
		uint32_t Encode_From_Bytes(const std::array<uint8_t, 4>& bytes) const {
			union {
				uint32_t conv;
				uint8_t bytedmp[4];
			} dump;

			std::copy(bytes.begin(), bytes.end(), dump.bytedmp);

			return dump.conv;
		}

		// encodes whole instruction from two most significant bytes and one 16bit value (e.g., an immediate value)
		uint32_t Encode_From_Byte_Half(const uint8_t msb, const uint8_t msb2, const int16_t param) const {
			union {
				int16_t orig;
				uint8_t dump[2];
			} paramdump;

			paramdump.orig = param;

			return Encode_From_Bytes({ msb, msb2, paramdump.dump[0], paramdump.dump[1] });
		}

		// encodes whole instruction from the MSB and 24bit value
		uint32_t Encode_From_Byte_Extended(const uint8_t msb, const int32_t param) const {
			union {
				int32_t orig;
				uint8_t dump[4];
			} paramdump;

			paramdump.orig = param;

			return Encode_From_Bytes({ msb, paramdump.dump[0], paramdump.dump[1], paramdump.dump[2] });
		}

		// decomposes an instruction from 4byte value to 4 1byte values
		std::array<uint8_t, 4> Decode_From_Bytes(uint32_t word) const {
			union {
				uint32_t conv;
				uint8_t bytedmp[4];
			} dump;

			std::array<uint8_t, 4> res;

			dump.conv = word;
			std::copy(&dump.bytedmp[0], &dump.bytedmp[3] + 1, res.begin());

			return res;
		}

		// decodes an immediate 16bit value from instruction encoding (2 least significant bytes)
		int32_t Decode_Immediate(uint32_t word) const {
			return std::bit_cast<int16_t>(static_cast<uint16_t>((word >> 16) & 0xFFFF)); // this does not preserve sign
		}

		// decodes an immediate 24bit value from instruction encoding (3 leats significant bytes)
		int32_t Decode_Immediate_24b(uint32_t word) const {
			return std::bit_cast<int32_t>(word) / static_cast<int32_t>(0x100); // shift right by 8 bits, but preserving sign
		}

	public:
		CInstruction() = default;
		CInstruction(NOpcode opcode, NCondition cond)
			: mOpcode{ opcode }, mCondition{ cond } {
			//
		}
		virtual ~CInstruction() = default;

		// parses instruction from string mnemonic and operands
		virtual bool Parse_String(const std::string& mnemonic, const std::vector<std::string>& operands) { throw sarch32_parser_exception{"Unimplemented string parser method"}; };

		// parses instruction from binary representation
		virtual bool Parse_Binary(const uint32_t instruction) { throw sarch32_parser_exception{ "Unimplemented binary parser method" }; };

		// generates string from already parsed internal state
		virtual std::string Generate_String(bool hexaFmt = false) const { throw sarch32_generator_exception{ "Unimplemented string generator method" }; }

		// generates binary encoding from already parsed internal state
		virtual uint32_t Generate_Binary() const { throw sarch32_generator_exception{ "Unimplemented binary generator method" }; }

		// executes the instruction in given context
		virtual bool Execute(CCPU_Context& cpu) const { throw sarch32_generator_exception{ "Unimplemented execute method" }; }

		// resolves the symbolic value within this instruction
		virtual void Resolve_Symbol(int32_t value) { throw sarch32_generator_exception{ "Unimplemented resolve symbol method" }; }

		// retrieves resolve request, if any
		virtual bool Get_Resolve_Request(std::string& str) const { return false; }

		// is this a pseudo-instruction? defaulting to "no"
		virtual bool Is_Pseudo_Instruction() const { return false; }

		// generates additional data - applicable to pseudo-instructions
		virtual bool Generate_Additional_Data(std::vector<uint8_t>& data) { return false; }

		// retrieves data or encoding length, defaults to "4" for regular machine instructions
		virtual uint32_t Get_Length() const { return 4; }

		// factory method - build instruction from string
		static std::unique_ptr<CInstruction> Build_From_String(const std::string& line);
		// factory method - build instruction from binary encoding
		static std::unique_ptr<CInstruction> Build_From_Binary(const uint32_t instruction);

		// checks the condition in given CPU context (flags)
		static bool Check_Condition(NCondition cond, const CCPU_Context& cpu) {

			switch (cond) {
				case NCondition::always:
				case NCondition::unspecified:
				default:
					return true;

				case NCondition::equal:
					return cpu.Has_Flag(NFlags::Zero);
				case NCondition::not_equal:
					return !cpu.Has_Flag(NFlags::Zero);
				case NCondition::greater:
					return !cpu.Has_Flag(NFlags::Zero) && (cpu.Has_Flag(NFlags::Sign) == cpu.Has_Flag(NFlags::Overflow));
				case NCondition::greater_equal:
					return (cpu.Has_Flag(NFlags::Sign) == cpu.Has_Flag(NFlags::Overflow));
				case NCondition::less:
					return (cpu.Has_Flag(NFlags::Sign) != cpu.Has_Flag(NFlags::Overflow));
				case NCondition::less_equal:
					return cpu.Has_Flag(NFlags::Zero) || (cpu.Has_Flag(NFlags::Sign) != cpu.Has_Flag(NFlags::Overflow));
			}

		}

		// retrieves operation code
		const NOpcode Get_Opcode() const {
			return mOpcode;
		}

		// decides if the instruction is an immediate one - the lowest bit is not set for immediates, 27 and higher breaks the pattern
		const bool Is_Immediate_Instruction() const {
			return static_cast<uint8_t>(mOpcode) < 27 && !(static_cast<uint8_t>(mOpcode) & 0x1);
		}

		// retrieves the condition part
		const NCondition Get_Condition() const {
			return mCondition;
		}
};

// attempt to parse any operand from given string
COperand Parse_Any(const std::string& str);

// convert a single word to byte dump
void Word_To_Bytes(uint32_t word, std::vector<uint8_t>& res);