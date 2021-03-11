#include <cstdio>
#include <cstdlib>

long fib(long n)
{
    if (n == 0)
        return 1;
    if (n == 1)
        return 1;
    return fib(n-1) + fib(n-2);
}

int main(int argc, char **argv)
{
    for (int a = 1; a < argc; a++)
    {
        long v = atoi(argv[a]);
        long result = fib(v);
        printf("fib(%ld)=%ld\n", v, result);
    }
}
