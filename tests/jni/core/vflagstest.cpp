#include "test.h"

#include <VFlags.h>

using namespace std;
NV_USING_NAMESPACE

namespace {

void test()
{
    enum Module
    {
        M1,
        M2,
        M3,
        M4,

        ModuleCount
    };

    VFlags<Module> flags(M1);
    assert(flags.contains(M1));
    assert(flags == 1);

    flags |= M2;
    assert(flags.contains(M1) && flags.contains(M2));
    assert(flags == 3);

    flags.set(M4);
    assert(flags.contains(M1) && flags.contains(M2) && flags.contains(M4));

    flags.unset(M1);
    assert(!flags.contains(M1));

    flags &= M1;
    assert(flags == 0);

    flags ^= M3;
    for (int m = M1; m < ModuleCount; m++) {
        assert(flags.contains(static_cast<Module>(m)) == (m == M3));
    }

    flags ^= M2;
    assert(flags.contains(M2));
    assert(flags.contains(M3));
    assert(!flags.contains(M1));
    assert(!flags.contains(M4));

    flags ^= M2;
    assert(!flags.contains(M2));
    assert(flags.contains(M3));
    assert(!flags.contains(M1));
    assert(!flags.contains(M4));
}

ADD_TEST(VFlags, test)

}
