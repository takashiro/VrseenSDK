#pragma once

#include "vglobal.h"
#include "VString.h"

NV_NAMESPACE_BEGIN

class VStringBuffer
{
    char*           m_data;
    UPInt           m_size;
    UPInt           m_bufferSize;
    UPInt           m_growSize;
    mutable bool    m_lengthIsSize;

public:

    // Constructors / Destructor.
    VStringBuffer();
    explicit VStringBuffer(UPInt growSize);
    VStringBuffer(const char* data);
    VStringBuffer(const char* data, UPInt buflen);
    VStringBuffer(const VString& src);
    VStringBuffer(const VStringBuffer& src);
    explicit VStringBuffer(const wchar_t* data);
    ~VStringBuffer();


    // Modify grow size used for growing/shrinking the buffer.
    UPInt       growSize() const         { return m_growSize; }
    void        setGrowSize(UPInt growSize);


    // *** General Functions
    // Does not release memory, just sets Size to 0
    void        clear();

    // For casting to a pointer to char.
    operator const char*() const        { return (m_data) ? m_data : ""; }
    // Pointer to raw buffer.
    const char* toCString() const          { return (m_data) ? m_data : ""; }

    // Returns number of bytes.
    UPInt       size() const         { return m_size ; }
    // Tells whether or not the string is empty.
    bool        isEmpty() const         { return size() == 0; }

    // Returns  number of characters
    UPInt       length() const;

    // Returns  character at the specified index
    UInt32      charAt(UPInt index) const;
    UInt32      firstCharAt(UPInt index, const char** offset) const;
    UInt32      nextChar(const char** offset) const;


    //  Resize the string to the new size
    void        resize(UPInt _size);
    void        reserve(UPInt _size);

    // Appends a character
    void        append(UInt32 ch);

    // Append a string
    void        append(const wchar_t* pstr, SPInt len = -1);
    void        append(const char* putf8str, SPInt utf8StrSz = -1);
    void        appendFormat(const char* format, ...);

    // Assigned a string with dynamic data (copied through initializer).
    //void        AssignString(const InitStruct& src, UPInt size);

    // Inserts substr at posAt
    void        insert (const char* substr, UPInt posAt, SPInt len = -1);
    // Inserts character at posAt
    UPInt       insert(UInt32 c, UPInt posAt);

    // Assignment
    void        operator =  (const char* str);
    void        operator =  (const wchar_t* str);
    void        operator =  (const VString& src);
    void        operator =  (const VStringBuffer& src);

    // Addition
    void        operator += (const VString& src)      { append(src.toCString(),src.size()); }
    void        operator += (const char* psrc)       { append(psrc); }
    void        operator += (const wchar_t* psrc)    { append(psrc); }
    void        operator += (char  ch)               { append(ch); }
    //String   operator +  (const char* str) const ;
    //String   operator +  (const String& src)  const ;

    // Accesses raw bytes
    char&       operator [] (int index)
    {
        OVR_ASSERT(((UPInt)index) < size());
        return m_data[index];
    }
    char&       operator [] (UPInt index)
    {
        OVR_ASSERT(index < size());
        return m_data[index];
    }

    const char&     operator [] (int index) const
    {
        OVR_ASSERT(((UPInt)index) < size());
        return m_data[index];
    }
    const char&     operator [] (UPInt index) const
    {
        OVR_ASSERT(index < size());
        return m_data[index];
    }
};

NV_NAMESPACE_END
