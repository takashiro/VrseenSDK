#pragma once

#include <vglobal.h>

NV_NAMESPACE_BEGIN

template<typename Enum, typename Storage = int>
class VFlags
{
public:
    typedef Enum EnumType;
    typedef Storage StorageType;

    VFlags() : m_flags(0) {}
    VFlags(const VFlags &other) : m_flags(other.m_flags) {}
    VFlags(Enum flag) : m_flags(1 << flag) {}
    VFlags(StorageType flags) : m_flags(flags) {}

    bool contains(Enum other) const { return (m_flags & (1 << other)) != 0; }
    void set(Enum other) { m_flags |= (1 << other); }
    void unset(Enum other) { m_flags &= ~(1 << other); }

    void setAll()
    {
        uint numBits = sizeof(Storage) * 8;
        Storage topBit = (1ULL << (numBits - 1));
        Storage allButTopBit = topBit - 1ULL;
        m_flags = allButTopBit | topBit;
    }

    operator Storage() const { return m_flags; }

    bool operator!() const { return m_flags == 0; }
    VFlags operator~() const { return ~m_flags; }
    VFlags &operator = (const VFlags &other) { m_flags = other; return *this; }

    VFlags operator & (Storage mask) const { return m_flags & mask; }
    VFlags operator & (Enum mask) const { return m_flags & (1 << mask); }

    const VFlags &operator &= (Storage mask) { m_flags &= mask; return *this; }
    const VFlags &operator &= (Enum mask) { m_flags &= (1 << mask); return *this; }

    VFlags operator ^ (VFlags other) const { return m_flags ^ other; }
    VFlags operator ^ (Enum other) const { return m_flags ^ (1 << other); }

    const VFlags &operator ^= (VFlags other) { m_flags ^= other; return *this; }
    const VFlags &operator ^= (Enum other) { m_flags ^= (1 << other); return *this; }

    VFlags operator | (VFlags other) const { return m_flags | other; }
    VFlags operator | (Enum other) const { return m_flags | (1 << other); }

    const VFlags &operator |= (VFlags other) { m_flags |= other; return *this; }
    const VFlags &operator |= (Enum other) { m_flags |= (1 << other); return *this; }


private:
    Storage m_flags;
};

NV_NAMESPACE_END
