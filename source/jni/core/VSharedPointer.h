#pragma once

#include "vglobal.h"

NV_NAMESPACE_BEGIN

template<class T>
class VSharedPointer
{
public:
    VSharedPointer()
        : m_ref(new int(1))
        , m_data(new T)
    {
	}

    VSharedPointer(const VSharedPointer<T> &source)
        : m_ref(source.m_ref)
        , m_data(source.m_data)
	{
        (*m_ref)++;
    }

    ~VSharedPointer()
	{
        (*m_ref)--;
        if (*m_ref == 0) {
            delete m_ref;
			delete m_data;
        }
	}

    VSharedPointer<T> &operator=(const VSharedPointer<T> &source)
    {
        m_ref = source.m_ref;
        m_data = source.m_data;
        (*m_ref)++;
        return *this;
    }

	T *operator->()
	{
		return m_data;
	}

	const T *operator->() const
	{
		return m_data;
	}

    T &operator*()
    {
        return *m_data;
    }

    const T &operator *() const
    {
        return *m_data;
    }

	void detach()
	{
		(*m_ref)--;
		m_ref = new int(1);
		m_data = new T(*m_data);
	}

    void reset()
    {
        (*m_ref)--;
        m_ref = new int(1);
        m_data = new T;
    }

private:
    int *m_ref;
    T *m_data;
};

NV_NAMESPACE_END
