#include <iostream>
#include <sstream>
#include <time.h>

#include "JSON.h"

using namespace std;
using namespace NervGear;

int main()
{
    srand(time(NULL));

    {
        stringstream s;
        s << "null";
        Json json;
        s >> json;
        cout << "Expected: null" << endl;
        cout << "  Output: " << json << endl;
    }

    {
        stringstream s;
        s << " null ";
        Json json;
        s >> json;
        cout << "Expected: null" << endl;
        cout << "  Output: " << json << endl;
    }

    {
        stringstream s;
        int value = rand();
        s << value;
        Json json;
        s >> json;
        cout << "Expected: " << value << endl;
        cout << "  Output: " << json << endl;
    }

    {
        stringstream s;
        double value = rand() / (double) rand();
        s << value;
        Json json;
        s >> json;
        cout << "Expected: " << value << endl;
        cout << "  Output: " << json << endl;
    }

    {
        stringstream s;
        s << "{\"test\" : [1,2,3]}";
        Json json;
        s >> json;
        cout << "Expected: 10" << endl;
        cout << "  Output: " << json.contains("test") << json.contains("what") << endl;
    }

    const char *values[] = {
        "true",
        "false",
        "[0,1,2,3,4,5,6]",
        "{\"haha\" : 789, \"test\": \"What \\\"know\\\".\"}",
        "[{\"haha\" : 789, \"test\": \"What\"}, 3, 46, 5]",
        "{\"array\" : [0,1,2,3,[4,3,2],5,6], \"number\" : 123456}"
    };

    for (const char *value : values) {
        stringstream s;
        s << value;
        Json json;
        s >> json;
        cout << "Expected: " << value << endl;
        cout << "  Output: " << json << endl;
    }

    return 0;
}
