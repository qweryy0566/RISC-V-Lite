#ifndef RISC_V_CPU_HPP_
#define RISC_V_CPU_HPP_

#include <iostream>
#include <iomanip>

#include "tools.hpp"

struct Register {
  unsigned arr[32]{0}, pc{0}, fake;
  unsigned &operator[](const int &i) {
    return i != 0 ? arr[i] : fake = 0;
  }
  friend std::ostream &operator<<(std::ostream &out, const Register &x) {
    for (int i = 0; i < 32; ++i) out << i << ": " << x.arr[i] << '\n';
    return out;
  }
};

class CPU {
  Register reg;
  uint8_t mem[1 << 20];

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
    unsigned code, tmp;
    for (unsigned need = 1;; reg.pc += need * 4, need = 1) {
      code = mem[reg.pc] | mem[reg.pc + 1] << 8 | mem[reg.pc + 2] << 16 |
             mem[reg.pc + 3] << 24;
      Instruct ins{Decode(code)};
      std::cerr << ToStr(ins.type) << '\n';
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
          // std::cerr << "ra = " << reg[1] << '\n';
          reg[ins.rd] = reg.pc + 4;
          need = 0, reg.pc += ins.imm;
          break;
        case JALR:
          std::cerr << "ra = " << (reg[ins.rs1] + ins.imm & -2) << '\n';
          tmp = reg.pc + 4;
          need = 0, reg.pc = reg[ins.rs1] + ins.imm & -2;
          reg[ins.rd] = tmp;
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
          // std::cerr << "## " << reg[ins.rs1] << ' '  << reg[ins.rs2] << '\n';
          if (reg[ins.rs1] < reg[ins.rs2])
            need = 0, reg.pc += ins.imm;
          break;
        case BGEU:
          if (reg[ins.rs1] >= reg[ins.rs2])
            need = 0, reg.pc += ins.imm;
          break;
        case LB:
          reg[ins.rd] = sign_extend(mem[reg[ins.rs1] + ins.imm], 7);
          break;
        case LH:
          reg[ins.rd] = mem[reg[ins.rs1] + ins.imm] |
                        mem[reg[ins.rs1] + ins.imm + 1] << 8;
          reg[ins.rd] = sign_extend(reg[ins.imm], 15);
          break;
        case LW:
          reg[ins.rd] = mem[reg[ins.rs1] + ins.imm] |
                        mem[reg[ins.rs1] + ins.imm + 1] << 8 |
                        mem[reg[ins.rs1] + ins.imm + 2] << 16 |
                        mem[reg[ins.rs1] + ins.imm + 3] << 24;
          break;
        case LBU:
          reg[ins.rd] = mem[reg[ins.rs1] + ins.imm];
          break;
        case LHU:
          reg[ins.rd] = mem[reg[ins.rs1] + ins.imm] |
                        mem[reg[ins.rs1] + ins.imm + 1] << 8;
          break;
        case SB:
          mem[reg[ins.rs1] + ins.imm] = reg[ins.rs2];
          break;
        case SH:
          mem[reg[ins.rs1] + ins.imm] = reg[ins.rs2];
          mem[reg[ins.rs1] + ins.imm + 1] = reg[ins.rs2] >> 8;
          break;
        case SW:
          mem[reg[ins.rs1] + ins.imm] = reg[ins.rs2];
          mem[reg[ins.rs1] + ins.imm + 1] = reg[ins.rs2] >> 8;
          mem[reg[ins.rs1] + ins.imm + 2] = reg[ins.rs2] >> 16;
          mem[reg[ins.rs1] + ins.imm + 3] = reg[ins.rs2] >> 24;
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
          reg[ins.rd] = (int)reg[ins.rs1] >> get(ins.imm, 4, 0);
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
          reg[ins.rd] = (int)reg[ins.rs1] >> get(reg[ins.rs2], 4, 0);
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

#endif  // RISC_V_CPU_HPP_