// Positive controls: these deliberate errors must be detected by the runtime.
#if defined(SLONYARA_PROBE_MEMORY)
#  include <sanitizer/msan_interface.h>
#  include <cstdio>

int main()
{
    int value = 42;
    __msan_poison(&value, sizeof(value));
    if (value == 42)
        std::puts("The sanitizer should have stopped before this branch");
}
#else
#  include <thread>

int main()
{
    int shared = 0;
    auto write = [&] {
        for (int i = 0; i < 10000; ++i)
            shared = i;
    };
    std::thread first(write);
    std::thread second(write);
    first.join();
    second.join();
    return shared == 9999 ? 0 : 1;
}
#endif
