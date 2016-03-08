#include "test.h"
#include <VArray.h>

NV_USING_NAMESPACE

#define VARRAY_LENGTH 1000

namespace {

void test()
{
    {
        int nums[VARRAY_LENGTH];
        VArray<int> array;
        for (int &num : nums)
        {
            num = rand() % 100;
            array.append(num);
        }

        assert(array.length() == VARRAY_LENGTH);
        assert(array.size() == VARRAY_LENGTH);
        for (uint i = 0; i < VARRAY_LENGTH; i++) {
            assert(nums[i] == array[i]);
            assert(nums[i] == array.at(i));
        }

        array.removeAt(0);
        assert(array[0] == nums[1]);

        array.removeAll(nums[1]);
        assert(!array.contains(nums[1]));

        VArray<int> copy1 = array;
        copy1.append(array);

        VArray<int> copy2 = array;
        copy2.prepend(array);

        assert(copy1.size() == copy2.size());
        for (uint i = 0, max = copy1.size(); i < max; i++) {
            assert(copy1.at(i) == copy2.at(i));
        }
    }

    {
        VArray<int> array;
        array << 1 << 2 << 3;
        assert(array.size() == 3);
        assert(array[0] == 1);
        assert(array[1] == 2);
        assert(array[2] == 3);

        array.clear();
        assert(array.empty());
    }
}

ADD_TEST(VArray, test)

}
