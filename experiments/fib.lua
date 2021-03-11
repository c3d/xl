function fib(m)
    if m == 0 then
      return 1
    end
    if m == 1 then
      return 1
    end
    return fib(m-1) + fib(m-2)
end

print(fib(40))
