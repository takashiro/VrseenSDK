
/*
 * VArray.h
 *
 *  Created on: 2016/3/6
 *      Author: gaojialing
 */

#pragma once
#include <vector>
#include "vglobal.h"
#include "List.h"
using namespace std;
NV_NAMESPACE_BEGIN
template <class E> class VArray : public vector<E>
{
public:
	//Array ( const vector<T,Allocator>& x );
	VArray<E>(int size):vector<E>(size)
	{

	}
	VArray<E>(){}
	int length() const
	{
		return this->size();
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
	void append(const List<E> &elements)
	{
		for(E e:elements)
		{
			this->push_back(e);
		}
	}
	void prepend(const E &e)
	{
	    insert(0, e);
	}
	void prepend(const List<E> &elements)
	{
	    for (uint i = 0, max = elements.size(); i < max; i++) {
	                 insert(i, elements.at(i));
	            }
	}
	void insert(uint i, const E &e)
	{
	    std::vector<E>::insert(this->cbegin() + i, e);
	}
	void removeAt(int i)
	{
	    this->erase(this->begin() + i);
	}
	void removeOne(const E &e)
	{
	    for (uint i = 0, max = this->size(); i < max; i++) {
	               if (this->at(i) == e) {
	                     removeAt(i);
	                     break;
	                 }
	            }
	}
	void removeAll(const E &e)
	{
	    for (uint i = 0, max = this->size(); i < max; i++) {
	                 if (this->at(i) == e) {
	                     removeAt(i);
	                     i--;
	                     max--;
	                 }
	             }

	};
	bool contains(const E &e) const
	{
	    for (uint i = 0, max = this->size(); i < max; i++) {
	                 if (this->at(i) == e) {
	                     return true;
	                 }
	             }
	             return false;

	};
	void clearAndRelease()
	{
		this->clear();
	}
//
	void removeAtUnordered()
	{
		this->removeAtUnordered();
	}
//
	void removeAtUnordered(int i)
		{
			this->removeAtUnordered(i);
		}
	const E* dataPtr() const { return this->data(); }
	      E* dataPtr()       { return this->data(); }
//
	uint allocBack()
	  	{
	  		return this->allocBack();
	  	}
	};
NV_NAMESPACE_END
