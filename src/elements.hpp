#ifndef RISC_V_ELEMENTS_HPP_
#define RISC_V_ELEMENTS_HPP_

#include "tools.hpp"

const int QUEUE_SIZE = 32;

class Register {
  unsigned arr[32]{0}, fake, reorder[32]{0};

 public:
  unsigned &operator[](const int &i) { return i ? arr[i] : fake = 0; }
  unsigned &Reorder(const int &i) { return i ? reorder[i] : fake = 0; }
  void Clear() { memset(reorder, 0, sizeof(reorder)); }
};

struct RSElement {
  TYPE type;
  bool isBusy{0}, is_jump{0};
  unsigned Vj, Vk, Qj, Qk, dest, A, cur_pc;
};
class ReservationStation {
  RSElement arr[QUEUE_SIZE + 1];

 public:
  RSElement &operator[](const int &i) { return arr[i]; }
  bool Full() const {
    for (int i = 1; i <= QUEUE_SIZE; ++i)
      if (!arr[i].isBusy) return 0;
    return 1;
  }
  int Add(const RSElement &x) {
    int i = QUEUE_SIZE;
    while (i && arr[i].isBusy) --i;
    if (i) arr[i] = x;
    return i;
  }
  int Check() const {
    int i = QUEUE_SIZE;
    for (; i; --i)
      if (arr[i].isBusy && !arr[i].Qj && !arr[i].Qk) break;
    return i;
  }
  void Clear() {
    for (int i = 1; i <= QUEUE_SIZE; ++i) arr[i].isBusy = 0;
  }
};

struct SLBElement {
  TYPE type;
  bool isBusy{0}, isReady{0};
  unsigned Vj, Vk, Qj, Qk, dest, A;
  SLBElement() {}
  SLBElement(const RSElement &x)
      : type{x.type},
        isBusy{x.isBusy},
        Vj{x.Vj},
        Vk{x.Vk},
        Qj{x.Qj},
        Qk{x.Qk},
        dest{x.dest},
        A{x.A} {}
};

struct ROB {
  TYPE type;
  bool isBusy{0}, isReady{0};
  unsigned dest, value, to_pc;
};
struct ToReg {
  unsigned index, value, Q;
};
struct ToExec {
  TYPE type;
  unsigned Vj, Vk, dest, A;
  ToExec() {}
  ToExec(const RSElement &obj)
      : type{obj.type}, Vj{obj.Vj}, Vk{obj.Vk}, dest{obj.dest}, A{obj.A} {}
};
struct Result {
  unsigned value, to_pc, in_rob;
};
struct ToCommit {
  TYPE type;
  unsigned dest, value, to_pc, in_rob;
};

// Common Data Bus
struct CDB {
  bool is_stall{1}, need_clean{0}, is_jump{0};
  int id, cur_pc;
  Instruct inst;
  ROB to_ROB;
  ToCommit to_com;
  RSElement to_RS;
  SLBElement to_SLB;
  ToReg to_reg;
  ToExec to_exec;
  Result res;
};

struct MemoryAccess {
  SLBElement opt;
  bool isBusy{0};
  int last_time;

  void Add(const SLBElement &x) { isBusy = 1, last_time = 3, opt = x; }
};

#endif  // RISC_V_ELEMENTS_HPP_