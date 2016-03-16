
/*
 * VArray.h
 *
 *  Created on: 2016/3/6
 *      Author: gaojialing
 */

#pragma once
#include <vector>
#include <utility>
#include "vglobal.h"
#include "List.h"
using namespace std;
NV_NAMESPACE_BEGIN
template <class E> class VArray : public vector<E>
{
public:
    typename VArray<E>::iterator iter;
    typedef E ValueType;
	//Array ( const vector<T,Allocator>& x );
	VArray<E>(int size):vector<E>(size)
	{

	}
	VArray<E>(){}
//=====================
	//typedef E T;
	//VArray<VArray<E>>(){}
	int length() const
	{
		return (int)this->size();
	}
	 int capacityInt() const
	 {
	     return (int)this->capacity();
	 }
	   void  popBack()
	   {
	      this->pop_back();
	    }
	    uint allocBack()
	    {
	        uint size_temp=this->size();
	        this->resize(size_temp+1);
	        return size_temp;
	    }
	bool isEmpty() const
	{
		return this->empty();
	};

	 VArray &operator << (const E &e)
	 {
	     this->append(e);
	     return *this;
	 }
	 VArray &operator << (const VArray<E> &elements)
	 {
	     append(elements);
	     return *this;
	 }
	const E &first() const
	{
		return this->front();
	}
	E &first()
	{
	    return this->front();
	}

	const E &last() const
	{
	    return this->back();
	}
	E &last()
	{
		return this->back();
	}

	E &operator[](int i)
	{
		return this->at(i);
	}
	const E &operator[](int i) const
	{
		return this->at(i);
	}
	void append(const E &e)
	{
       this->push_back(e);
	};
	void append(const VArray<E> &elements)
	{
		for(E e:elements)
		{
			this->push_back(e);
		}
	}
	void prepend(const E &e)
	{
	    this->insert(this->begin(), e);
	}
	void prepend(const VArray<E> &elements)
	{
	    for (auto i = elements.rbegin();i != elements.rend();i++) {
	        this->insert(this->begin(), *i);
	    }
	}

	void removeAt(int i)
	{
	    this->erase(this->begin() + i);
	}
	void removeOne(const E &e)
	{
	    for (auto i = this->begin();
	            i != this->end();
	            i++) {
	        if (*i == e) {
	            this->erase(i);
	            break;
	        }
	    }
	}
	void removeAll(const E &e)
	{
	   this->clear();
	};
	bool contains(const E &e) const
	{
	    for(auto i:*this) {
	        if (*i == e) {
	            return true;
	        }
	    }
	    return false;
	}
	void clearAndRelease()
	{
       this->clear();
	}

	void removeAtUnordered(uint index)
	{
	    OVR_ASSERT(index < this->size());
	    if (this->size() == 1) {
	        this->clear();
	        return;
	    }
	    swap((*this)[index], this->back());
	    this->pop_back();
	}

	const E* dataPtr() const { return this->data(); }
	      E* dataPtr()       { return this->data(); }

	};
NV_NAMESPACE_END

