#ifndef RISC_V_ELEMENTS_HPP_
#define RISC_V_ELEMENTS_HPP_

#include "tools.hpp"

struct RSElement {
  TYPE type;
  bool isBusy{0};
  unsigned Vj, Vk, Qj, Qk, dest, A;
};
struct ROB {
  TYPE type;
  bool isBusy{0}, isReady{0};
  unsigned dest, value;
};
struct SLB {
  TYPE type;
  // bool 
};
struct ToReg {
  unsigned index, value, reorder;
};

// Common Data Bus
struct CDB {
  bool is_stall{0};
  Instruct inst;
  ROB to_ROB;
  RSElement to_RS, to_SLB;
  ToReg to_reg;
};



#endif  // RISC_V_ELEMENTS_HPP_