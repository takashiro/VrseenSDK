/*
 * JNI_Tests.cpp
 *
 *  Created on: 2016年3月9日
 *      Author: yangkai
 */

#include "test.h"

#include <string>
#include <VList.h>

using namespace std;
NV_USING_NAMESPACE

namespace {

class StrNode: public NodeOfVList<VList<StrNode*>>
{
public:
    string* pstr;//需要自己释放
    StrNode(string *p):pstr(p)
    {
    }

    bool operator == (const StrNode& a)
    {
        return *(a.pstr) == *(this->pstr);
    }
};

void testVList()
{
    VList<StrNode*> tester;
    StrNode* pStr = nullptr, *tmp = nullptr;

    assert(tester.isEmpty());
    pStr = new StrNode(new string("tester2"));
    pStr->pointToVList = &tester;
    tester.append(pStr);

    pStr = new StrNode(new string("tester3"));
    pStr->pointToVList = &tester;
    tester.append(pStr);

    pStr = new StrNode(new string("tester1"));
    pStr->pointToVList = &tester;
    tester.prepend(pStr);
    tmp = pStr;

    pStr = new StrNode(new string("tester0"));
    pStr->pointToVList = &tester;
    tester.prepend(pStr);

    tmp = tester.getNextByContent(tmp);
    assert(tmp->pstr->compare("tester2") == 0);
    assert(!tester.isEmpty());

    assert(4 == tester.size());

    assert(tester.first()->pstr->compare("tester0") == 0);
    assert(tester.last()->pstr->compare("tester3") == 0);

    VList<StrNode*>::iterator iter0 = tester.end();
    iter0--;

    tester.bringToFront(iter0);
    assert(tester.first()->pstr->compare("tester3") == 0);

    tester.sendToBack(tester.begin());
    assert(tester.last()->pstr->compare("tester3") == 0);

    assert(!tester.contains(new StrNode(new string("tester2"))));
    assert(tester.contains(tester.front()));

    StrNode* p = tester.front();
    p->pointToVList->remove(p);
    assert(tester.front()->pstr->compare("tester1") == 0);

    tester.back()->pointToVList->remove(tester.back());
    assert(tester.back()->pstr->compare("tester2") == 0);

    assert(2 == tester.size());
}

ADD_TEST(VList, testVList)

} //namespace
