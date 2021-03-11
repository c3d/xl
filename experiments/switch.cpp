#include <cstdio>
#include <vector>
#include <cstdint>
#include <cstdlib>

struct RunState;
typedef intptr_t opcode_fn;
typedef intptr_t data;

struct Bytecode
{
    std::vector<opcode_fn> code;
    void Run(RunState &state);
    void Op(opcode_fn op) { code.push_back(op); }
    data Data(size_t pc) { return (data) code[pc]; }
    data Label() { return code.size(); }
    void Enter(data n) { code.push_back((opcode_fn) n); }
    void Patch(data pc) { code[pc] = (opcode_fn) (code.size() - (pc+1)); }
    void Jump(data label) { Enter(label - (code.size() + 1)); }

    enum Opcode
    {
        dup, drop, swap, add, sub, cst, eq, jfalse, jump, call, ret
    };
};

struct RunState
{
    RunState(): pc(0), bytecode(), transfer() {}
    size_t pc;
    Bytecode *bytecode;
    Bytecode *transfer;
    std::vector<data> stack;
    data Pop() { data r = stack.back(); Show("pop"); stack.pop_back(); return r; }
    void Push(data n) { stack.push_back(n); Show("push");  }
    data Data() { return bytecode->Data(pc++); }
    void Show(const char *msg)
    {
        return;
        printf("%s:", msg);
        for (auto x : stack) printf(" %ld", x);
        printf("\n");
    }

};


#define OPCODE(n)   break; case n:
#define dprintf(...)


void Bytecode::Run(RunState &state)
{
    Bytecode *bc = this;
    while (bc)
    {
        data max = bc->code.size();
        state.pc = 0;
        state.bytecode = bc;
        state.transfer = nullptr;
        while (state.pc < max)
        {
            switch(bc->code[state.pc++])
            {
            default:
                OPCODE(dup)
                {
                    dprintf("dup\n");
                    data x = state.Pop();
                    state.Push(x);
                    state.Push(x);
                }
            OPCODE(drop)
            {
                dprintf("drop\n");
                state.Pop();
            }
            OPCODE(swap)
            {
                data y = state.Pop();
                data x = state.Pop();
                dprintf("swap %ld %ld\n", x, y);
                state.Push(y);
                state.Push(x);
            }

            OPCODE(add)
            {
                data y = state.Pop();
                data x = state.Pop();
                dprintf("add %ld+%ld\n", x, y);
                state.Push(x+y);
            }

            OPCODE(sub)
            {
                data y = state.Pop();
                data x = state.Pop();
                dprintf("sub %ld-%ld\n", x, y);
                state.Push(x-y);
            }

            OPCODE(cst)
            {
                data x = state.Data();
                dprintf("cst %ld\n", x);
                state.Push(x);
            }

            OPCODE(eq)
            {
                data y = state.Pop();
                data x = state.Pop();
                dprintf("eq %ld %ld\n", x, y);
                state.Push(x==y);
            }

            OPCODE(jfalse)
            {
                data x = state.Pop();
                data b = state.Data();
                dprintf("jfalse %ld %ld\n", x, b);
                if (!x)
                    state.pc += b;
            }

            OPCODE(jump)
            {
                data j = state.Data();
                dprintf("jump %ld to %ld\n", j, state.pc + j);
                state.pc += j;
            }

            OPCODE(call)
            {
                data arg = state.Pop();
                data j = state.Data();
                state.Push(state.pc);
                state.Push(arg);
                dprintf("(%ld) call %ld %ld\n", state.pc, arg, j);
                state.pc += j;
            }

            OPCODE(ret)
            {
                data result = state.Pop();
                data pc = state.stack.size() > 0 ? state.Pop() : 0xFFFFFFF;
                dprintf("ret %ld to %ld\n", result, pc);
                state.pc = pc;
                state.Push(result);
            }
            }
        }

        bc = state.transfer;
    }
}

#define OP(n)   bytecode->Op(Bytecode::n)
#define JUMP(n) bytecode->Jump(n);
#define DATA(n) bytecode->Enter(n);
#define LABEL   bytecode->Label()
#define PATCH(l) bytecode->Patch(l);

Bytecode *fib()
{
    Bytecode *bytecode = new Bytecode;
    data start = LABEL;
    OP(dup);
    OP(cst); DATA(0);
    OP(eq);
    OP(jfalse); data l0 = LABEL; DATA(0);
    OP(drop);
    OP(cst); DATA(1);
    OP(ret);
    PATCH(l0);
    OP(dup);
    OP(cst); DATA(1);
    OP(eq);
    OP(jfalse); data l1 = LABEL; DATA(0);
    OP(drop);
    OP(cst); DATA(1);
    OP(ret);
    PATCH(l1);
    OP(dup);
    OP(cst); DATA(1);
    OP(sub);
    OP(call); JUMP(start);
    OP(swap);
    OP(cst); DATA(2);
    OP(sub);
    OP(call); JUMP(start);
    OP(add);
    OP(ret);

    return bytecode;
}


int main(int argc, char *argv[])
{
    Bytecode *bc = fib();
    RunState state;
    for (int a = 1; a < argc; a++)
    {
        data v = atoi(argv[a]);
        state.Push(v);
        bc->Run(state);
        data result = state.Pop();
        printf("fib(%ld)=%ld\n", v, result);
    }
}
