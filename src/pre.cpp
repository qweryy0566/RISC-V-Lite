#include <unordered_map>

enum TYPE {
  HALT,
  LUI,
  AUIPC,
  JAL,
  JALR,
  BEQ,
  BNE,
  BLT,
  BGE,
  BLTU,
  BGEU,
  LB,
  LH,
  LW,
  LBU,
  LHU,
  SB,
  SH,
  SW,
  ADDI,
  SLTI,
  SLTIU,
  XORI,
  ORI,
  ANDI,
  SLLI,
  SRLI,
  SRAI,
  ADD,
  SUB,
  SLL,
  SLT,
  SLTU,
  XOR,
  SRL,
  SRA,
  OR,
  AND
};

std::unordered_map<unsigned, TYPE> to_type{
    {0b00000000000110111, LUI},   {0b00000000000010111, AUIPC},
    {0b00000000001101111, JAL},   {0b00000000001100111, JALR},
    {0b00000000001100011, BEQ},   {0b00000000011100011, BNE},
    {0b00000001001100011, BLT},   {0b00000001011100011, BGE},
    {0b00000001101100011, BLTU},  {0b00000001111100011, BGEU},
    {0b00000000000000011, LB},    {0b00000000010000011, LH},
    {0b00000000100000011, LW},    {0b00000001000000011, LBU},
    {0b00000001010000011, LHU},   {0b00000000000100011, SB},
    {0b00000000010100011, SH},    {0b00000000100100011, SW},
    {0b00000000000010011, ADDI},  {0b00000000100010011, SLTI},
    {0b00000000110010011, SLTIU}, {0b00000001000010011, XORI},
    {0b00000001100010011, ORI},   {0b00000001110010011, ANDI},
    {0b00000000010010011, SLLI},  {0b00000001010010011, SRLI},
    {0b01000001010010011, SRAI},  {0b00000000000110011, ADD},
    {0b01000000000110011, SUB},   {0b00000000010110011, SLL},
    {0b00000000100110011, SLT},   {0b00000000110110011, SLTU},
    {0b00000001000110011, XOR},   {0b00000001010110011, SRL},
    {0b01000001010110011, SRA},   {0b00000001100110011, OR},
    {0b00000001110110011, AND}};
std::unordered_map<unsigned, char> imm_type{
    {0b0110111, 'U'}, {0b0010111, 'U'}, {0b1101111, 'J'},
    {0b1100111, 'I'}, {0b1100011, 'B'}, {0b0000011, 'I'},
    {0b0100011, 'S'}, {0b0010011, 'I'}, {0b0110011, '\0'}};

struct Instruct {
  TYPE type;
  unsigned rs1, rs2, rd, imm;
};

// 将一个 (beg+1) 位的符号数扩展为一个 32 位符号数。
unsigned sign_extend(unsigned x, unsigned beg) {
  return ~x >> beg & 1 ? x : -1 ^ (1u << beg) - 1 | x;
}
unsigned get(unsigned code, unsigned high, unsigned low) {
  return code >> low & (1u << high - low + 1) - 1u;
}

Instruct Decode(unsigned code) {
  Instruct res;
  if (code == 0x0ff00513) {
    res.type = HALT;
    return res;
  }
  unsigned opcode{get(code, 6, 0)}, funct3{0}, funct7{0};
  if (opcode != 0b0110111 && opcode != 0b0010111 && opcode != 0b1101111) {
    funct3 = get(code, 14, 12);
    if (opcode == 0b0110011 || opcode == 0b0010011 && funct3 == 0b101)
      // R-type of I-type shift
      funct7 = get(code, 31, 25);
  }
  res.type = to_type[(funct7 << 3 | funct3) << 7 | opcode];
  res.rd = get(code, 11, 7);
  res.rs1 = get(code, 19, 15);
  res.rs2 = get(code, 24, 20);
  switch (imm_type[opcode]) {
    case 'U':
      res.imm = get(code, 31, 12) << 12;
      break;
    case 'J':
      res.imm =
          sign_extend(get(code, 31, 31) << 20 | get(code, 19, 12) << 12 |
                          get(code, 20, 20) << 11 | get(code, 30, 21) << 1,
                      20);
      break;
    case 'I':
      res.imm = sign_extend(get(code, 31, 20), 11);
      break;
    case 'S':
      res.imm = sign_extend(get(code, 31, 25) << 5 | get(code, 11, 7), 11);
      break;
    case 'B':
      res.imm = sign_extend(get(code, 31, 31) << 12 | get(code, 7, 7) << 11 |
                                get(code, 30, 25) << 5 | get(code, 11, 8) << 1,
                            12);
  }
  return res;
}

#include <iostream>
#include <iomanip>

struct Register {
  unsigned arr[32]{0}, pc{0}, fake;
  unsigned &operator[](const int &i) {
    return i != 0 ? arr[i] : fake = 0;
  }
};

class CPU {
  Register reg;
  unsigned mem[1 << 20];

 public:
  void Load() {
    unsigned at;
    std::string in;
    while (std::cin >> in)
      if (in[0] == '@')
        at = std::stoi(in.substr(1), 0, 16);
      else
        mem[at++] = std::stoi(in, 0, 16);
  }
  void Run() {
    unsigned code;
    for (unsigned need = 1;; reg.pc += need * 4, need = 1) {
      code = mem[reg.pc] | mem[reg.pc + 1] << 8 | mem[reg.pc + 2] << 16 |
             mem[reg.pc + 3] << 24;
      Instruct ins{Decode(code)};
      std::cerr << std::hex << code << '\n';
      if (code == 0x40e78633) {
        for (int i = 0; i < 32; ++i)
          std::cerr << reg[i] << ' ';
        std::cerr << reg.pc << '\n';
        exit(0);
      }
      switch (ins.type) {
        case HALT:
          std::cout << (reg[10] & 255u) << '\n';
          return;
        case LUI:
          reg[ins.rd] = ins.imm;
          break;
        case AUIPC:
          reg[ins.rd] = ins.imm + reg.pc;
          break;
        case JAL:
          reg[ins.rd] = reg.pc + 4;
          need = 0, reg.pc += ins.imm;
          break;
        case JALR:
          reg[ins.rd] = reg.pc + 4;
          need = 0, reg.pc = reg[ins.rs1] + ins.imm & -2;
          break;
        case BEQ:
          if (reg[ins.rs1] == reg[ins.rs2])
            need = 0, reg.pc += ins.imm;
          break;
        case BNE:
          if (reg[ins.rs1] != reg[ins.rs2])
            need = 0, reg.pc += ins.imm;
          break;
        case BLT:
          if ((int)reg[ins.rs1] < (int)reg[ins.rs2])
            need = 0, reg.pc += ins.imm;
          break;
        case BGE:
          if ((int)reg[ins.rs1] >= (int)reg[ins.rs2])
            need = 0, reg.pc += ins.imm;
          break;
        case BLTU:
          if (reg[ins.rs1] < reg[ins.rs2])
            need = 0, reg.pc += ins.imm;
          break;
        case BGEU:
          if (reg[ins.rs1] >= reg[ins.rs2])
            need = 0, reg.pc += ins.imm;
          break;
        case LB:
          reg[ins.rd] = sign_extend(get(mem[reg[ins.rs1] + ins.imm], 7, 0), 7);
          break;
        case LH:
          reg[ins.rd] = sign_extend(get(mem[reg[ins.rs1] + ins.imm], 15, 0), 15);
          break;
        case LW:
          reg[ins.rd] = mem[reg[ins.rs1] + ins.imm];
          std::cerr << "# " << reg[ins.rs1] + ins.imm << ' ' << mem[reg[ins.rs1] + ins.imm] << '\n';
          break;
        case LBU:
          reg[ins.rd] = get(mem[reg[ins.rs1] + ins.imm], 7, 0);
          break;
        case LHU:
          reg[ins.rd] = get(mem[reg[ins.rs1] + ins.imm], 15, 0);
          break;
        case SB:
          mem[reg[ins.rs1] + ins.imm] = get(reg[ins.rs2], 7, 0);
          break;
        case SH:
          mem[reg[ins.rs1] + ins.imm] = get(reg[ins.rs2], 15, 0);
          break;
        case SW:
          mem[reg[ins.rs1] + ins.imm] = reg[ins.rs2];
          break;
        case ADDI:
          reg[ins.rd] = reg[ins.rs1] + ins.imm;
          break;
        case SLTI:
          reg[ins.rd] = (int)reg[ins.rs1] < (int)ins.imm;
          break;
        case SLTIU:
          reg[ins.rd] = reg[ins.rs1] < ins.imm;
          break;
        case XORI:
          reg[ins.rd] = reg[ins.rs1] ^ ins.imm;
          break;
        case ORI:
          reg[ins.rd] = reg[ins.rs1] | ins.imm;
          break;
        case ANDI:
          reg[ins.rd] = reg[ins.rs1] & ins.imm;
          break;
        case SLLI:
          reg[ins.rd] = reg[ins.rs1] << get(ins.imm, 4, 0);
          break;
        case SRLI:
          reg[ins.rd] = reg[ins.rs1] >> get(ins.imm, 4, 0);
          break;
        case SRAI:
          reg[ins.rd] = reg[ins.rs1] >> get(ins.imm, 4, 0);
          reg[ins.rd] = sign_extend(reg[ins.rd], 31 - get(ins.imm, 4, 0));
          break;
        case ADD:
          reg[ins.rd] = reg[ins.rs1] + reg[ins.rs2];
          break;
        case SUB:
          reg[ins.rd] = reg[ins.rs1] - reg[ins.rs2];
          break;
        case SLL:
          reg[ins.rd] = reg[ins.rs1] << get(reg[ins.rs2], 4, 0);
          break;
        case SLT:
          reg[ins.rd] = (int)reg[ins.rs1] < (int)reg[ins.rs2];
          break;
        case SLTU:
          reg[ins.rd] = reg[ins.rs1] < reg[ins.rs2];
          break;
        case XOR:
          reg[ins.rd] = reg[ins.rs1] ^ reg[ins.rs2];
          break;
        case SRL:
          reg[ins.rd] = reg[ins.rs1] >> get(reg[ins.rs2], 4, 0);
          break;
        case SRA:
          reg[ins.rd] = reg[ins.rs1] >> get(reg[ins.rs2], 4, 0);
          reg[ins.rd] = sign_extend(reg[ins.rd], 31 - get(reg[ins.rs2], 4, 0));
          break;
        case OR:
          reg[ins.rd] = reg[ins.rs1] | reg[ins.rs2];
          break;
        case AND:
          reg[ins.rd] = reg[ins.rs1] & reg[ins.rs2];
          break;
      }
    }
  }
};

CPU cpu;

int main() {
  cpu.Load();
  cpu.Run();
  return 0;
}