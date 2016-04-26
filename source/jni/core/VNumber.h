#pragma once

#include "vglobal.h"

NV_NAMESPACE_BEGIN

template<typename T, T InitialValue = 0>
class VNumber
{
public:
    VNumber() : m_value(InitialValue) { }
    explicit VNumber(T value) : m_value(value) { }

    VNumber &operator = (const VNumber &other) { m_value = other.value(); return *this; }

    // comparison operators
    bool operator == (const VNumber &other) const { return m_value == other.value(); }
    bool operator != (const VNumber &other) const { return m_value != other.value(); }

    bool operator < (const VNumber &other) const { return m_value < other.value(); }
    bool operator <= (const VNumber &other) const { return m_value <= other.value(); }

    bool operator > (const VNumber &other) const { return m_value > other.value(); }
    bool operator >= (const VNumber &other) const { return m_value >= other.value(); }

    // unary operators
    VNumber &operator++() { m_value++; return *this; }
    VNumber operator++(int) { VNumber prev(*this); m_value++; return prev; }

    VNumber &operator--(){ m_value--; return *this; }
    VNumber operator--(int){ VNumber prev(*this); m_value--; return prev; }

    // compound assignment operators
    VNumber &operator += (const VNumber &other) { m_value += other.value(); return *this; }
    VNumber &operator -= (const VNumber &other) { m_value -= other.value(); return *this; }
    VNumber &operator *= (const VNumber &other) { m_value *= other.value(); return *this; }
    VNumber &operator /= (const VNumber &other) { m_value /= other.value(); return *this; }
    VNumber &operator %= (const VNumber &other) { m_value %= other.value(); return *this; }

    // binary arithmetic operators
    VNumber operator + (const VNumber &other) const { return m_value + other.value(); }
    VNumber operator - (const VNumber &other) const { return m_value - other.value(); }
    VNumber operator * (const VNumber &other) const { return m_value * other.value(); }
    VNumber operator / (const VNumber &other) const { return m_value / other.value(); }
    VNumber operator % (const VNumber &other) const { return m_value % other.value(); }

    T value() const { return m_value; }

	// for using as a handle
    void reset() { m_value = InitialValue; }

    bool isValid() const { return m_value != InitialValue; }

private:
    T m_value;    // the value itself
};

NV_NAMESPACE_END

