
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
	}//鐜版湁Array璇ュ嚱鏁板悕瀛楁槸sizeInt()锛岄』淇敼
	//uint size() const; //uint瀹氫箟鍦╲global.h涓紝鍗硊nsigned int
	bool isEmpty() const
	{
		return this->empty();
	};

	/*const E &begin() const
	{
		return this->begin();
	};

	const E &end() const
	{
		return this->end();
	};*/

//	VArray<E>::iterator begin(){
//		return this->begin();
//	}

	const E &first() const
	{
		return this->front();
	};
	E &first();

	const E &last() const
	{
		return this->back();
	};
	E &last()
	{
		return this->back();
	};

	E &operator[](int i)
	{
		return this->operator [](i);
	};
	const E &operator[](int i) const
	{
		return this->operator [](i);
	};
	const E &at(int i) const
	{
		return this->at(i);
	};

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


	}; //鎻掑叆澶氫釜鍏冪礌鑷虫湯灏�

	void prepend(const E &e)
	{
		this->prepend(e);
	}; //鎻掑叆鍏冪礌鑷冲紑澶�
	void prepend(const List<E> &elements)
	{
		this->prepend(elements);

	}; //鎻掑叆澶氫釜鍏冪礌鑷冲紑澶达紙鎻掑叆瀹屾垚鍚庝粛鎸夊師elements鐨勯『搴忥級

	void insert(int i, const E &e) //鎻掑叆鍏冪礌鑷崇i浣嶏紝insert(0, e)涓巔repend(e)鐩稿悓
	{
		this->insert(i,e);
	};
	void removeAt(int i)
	{
		this->removeAt(i);
	}
	void removeOne(const E &e)
	{
		this->removeOne(e);
	}
	void removeAll(const E &e)
	{
		this->removeAll(e);
	};
	bool contains(const E &e) const
	{
		return this->contains(e);
	};
	void clearAndRelease()
	{
		this->clear();

	}
	void removeAtUnordered()
	{
		this->removeAtUnordered();
	}
	void removeAtUnordered(int i)
		{
			this->removeAtUnordered(i);
		}
	const E* dataPtr() const { return this->data(); }
	      E* dataPtr()       { return this->data(); }
	uint allocBack()
	  	{
	  		return this->allocBack();
	  	}
	};
NV_NAMESPACE_END

