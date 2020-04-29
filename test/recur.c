int fib(int n) {
    if (n <= 1)
        return n;
    return fib(n - 1) + fib(n - 2);
}

int main(int param) {
    int i = 0;
    putstr("recur.c");
    if (param == 0)
        param = 10;
    while (i <= param) {
        putnum(fib(i));
        i = i + 1;
    }
    //Here's the comment test!
    return 0;
}
