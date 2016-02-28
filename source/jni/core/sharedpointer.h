#pragma once

namespace NervGear
{

template<class T>
class SharedPointer
{
public:
	SharedPointer()
        : m_ref(new int(1))
        , m_data(new T)
    {
	}

	SharedPointer(const SharedPointer<T> &source)
        : m_ref(source.m_ref)
        , m_data(source.m_data)
	{
        (*m_ref)++;
    }

	~SharedPointer()
	{
        (*m_ref)--;
        if (*m_ref == 0) {
            delete m_ref;
			delete m_data;
        }
	}

    SharedPointer<T> &operator=(const SharedPointer<T> &source)
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

}
