#ifndef RISC_V_CPU_PREMIUM_HPP_
#define RISC_V_CPU_PREMIUM_HPP_

#include "elements.hpp"
#include "tools.hpp"

const int QUEUE_SIZE = 32;

class CPU_PREM {
  uint8_t mem[1 << 20];
  unsigned pc{0};
  class Register {
    unsigned arr[32]{0}, fake;

   public:
    unsigned reorder[32]{0};
    unsigned &operator[](const int &i) { return i != 0 ? arr[i] : fake = 0; }

  } reg_in, reg_out;

  class ReservationStation {
    RSElement arr[QUEUE_SIZE + 1];
    int size{0};

   public:
    bool Full() const { return size == QUEUE_SIZE; }
    int Add(const RSElement &x) {
      int i = QUEUE_SIZE;
      while (i && arr[i].isBusy) --i;
      return arr[i] = x, i;
    }

  } RS_in, RS_out;

  CircQueue<ROB> ROB_in, ROB_out;
  CircQueue<Instruct> in_queue;

  CircQueue<RSElement> SLB_in, SLB_out;
  CDB to_issue, issue_to_rob, issue_to_rs, issue_to_slb, issue_to_reg;
  CDB to_execute;

  void Issue() {
    if (to_issue.is_stall) return;
    // 将对应指令放到 RS 与 ROB 中.
    Instruct inst = to_issue.inst;
    issue_to_rob.to_ROB = {inst.type, 1, 0, inst.rd, 0};
    RSElement send;
    send.type = inst.type;
    send.A = inst.imm;
    send.isBusy = 1;
    send.dest = ROB_in.Next();
    if (reg_in.reorder[inst.rs1])
      if (ROB_in[reg_in.reorder[inst.rs1]].isReady)
        send.Vj = ROB_in[reg_in.reorder[inst.rs1]].value, send.Qj = 0;
      else
        send.Qj = reg_in.reorder[inst.rs1];
    else
      send.Vj = reg_in[inst.rs1], send.Qj = 0;
    if (reg_in.reorder[inst.rs2])
      if (ROB_in[reg_in.reorder[inst.rs2]].isReady)
        send.Vk = ROB_in[reg_in.reorder[inst.rs2]].value, send.Qk = 0;
      else
        send.Qk = reg_in.reorder[inst.rs2];
    else
      send.Vk = reg_in[inst.rs2], send.Qk = 0;
    issue_to_reg.is_stall = 0;
    issue_to_reg.to_reg = {inst.rd, 0, send.dest};

    if (LB <= inst.type && inst.type <= SW) {
      issue_to_slb.to_SLB = send;
      issue_to_slb.is_stall = 0, issue_to_rs.is_stall = 1;
    } else {
      issue_to_rs.to_RS = send;
      issue_to_rs.is_stall = 0, issue_to_slb.is_stall = 1;
    }
  }

  void Execute() {

  }

  void Commit() {}

  void Update() {
    reg_in = reg_out;
    ROB_in = ROB_out;
    RS_in = RS_out;
    ROB_in = ROB_out;
    SLB_in = SLB_out;
  }

  void RunRob() {}
  void RunSlb() {}
  void RunRs() {
    RS_out.Add(issue_to_rs.to_RS);
    
  }
  void RunReg() {}
  void RunInQueue() {
    // 在这里判断是否可以发送指令，否则 stall.
    to_issue.is_stall = 0;
    if (RS_in.Full() || ROB_in.Full() || SLB_in.Full()) {
      to_issue.is_stall = 1;
      return;
    }
    to_issue.inst = Decode(*(unsigned *)(mem + pc)), pc += 4;
  }

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
    int clk = 0;
    for (;; ++clk) {
      Update();
      RunRob();
      RunSlb();
      RunRs();
      RunReg();
      RunInQueue();

      Execute();
      Issue();
      Commit();
    }
  }
};

#endif  // RISC_V_CPU_PREMIUM_HPP_