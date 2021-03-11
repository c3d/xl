#include <cstdio>
#include <cstdlib>

int counter = 0;

long fib(long n)
{
    if (n == 0)
        return counter + 1;
    if (n == 1)
        return 1;
    return fib(n-1) + fib(n-2);
}

int main(int argc, char **argv)
{
    for (int a = 1; a < argc; a++)
    {
        long v = atoi(argv[a]);
        long sum = 0;
        for (counter = 25; counter > 0; counter--)
            sum += fib(v);
        long result = fib(v);
        printf("fib(%ld)=%ld sum=%ld\n", v, result, sum);
    }
}
