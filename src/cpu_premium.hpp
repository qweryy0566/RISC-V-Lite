#ifndef RISC_V_CPU_PREMIUM_HPP_
#define RISC_V_CPU_PREMIUM_HPP_

#include "elements.hpp"
#include "tools.hpp"

class CPU_PREM {
  uint8_t mem[1 << 20];
  unsigned pc{0};
  class Register {
    unsigned arr[32]{0}, fake, reorder[32]{0};

   public:
    unsigned &operator[](const int &i) { return i ? arr[i] : fake = 0; }
    unsigned &Reorder(const int &i) { return i ? reorder[i] : fake = 0; }
    void Clear() { memset(reorder, 0, sizeof(reorder)); }

  } reg_in, reg_out;

  ReservationStation RS_in, RS_out;

  CircQueue<ROB> ROB_in, ROB_out;
  MemoryAccess MA;

  CircQueue<SLBElement> SLB_in, SLB_out;
  CDB to_issue, issue_to_rob, issue_to_rs, issue_to_slb, issue_to_reg;
  CDB to_execute, exec_to_rob, exec_to_rs;
  CDB slb_to_rob, slb_to_rs;
  CDB to_commit, commit_to_reg, commit_to_slb;
  bool commit_need_clear{0};  // broadcast

  void Issue() {
    issue_to_reg.is_stall = 1;
    issue_to_rob.is_stall = 1;
    issue_to_rs.is_stall = 1;
    issue_to_slb.is_stall = 1;
    if (to_issue.is_stall) return;
    // 将对应指令放到 RS 与 ROB 中.
    Instruct inst = to_issue.inst;
    issue_to_rob.to_ROB = {inst.type, 1, 0, inst.rd, 0};
    RSElement send;
    send.type = inst.type;
    send.A = inst.imm;
    send.isBusy = 1;
    send.dest = ROB_in.Next();
    if (reg_in.Reorder(inst.rs1))
      if (ROB_in[reg_in.Reorder(inst.rs1)].isReady)
        send.Vj = ROB_in[reg_in.Reorder(inst.rs1)].value, send.Qj = 0;
      else
        send.Qj = reg_in.Reorder(inst.rs1);
    else
      send.Vj = reg_in[inst.rs1], send.Qj = 0;
    if (reg_in.Reorder(inst.rs2))
      if (ROB_in[reg_in.Reorder(inst.rs2)].isReady)
        send.Vk = ROB_in[reg_in.Reorder(inst.rs2)].value, send.Qk = 0;
      else
        send.Qk = reg_in.Reorder(inst.rs2);
    else
      send.Vk = reg_in[inst.rs2], send.Qk = 0;
    issue_to_reg.is_stall = 0;
    issue_to_reg.to_reg = {inst.rd, 0, send.dest};

    if (LB <= inst.type && inst.type <= SW) {
      issue_to_slb.cur_pc = to_issue.cur_pc;
      issue_to_slb.to_SLB = send;
      issue_to_slb.is_stall = 0;
    } else {
      issue_to_rs.cur_pc = to_issue.cur_pc;
      issue_to_rs.to_RS = send;
      issue_to_rs.is_stall = 0;
    }
  }

  void Execute() {
    exec_to_rob.is_stall = 1;
    exec_to_rs.is_stall = 1;
    if (to_execute.is_stall) return;
    unsigned value, to_pc;
    switch (to_execute.to_exec.type) {
      case LUI:
        value = to_execute.to_exec.A;
        break;
      case AUIPC:
        value = to_execute.to_exec.A + to_execute.cur_pc;
        break;
      case JAL:
        value = to_execute.cur_pc + 4;
        to_pc = to_execute.to_exec.A + to_execute.cur_pc;
        break;
      case JALR:
        to_pc = to_execute.to_exec.Vj + to_execute.to_exec.A & -2;
        value = to_execute.cur_pc + 4;
        break;
      case BEQ:
        to_pc = to_execute.to_exec.Vj == to_execute.to_exec.Vk
                    ? to_execute.to_exec.A + to_execute.cur_pc
                    : -1;
        break;
      case BNE:
        to_pc = to_execute.to_exec.Vj != to_execute.to_exec.Vk
                    ? to_execute.to_exec.A + to_execute.cur_pc
                    : -1;
        break;
      case BLT:
        to_pc = (int)to_execute.to_exec.Vj < (int)to_execute.to_exec.Vk
                    ? to_execute.to_exec.A + to_execute.cur_pc
                    : -1;
        break;
      case BGE:
        to_pc = (int)to_execute.to_exec.Vj >= (int)to_execute.to_exec.Vk
                    ? to_execute.to_exec.A + to_execute.cur_pc
                    : -1;
        break;
      case BLTU:
        to_pc = to_execute.to_exec.Vj < to_execute.to_exec.Vk
                    ? to_execute.to_exec.A + to_execute.cur_pc
                    : -1;
        break;
      case BGEU:
        to_pc = to_execute.to_exec.Vj >= to_execute.to_exec.Vk
                    ? to_execute.to_exec.A + to_execute.cur_pc
                    : -1;
        break;
      case ADDI:
        value = to_execute.to_exec.Vj + to_execute.to_exec.A;
        break;
      case SLTI:
        value = (int)to_execute.to_exec.Vj < (int)to_execute.to_exec.A;
        break;
      case SLTIU:
        value = to_execute.to_exec.Vj < to_execute.to_exec.A;
        break;
      case XORI:
        value = to_execute.to_exec.Vj ^ to_execute.to_exec.A;
        break;
      case ORI:
        value = to_execute.to_exec.Vj | to_execute.to_exec.A;
        break;
      case ANDI:
        value = to_execute.to_exec.Vj & to_execute.to_exec.A;
        break;
      case SLLI:
        value = to_execute.to_exec.Vj << get(to_execute.to_exec.A, 4, 0);
        break;
      case SRLI:
        value = to_execute.to_exec.Vj >> get(to_execute.to_exec.A, 4, 0);
        break;
      case SRAI:
        value = (int)to_execute.to_exec.Vj >> get(to_execute.to_exec.A, 4, 0);
        break;
      case ADD:
        value = to_execute.to_exec.Vj + to_execute.to_exec.Vk;
        break;
      case SUB:
        value = to_execute.to_exec.Vj - to_execute.to_exec.Vk;
        break;
      case SLL:
        value = to_execute.to_exec.Vj << get(to_execute.to_exec.Vk, 4, 0);
        break;
      case SLT:
        value = (int)to_execute.to_exec.Vj < (int)to_execute.to_exec.Vk;
        break;
      case SLTU:
        value = to_execute.to_exec.Vj < to_execute.to_exec.Vk;
        break;
      case XOR:
        value = to_execute.to_exec.Vj ^ to_execute.to_exec.Vk;
        break;
      case SRL:
        value = to_execute.to_exec.Vj >> get(to_execute.to_exec.Vk, 4, 0);
        break;
      case SRA:
        value = (int)to_execute.to_exec.Vj >> get(to_execute.to_exec.Vk, 4, 0);
        break;
      case OR:
        value = to_execute.to_exec.Vj | to_execute.to_exec.Vk;
        break;
      case AND:
        value = to_execute.to_exec.Vj & to_execute.to_exec.Vk;
        break;
    }
    exec_to_rob.is_stall = 0;
    exec_to_rob.res = {value, to_pc, to_execute.to_exec.dest};
    exec_to_rs.is_stall = 0;
    exec_to_rs.res = {value, to_pc, to_execute.to_exec.dest};
  }

  void Commit() {
    commit_to_reg.is_stall = 1;
    commit_to_slb.is_stall = 1;
    commit_need_clear = 0;
    if (!to_commit.is_stall) {
      switch (to_commit.to_com.type) {
        case JAL ... JALR:
          pc = to_commit.to_com.to_pc, commit_need_clear = 1;
          break;
        case BEQ ... BGEU:
          if (~to_commit.to_com.to_pc)
            pc = to_commit.to_com.to_pc, commit_need_clear = 1;
          break;
        case SB ... SW:
          commit_to_slb.is_stall = 0;
          break;
        case HALT:
          std::cout << (reg_in[10] & 255u) << '\n';
          exit(0);
        default:
          commit_to_reg.to_reg.Q = to_commit.to_com.in_rob;
          commit_to_reg.to_reg.index = to_commit.to_com.dest;
          commit_to_reg.to_reg.value = to_commit.to_com.value;
      }
    }
  }

  void Update() {
    reg_in = reg_out;
    ROB_in = ROB_out;
    RS_in = RS_out;
    ROB_in = ROB_out;
    SLB_in = SLB_out;
  }

  void RunRob() {
    to_commit.is_stall = 1;
    if (commit_need_clear) {
      ROB_out.Clear();
      return;
    }
    if (!exec_to_rob.is_stall) {
      int i = exec_to_rob.res.in_rob;
      ROB_out[i].isReady = 1;
      ROB_out[i].value = exec_to_rob.res.value;
      ROB_out[i].to_pc = exec_to_rob.res.to_pc;
    }
    if (!slb_to_rob.is_stall) {
      int i = slb_to_rob.res.in_rob;
      ROB_out[i].isReady = 1;
      ROB_out[i].value = slb_to_rob.res.value;
    }
    if (ROB_in.Top().isReady) {
      to_commit.to_com = {ROB_in.Top().type, ROB_in.Top().dest,
                          ROB_in.Top().value, ROB_in.Top().to_pc,
                          ROB_in.TopId()};
      ROB_out.Pop();
    }
    if (!issue_to_rob.is_stall) ROB_out.Push(issue_to_rob.to_ROB);
  }
  void RunSlb() {
    /*
    2. 1）SLB每个周期会查看队头，若队头指令还未ready，则阻塞。

       2）当队头ready且是load指令时，SLB会直接执行load指令，包括计算地址和读内存，
       然后把结果通知ROB，同时将队头弹出。ROBcommit到这条指令时通知Regfile写寄存器。

       3）当队头ready且是store指令时，SLB会等待ROB的commit，commit之后会SLB执行这
       条store指令，包括计算地址和写内存，写完后将队头弹出。
    */
    slb_to_rob.is_stall = 1;
    slb_to_rs.is_stall = 1;
    if (commit_need_clear) {
      SLB_out.Clear(), MA.isBusy = 0;
      return;
    }
    if (!issue_to_slb.is_stall)
      SLB_out.Push(issue_to_slb.to_SLB);
    if (MA.isBusy) {
      --MA.last_time;
      if (!MA.last_time) {
        if (MA.opt.type == SB)
          mem[MA.opt.A] = MA.opt.Vk;
        else if (MA.opt.type == SH)
          *(uint16_t *)(mem + MA.opt.A) = MA.opt.Vk;
        else if (MA.opt.type == SW)
          *(uint32_t *)(mem + MA.opt.A) = MA.opt.Vk;
        else {
          int value;
          switch (MA.opt.type) {
            case LB:
              value = sign_extend(mem[MA.opt.Vj + MA.opt.A], 7);
              break;
            case LH:
              value =
                  sign_extend(*(uint16_t *)(mem + MA.opt.Vj + MA.opt.A), 15);
              break;
            case LW:
              value = *(uint32_t *)(mem + MA.opt.Vj + MA.opt.A);
              break;
            case LBU:
              value = mem[MA.opt.Vj + MA.opt.A];
              break;
            case LHU:
              value = *(uint16_t *)(mem + MA.opt.Vj + MA.opt.A);
              break;
          }
          slb_to_rob.is_stall = slb_to_rs.is_stall = 0;
          slb_to_rob.res.in_rob = slb_to_rs.res.in_rob = MA.opt.dest;
          slb_to_rob.res.value = slb_to_rs.res.value = value;
        }
        SLB_out.Pop();
        MA.isBusy = 0;
      }
    }
    if (!SLB_in.Empty()) {
      if (SLB_in.Top().isBusy && !SLB_in.Top().Qj && !SLB_in.Top().Qk) {
        if (SLB_in.Top().type >= SB) {  // store
          slb_to_rob.res.in_rob = SLB_in.Top().dest;
          slb_to_rob.res.value = SLB_in.Top().Vj + SLB_in.Top().A;
          SLB_out.Top().isBusy = 0;
        } else if (!MA.isBusy) {
          MA.Add(SLB_in.Top()), SLB_out.Top().isBusy = 0;
        }
      } else {
        if (!commit_to_slb.is_stall) SLB_out.Top().isReady = 1;
        if (SLB_in.Top().type >= SB && SLB_in.Top().isReady && !MA.isBusy)
          MA.Add(SLB_in.Top()), SLB_out.Top().isReady = 0;
      }
    }
    if (!exec_to_rs.is_stall)
      for (int i = 1; i <= QUEUE_SIZE; ++i) {
        if (!SLB_in[i].isBusy) continue;
        if (SLB_in[i].Qj == exec_to_rs.res.in_rob)
          SLB_out[i].Qj = 0, SLB_out[i].Vj = exec_to_rs.res.value;
        if (SLB_in[i].Qk == exec_to_rs.res.in_rob)
          SLB_out[i].Qk = 0, SLB_out[i].Vk = exec_to_rs.res.value;
      }
  }
  void RunRs() {
    to_execute.is_stall = 1;
    if (commit_need_clear) {
      RS_out.Clear();
      return;
    }
    if (!issue_to_rs.is_stall) RS_out.Add(issue_to_rs.to_RS);
    int i = RS_out.Check();
    if (i) {
      to_execute.is_stall = 0;
      to_execute.cur_pc = issue_to_rs.cur_pc;
      to_execute.to_exec = RS_out[i];
      RS_out[i].isBusy = 0;
    }
    if (!exec_to_rs.is_stall)
      for (int i = 1; i <= QUEUE_SIZE; ++i) {
        if (!RS_in[i].isBusy) continue;
        if (RS_in[i].Qj == exec_to_rs.res.in_rob)
          RS_out[i].Qj = 0, RS_out[i].Vj = exec_to_rs.res.value;
        if (RS_in[i].Qk == exec_to_rs.res.in_rob)
          RS_out[i].Qk = 0, RS_out[i].Vk = exec_to_rs.res.value;
      }
    if (!slb_to_rs.is_stall)
      for (int i = 1; i <= QUEUE_SIZE; ++i) {
        if (!RS_in[i].isBusy) continue;
        if (RS_in[i].Qj == slb_to_rs.res.in_rob)
          RS_out[i].Qj = 0, RS_out[i].Vj = slb_to_rs.res.value;
        if (RS_in[i].Qk == slb_to_rs.res.in_rob)
          RS_out[i].Qk = 0, RS_out[i].Vk = slb_to_rs.res.value;
      }
  }
  void RunReg() {
    if (commit_need_clear) {
      reg_out.Clear();
      return;
    }
    if (!commit_to_reg.is_stall) {
      int index = commit_to_reg.to_reg.index;
      if (commit_to_reg.to_reg.Q == reg_in.Reorder(index))
        reg_out.Reorder(index) = 0;
      reg_out[index] = commit_to_reg.to_reg.value;
    }
    if (!issue_to_reg.is_stall)
      reg_out.Reorder(issue_to_reg.to_reg.index) = issue_to_reg.to_reg.Q;
  }
  void RunInQueue() {
    // 在这里判断是否可以发送指令，否则 stall.
    to_issue.is_stall = 0;
    if (RS_in.Full() || ROB_in.Full() || SLB_in.Full()) {
      to_issue.is_stall = 1;
      return;
    }
    to_issue.cur_pc = pc;
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