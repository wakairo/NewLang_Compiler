#define main p752_oom_main
#include "../p752/unit_test.c"
#undef main
int main(int argc, char **argv)
{
    return argc == 2 && oom(argv[1]) ? 0 : 1;
}
