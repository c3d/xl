#include <cstdio>
#include <cassert>
#include <vector>
#include <cstdint>
#include <cstdlib>

struct Stack;
typedef signed char byte;
typedef byte opcode;
typedef intptr_t data;

struct Jump {
    Jump():pc(-1), target(-1) {}
    opcode Distance()
    {
        data d = target - (pc + 1);
        byte b = d;
        assert (b == d);
        return b;
    }
    data pc, target;
};

struct Bytecode
{
    std::vector<opcode> code;
    data Run(Stack &stack);
    void Op(opcode op)  { code.push_back(op); }
    void Op(const Jump &jc)
    {
        Jump &j = *((Jump *) &jc);
        j.pc = code.size();
        if (j.target < 0)
            code.push_back(0);
        else
            code.push_back(j.Distance());
    }
    template<typename ...Args>
    void Op(opcode op, const Args & ... a)
    {
        Op(op);
        Op(a...);
    }
    template<typename ...Args>
    void Op(const Jump &j, const Args & ... a)
    {
        Op(j);
        Op(a...);
    }
    void Label(Jump &j)
    {
        j.target = code.size();
        if (j.pc >= 0)
            code[j.pc] = j.Distance();
    }

    enum Opcode
    {
        loadx, storex, loady, storey, alloc,
        copy, add, sub, cstx, csty, jne, jump, call, ret
    };
};

struct Stack
{
    Stack() {}
    std::vector<data> stack;
    data Pop() { data r = stack.back(); stack.pop_back(); return r; }
    void Push(data n) { stack.push_back(n);  }
    data &operator[](data n) { return stack[n]; }
    size_t Size() { return stack.size(); }
    void Cut(size_t n) { stack.erase(stack.begin() + n, stack.end()); }
};


#define OPCODE(n)       case n:
#define DATA            (code[pc++])
#define LOCAL           (plocals[DATA])
#define REFRAME         (plocals = &stack[locals])
#define RETARGET        (max = bc->code.size(), code = &bc->code[0])
#define dprintf(...)


data Bytecode::Run(Stack &stack)
{
    Bytecode *bc = this;
    data max = bc->code.size();
    opcode *code = &bc->code[0];
    data locals = 0;
    data *plocals;
    data pc = 0;
    data x=0, y=0;
    REFRAME;

    while (pc < max)
    {
        switch((enum Opcode) code[pc++])
        {
            OPCODE(loadx)
            {
                x = LOCAL;
                break;
            }
            OPCODE(loady)
            {
                y = LOCAL;
                break;
            }
            OPCODE(storex)
            {
                LOCAL = x;
                break;
            }
            OPCODE(storey)
            {
                LOCAL = x;
                break;
            }
            OPCODE(alloc)
            {
                stack.Push(x);
                REFRAME;
                break;
            }
            OPCODE(copy)
            {
                y = x;
                break;
            }

            OPCODE(add)
            {
                x = x + y;
                break;
            }

            OPCODE(sub)
            {
                x = x - y;
                break;
            }

            OPCODE(cstx)
            {
                x = DATA;
                break;
            }

            OPCODE(csty)
            {
                y = DATA;
                break;
            }

            OPCODE(jne)
            {
                data b = DATA;
                if (x != y)
                    pc += b;
                break;
            }

            OPCODE(jump)
            {
                data b = DATA;
                pc += b;
                break;
            }

            OPCODE(call)
            {
                data b = DATA;
                data jpc = pc;
                size_t args = DATA;
                stack.Push(pc + args);
                stack.Push(locals);
                data base = stack.Size();
                for (size_t a = 0; a < args; a++)
                {
                    REFRAME;
                    stack.Push(LOCAL);
                }
                pc = jpc + b;
                locals = base;
                REFRAME;
                break;
            }

            OPCODE(ret)
            {
                stack.Cut(locals);
                if (locals)
                {
                    locals = stack.Pop();
                    pc = stack.Pop();
                }
                else
                {
                    pc = max;
                }
                REFRAME;
                break;
            }
        }
    }
    return x;
}

#define OP(n, ...)      bytecode->Op(Bytecode::n, __VA_ARGS__)
#define OP0(n)          bytecode->Op(Bytecode::n);
#define LABEL(name)     bytecode->Label(name);

Bytecode *fib()
{
    Bytecode *bytecode = new Bytecode;
    Jump start, not0, not1;

    LABEL(start);               // [N]
    {
        OP(loadx, 0);
        OP(csty, 0);
        OP(jne, not0);
        OP(cstx, 1);
        OP0(ret);
    }
    LABEL(not0);
    {
        OP(csty, 1);
        OP(jne, not1);
        OP(cstx, 1);
        OP0(ret);
    }
    LABEL(not1);
    {
        OP(csty, 1);
        OP0(sub);
        OP0(alloc)              // [N, N-1]
        OP(call, start, 1, 1);  // [N, N-1]
        OP0(alloc);             // [N, N-1, fib(N-1)]
        OP(loadx, 0);
        OP(csty, 2);
        OP0(sub);
        OP0(alloc);             // [N, N-1, fib(N-1), N-2]
        OP(call, start, 1, 3);
        OP(loady, 2);
        OP0(add);
        OP0(ret);
    }

    return bytecode;
}


int main(int argc, char *argv[])
{
    Bytecode *bc = fib();
    Stack stack;
    for (int a = 1; a < argc; a++)
    {
        data v = atoi(argv[a]);
        stack.Push(v);
        data result = bc->Run(stack);
        printf("fib(%ld)=%ld\n", v, result);
    }
}
