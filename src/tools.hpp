#ifndef RISC_V_TOOLS_HPP_
#define RISC_V_TOOLS_HPP_

#include <cstring>
#include <iostream>
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
    {0b0100011, 'S'}, {0b0010011, 'I'}, {0b0110011, 'R'}};

struct Instruct {
  TYPE type;
  unsigned rs1{0}, rs2{0}, rd{0}, imm{0};
};

// 将一个 (beg+1) 位的符号数扩展为一个 32 位符号数。
unsigned sign_extend(unsigned x, int beg) {
  return x >> beg  ? -1 ^ (1u << beg) - 1 | x : x;
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
  switch (imm_type[opcode]) {
    case 'U':
      res.imm = get(code, 31, 12) << 12;
      res.rd = get(code, 11, 7);
      break;
    case 'J':
      res.imm =
          sign_extend(get(code, 31, 31) << 20 | get(code, 19, 12) << 12 |
                          get(code, 20, 20) << 11 | get(code, 30, 21) << 1,
                      20);
      res.rd = get(code, 11, 7);
      break;
    case 'I':
      res.imm = sign_extend(get(code, 31, 20), 11);
      res.rd = get(code, 11, 7);
      res.rs1 = get(code, 19, 15);
      break;
    case 'S':
      res.imm = sign_extend(get(code, 31, 25) << 5 | get(code, 11, 7), 11);
      res.rs1 = get(code, 19, 15);
      res.rs2 = get(code, 24, 20);
      break;
    case 'B':
      res.imm = sign_extend(get(code, 31, 31) << 12 | get(code, 7, 7) << 11 |
                                get(code, 30, 25) << 5 | get(code, 11, 8) << 1,
                            12);
      res.rs1 = get(code, 19, 15);
      res.rs2 = get(code, 24, 20);
      break;
    case 'R':
      res.rd = get(code, 11, 7);
      res.rs1 = get(code, 19, 15);
      res.rs2 = get(code, 24, 20);
  }
  return res;
}

template <class T, int SIZE = 32>
class CircQueue {
  T q[SIZE + 1]{};
  int hd{1}, tl{1};

  static int Add(const int &x) {
    return x == SIZE ? 1 : x + 1;
  }

 public:
  T &operator[](const int &i) { return q[i]; }
  bool Full() const { return Add(tl) == hd; }
  bool Empty() const { return hd == tl; }
  bool Push(const T &x) {
    return Full() ? 0 : (q[tl = Add(tl)] = x, 1);
  }
  unsigned TopId() const { return hd; }
  T &Top() { return q[Add(hd)]; }
  bool Pop() {
    return Empty() ? 0 : (hd = Add(hd), 1);
  }
  void Clear() { hd = tl = 1; }
  int Next() const { return Add(tl); }
};

#endif  // RISC_V_TOOLS_HPP_