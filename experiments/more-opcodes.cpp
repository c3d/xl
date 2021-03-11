#include <cstdio>
#include <cassert>
#include <vector>

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
        copy, add, add_load, sub, sub_cst, sub_load_cst,
        cstx, csty, jne_cst, jump,
        call, call1x, ret, ret_cst
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

            OPCODE(add_load)
                y = LOCAL;
            OPCODE(add)
            {
                x = x + y;
                break;
            }

            OPCODE(sub_load_cst)
                x = LOCAL;
            OPCODE(sub_cst)
                y = DATA;
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

            OPCODE(jne_cst)
            {
                y = DATA;
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

            OPCODE(call1x)
            {
                data b = DATA;
                data jpc = pc;
                stack.Push(pc);
                stack.Push(locals);
                data base = stack.Size();
                stack.Push(x);
                pc = jpc + b;
                locals = base;
                REFRAME;
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

            OPCODE(ret_cst)
                x = DATA;
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
        OP(jne_cst, 0, not0);
        OP(ret_cst, 1);
    }
    LABEL(not0);
    {
        OP(jne_cst, 1, not1);
        OP(cstx, 1);
        OP(ret_cst, 1);
    }
    LABEL(not1);
    {
        OP(sub_cst, 1);
        OP0(alloc)              // [N, N-1]
        OP(call1x, start);      // [N, N-1]
        OP0(alloc);             // [N, N-1, fib(N-1)]
        OP(sub_load_cst, 0, 2);
        OP(call1x, start);
        OP(add_load, 2);
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
