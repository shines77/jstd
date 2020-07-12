
#include <stdlib.h>
#include <stdio.h>

#include <jstd/all.h>

class A {
    //
};

int main(int argc, char *argv[])
{
    bool a = jstd::is_iterator<A>::value;
    printf("dict_test.exe - %d\n\n", (int)a);
    return 0;
}
