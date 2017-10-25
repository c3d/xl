#include "gc.h"
#include <iostream>

struct Test
{
    Test()      { std::cerr << "Test::Test\n"; }
    ~Test()     { std::cerr << "Test::~Test\n"; }

    void Do()   { std::cerr << "Test::Do\n"; }

    GARBAGE_COLLECT(Test);
};

typedef XL::GCPtr<Test> Test_p;

struct Derived : Test
{
    Derived(Test *g, Test *u): glop(g), glap(u) { std::cerr << "Derived::Derived\n"; }
    ~Derived()  { std::cerr << "Derived::~Derived\n"; }

    Test_p glop;
    Test_p glap;

    GARBAGE_COLLECT(Derived);
};

typedef XL::GCPtr<Derived> Derived_p;


int main()
{
    Test_p ptr = new Test;
    Derived_p ptr2 = new Derived(ptr, ptr);
    ptr2->Do();
    new Derived(0, 0);

    for (uint i = 0; i < 2030; i++)
    {
        new Test();
        if (i > 2000)
            XL::GarbageCollector::Collect();
    }
    XL::GarbageCollector::Collect(true);
    XL::GarbageCollector::Collect(true);
}
