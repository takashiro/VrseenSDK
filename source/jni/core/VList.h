/*
 * VList.h
 *
 *  Created on: 2016年3月2日
 *      Author: yangkai
 */

#pragma once
#include <list>
#include "vglobal.h"
#include "logcat_cpp.h"
#define LOG_TAG "JNI_Tests"
#define LOGD(...) _LOGD(LOG_TAG, __VA_ARGS__)
using namespace std;

NV_NAMESPACE_BEGIN
//使用VList的类型应该继承该模板；比如 class X:public NodeOfVList<VList<X>>
template <typename Container> class NodeOfVList
{
public:
    Container* pointToVList;
};
template <class E> class VList : public list<E>
{
public:
    bool isEmpty() const
    {
        return this->empty();
    }

    const E &first() const
    {
        return this->front();
    }

    const E &last() const
    {
        return this->back();
    }

    E &first()
    {
        return this->front();
    }

    E &last()
    {
        return this->back();
    }

    bool isFirst(typename VList<E>::const_iterator ci) const
    {
        if (ci == this->begin()) {
            return true;
        }
        return false;
    }

    bool isLast(typename VList<E>::const_iterator ci) const
    {
        typename VList<E>::const_iterator iter = this->end();
        iter--;
        if (ci == iter) {
            return true;
        }
        return false;
    }

    void append(const E &e)
    {
        this->push_back(e);
    }

    void append(const VList<E> &elements)
    {
        for (E e : elements) {
            this->push_back(e);
        }
    }

    void prepend(const E &e)
    {
        this->push_front(e);
    }

    void prepend(const VList<E> &elements)
    {
        for (typename VList<E>::reverse_iterator ri = elements.rbegin();
                ri != elements.rend();ri++) {
            this->push_front(*ri);
        }
    }

    void bringToFront(typename VList<E>::iterator ci)
    {
        E temp = *ci;
        this->erase(ci);
        this->push_front(temp);
    }

    void sendToBack(typename VList<E>::iterator ci)
    {
        E temp = *ci;
        this->erase(ci);
        this->push_back(temp);
    }

    void removeOne(const E &e)
    {
        for (typename VList<E>::iterator i = this->begin();i != this->end();i++) {
            if (*i == e) {
                this->erase(i);
                break;
            }
        }
    }

    void removeAll(const E &e)
    {
        this->remove(e);
    }

    bool contains(const E &e) const
    {
        for (E i:*this) {
            if (i == e) {
                return true;
            }
        }
        return false;
    }

    //把s在iFirst(包括)之后的元素整体移接到本链表的前面，并删除
    void pushFollowingListItemsToFront(VList<E> &s, typename VList<E>::const_iterator iFirst)
    {
        for (typename VList<E>::const_iterator ci = (s.end() - 1);
                ci >= iFirst;
                ci--) {
            this->push_front(*ci);
        }
        s.erase(iFirst,s.end());
    }

    //把s从开始到iLast(不包括)的元素整体移接到本链表的前面，并删除
    void pushPrecedingListItemsToFront(VList<E> &s,typename VList<E>::const_iterator iLast)
    {
        for (typename VList<E>::const_iterator ci = iLast - 1;
                ci >= s.begin();ci--) {
            this->push_front(*ci);
        }
        s.erase(s.begin(),iLast);
    }

    //将iFirst(包括)和iLast(不包括)整体移接到本链表的前面，并从原链表中删除
    void pushListItemsToFront(VList<E> &s,typename VList<E>::const_iterator iFirst,typename VList<E>::const_iterator iLast)
    {
        for (typename VList<E>::const_iterator ci = iLast - 1;
                ci >= iFirst;
                ci--) {
            this->push_front(*ci);
        }
        s.erase(iFirst,iLast);
    }

    void allocMoveTo(VList<E> &s)
    {
        this->swap(s);
        this->clear();
    }
};
NV_NAMESPACE_END
