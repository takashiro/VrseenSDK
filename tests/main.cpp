#include "test.h"

#include <time.h>
#include <iostream>

using namespace std;

list<TestUnit> &Tests()
{
    static list<TestUnit> tests;
    return tests;
}

int main()
{
    cout << "NervGear Test" << endl;
    cout << "======================" << endl;
    srand(time(nullptr));

    const list<TestUnit> &tests = Tests();
    for (const TestUnit &test : tests) {
        cout << "Testing " << test.name << " ..." << endl;
        (*test.function)();
        cout << test.name << " has been tested." << endl;
    }

    cout << "======================" << endl;
    cout << "Everything works fine!" << endl << endl;

    return 0;
}
