/************************************************************************************

PublicHeader:   OVR.h
Filename    :   OVR_String.h
Content     :   String UTF8 string implementation with copy-on-write semantics
                (thread-safe for assignment but not modification).
Created     :   September 19, 2012
Notes       :

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

************************************************************************************/

#pragma once

#include "Types.h"
#include "Allocator.h"
#include "UTF8Util.h"
#include "Atomic.h"
#include "Std.h"
#include "Alg.h"

namespace NervGear {

// ***** Classes

class String;
class StringBuffer;


//-----------------------------------------------------------------------------------
// ***** String Class

// String is UTF8 based string class with copy-on-write implementation
// for assignment.

class String
{
protected:

    enum FlagConstants
    {
        //Flag_GetLength      = 0x7FFFFFFF,
        // This flag is set if GetLength() == GetSize() for a string.
        // Avoid extra scanning is Substring and indexing logic.
        Flag_LengthIsSizeShift   = (sizeof(UPInt)*8 - 1)
    };


    // Internal structure to hold string data
    struct DataDesc
    {
        // Number of bytes. Will be the same as the number of chars if the characters
        // are ascii, may not be equal to number of chars in case string data is UTF8.
        UPInt   m_size;
        volatile SInt32 refCount;
        char    data[1];

        void    addRef()
        {
            AtomicOps<SInt32>::ExchangeAdd_NoSync(&refCount, 1);
        }
        // Decrement ref count. This needs to be thread-safe, since
        // a different thread could have also decremented the ref count.
        // For example, if u start off with a ref count = 2. Now if u
        // decrement the ref count and check against 0 in different
        // statements, a different thread can also decrement the ref count
        // in between our decrement and checking against 0 and will find
        // the ref count = 0 and delete the object. This will lead to a crash
        // when context switches to our thread and we'll be trying to delete
        // an already deleted object. Hence decrementing the ref count and
        // checking against 0 needs to made an atomic operation.
        void    release()
        {
            if ((AtomicOps<SInt32>::ExchangeAdd_NoSync(&refCount, -1) - 1) == 0)
                OVR_FREE(this);
        }

        static UPInt lengthFlagBit()     { return UPInt(1) << Flag_LengthIsSizeShift; }
        UPInt       size() const         { return m_size & ~lengthFlagBit() ; }
        UPInt       lengthFlag()  const  { return m_size & lengthFlagBit(); }
        bool        lengthIsSize() const    { return lengthFlag() != 0; }
    };

    // Heap type of the string is encoded in the lower bits.
    enum HeapType
    {
        HT_Global   = 0,    // Heap is global.
        HT_Local    = 1,    // SF::String_loc: Heap is determined based on string's address.
        HT_Dynamic  = 2,    // SF::String_temp: Heap is stored as a part of the class.
        HT_Mask     = 3
    };

    union {
        DataDesc* pData;
        UPInt     heapTypeBits;
    };
    typedef union {
        DataDesc* pData;
        UPInt     heapTypeBits;
    } DataDescUnion;

    inline HeapType    heapType() const { return (HeapType) (heapTypeBits & HT_Mask); }

    inline DataDesc*   data() const
    {
        DataDescUnion u;
        u.pData    = pData;
        u.heapTypeBits = (u.heapTypeBits & ~(UPInt)HT_Mask);
        return u.pData;
    }

    inline void        setData(DataDesc* pdesc)
    {
        HeapType ht = heapType();
        pData = pdesc;
        OVR_ASSERT((heapTypeBits & HT_Mask) == 0);
        heapTypeBits |= ht;
    }


    DataDesc*   allocData(UPInt size, UPInt lengthIsSize);
    DataDesc*   allocDataCopy1(UPInt size, UPInt lengthIsSize,
                               const char* pdata, UPInt copySize);
    DataDesc*   allocDataCopy2(UPInt size, UPInt lengthIsSize,
                               const char* pdata1, UPInt copySize1,
                               const char* pdata2, UPInt copySize2);

    // Special constructor to avoid data initalization when used in derived class.
    struct NoConstructor { };
    String(const NoConstructor&) { }

public:
    // Constructors / Destructors.
    String();
    String(const char* data);
    String(const char* data1, const char* pdata2, const char* pdata3 = 0);
    String(const char* data, UPInt buflen);
    String(const String& src);
    String(const StringBuffer& src);
    explicit String(const wchar_t* data);

    // Destructor (Captain Obvious guarantees!)
    ~String()
    {
        data()->release();
    }

    // Declaration of NullString
    static DataDesc NullData;


    // *** General Functions

    void        clear();

    // For casting to a pointer to char.
    operator const char*() const        { return data()->data; }
    // Pointer to raw buffer.
    const char* toCString() const          { return data()->data; }

    // Returns number of bytes
    UPInt       size() const         { return data()->size() ; }
    // Tells whether or not the string is empty
    bool        isEmpty() const         { return size() == 0; }

    // Returns  number of characters
    UPInt       length() const;

    // Returns  character at the specified index
    UInt32      at(UPInt index) const;
    UInt32      firstCharAt(UPInt index, const char** offset) const;
    UInt32      nextChar(const char** offset) const;

    // Appends a character
    void        append(UInt32 ch);

    // Append a string
    void        append(const wchar_t* pstr, SPInt len = -1);
    void        append(const char* putf8str, SPInt utf8StrSz = -1);

    // Assigns string with known size.
    void        assign(const char* putf8str, UPInt size);

    //  Resize the string to the new size
//  void        Resize(UPInt _size);

    // Removes the character at posAt
    void        remove(UPInt posAt, SPInt len = 1);

    // Returns a String that's a substring of this.
    //  -start is the index of the first UTF8 character you want to include.
    //  -end is the index one past the last UTF8 character you want to include.
    String		mid(UPInt start, UPInt end) const;

    String		left(UPInt count) const { return mid(0, count); }
    String		right(UPInt count) const { return mid(length() - count, length()); }

    // Case-conversion
    String		toUpper() const;
    String		toLower() const;

    // Inserts substr at posAt
    String&		insert(const char* substr, UPInt posAt, SPInt len = -1);

    // Inserts character at posAt
    UPInt       insert(UInt32 c, UPInt posAt);

    // Inserts substr at posAt, which is an index of a character (not byte).
    // Of size is specified, it is in bytes.
//  String&    Insert(const UInt32* substr, UPInt posAt, SPInt size = -1);

    // Get Byte index of the character at position = index
    UPInt       byteIndex(UPInt index) const { return (UPInt)UTF8Util::GetByteIndex(index, data()->data); }

    void		stripTrailing(const char * str);

    // Utility: case-insensitive string compare.  stricmp() & strnicmp() are not
    // ANSI or POSIX, do not seem to appear in Linux.
    static int OVR_STDCALL   CompareNoCase(const char* a, const char* b);
    static int OVR_STDCALL   CompareNoCase(const char* a, const char* b, SPInt len);

    // Hash function, case-insensitive
    static UPInt OVR_STDCALL BernsteinHashFunctionCIS(const void* pdataIn, UPInt size, UPInt seed = 5381);

    // Hash function, case-sensitive
    static UPInt OVR_STDCALL BernsteinHashFunction(const void* pdataIn, UPInt size, UPInt seed = 5381);


    // ***** File path parsing helper functions.
    // Implemented in OVR_String_FilePath.cpp.

    // Absolute paths can star with:
    //  - protocols:        'file://', 'http://'
    //  - windows drive:    'c:\'
    //  - UNC share name:   '\\share'
    //  - unix root         '/'
    static bool HasAbsolutePath(const char* path);
    static bool HasExtension(const char* path);
    static bool HasProtocol(const char* path);

    bool    hasAbsolutePath() const { return HasAbsolutePath(toCString()); }
    bool    hasExtension() const    { return HasExtension(toCString()); }
    bool    hHasProtocol() const     { return HasProtocol(toCString()); }

    String  protocol() const;    // Returns protocol, if any, with trailing '://'.
    String  path() const;        // Returns path with trailing '/'.
    String  fileName() const;    // Returns filename, including extension.
    String  extension() const;   // Returns extension with a dot.

    void    stripProtocol();        // Strips front protocol, if any, from the string.
    void    stripExtension();       // Strips off trailing extension.


    // Operators
    // Assignment
    void        operator =  (const char* str);
    void        operator =  (const wchar_t* str);
    void        operator =  (const String& src);
    void        operator =  (const StringBuffer& src);

    // Addition
    void        operator += (const String& src);
    void        operator += (const char* psrc)       { append(psrc); }
    void        operator += (const wchar_t* psrc)    { append(psrc); }
    void        operator += (char  ch)               { append(ch); }
    String      operator +  (const char* str) const;
    String      operator +  (const String& src)  const;

    // Comparison
    bool        operator == (const String& str) const
    {
        return (OVR_strcmp(data()->data, str.data()->data)== 0);
    }

    bool        operator != (const String& str) const
    {
        return !operator == (str);
    }

    bool        operator == (const char* str) const
    {
        return OVR_strcmp(data()->data, str) == 0;
    }

    bool        operator != (const char* str) const
    {
        return !operator == (str);
    }

    bool        operator <  (const char* pstr) const
    {
        return OVR_strcmp(data()->data, pstr) < 0;
    }

    bool        operator <  (const String& str) const
    {
        return *this < str.data()->data;
    }

    bool        operator >  (const char* pstr) const
    {
        return OVR_strcmp(data()->data, pstr) > 0;
    }

    bool        operator >  (const String& str) const
    {
        return *this > str.data()->data;
    }

    int CompareNoCase(const char* pstr) const
    {
        return CompareNoCase(data()->data, pstr);
    }
    int CompareNoCase(const String& str) const
    {
        return CompareNoCase(data()->data, str.toCString());
    }

    // Accesses raw bytes
    const char&     operator [] (int index) const
    {
        OVR_ASSERT(index >= 0 && (UPInt)index < size());
        return data()->data[index];
    }
    const char&     operator [] (UPInt index) const
    {
        OVR_ASSERT(index < size());
        return data()->data[index];
    }


    // Case insensitive keys are used to look up insensitive string in hash tables
    // for SWF files with version before SWF 7.
    struct NoCaseKey
    {
        const String* pStr;
        NoCaseKey(const String &str) : pStr(&str){};
    };

    bool    operator == (const NoCaseKey& strKey) const
    {
        return (CompareNoCase(toCString(), strKey.pStr->toCString()) == 0);
    }
    bool    operator != (const NoCaseKey& strKey) const
    {
        return !(CompareNoCase(toCString(), strKey.pStr->toCString()) == 0);
    }

    // Hash functor used for strings.
    struct HashFunctor
    {
        UPInt  operator()(const String& data) const
        {
            UPInt  size = data.size();
            return String::BernsteinHashFunction((const char*)data, size);
        }
    };
    // Case-insensitive hash functor used for strings. Supports additional
    // lookup based on NoCaseKey.
    struct NoCaseHashFunctor
    {
        UPInt  operator()(const String& data) const
        {
            UPInt  size = data.size();
            return String::BernsteinHashFunctionCIS((const char*)data, size);
        }
        UPInt  operator()(const NoCaseKey& data) const
        {
            UPInt  size = data.pStr->size();
            return String::BernsteinHashFunctionCIS((const char*)data.pStr->toCString(), size);
        }
    };

};


//-----------------------------------------------------------------------------------
// ***** String Buffer used for Building Strings

class StringBuffer
{
    char*           m_data;
    UPInt           m_size;
    UPInt           m_bufferSize;
    UPInt           m_growSize;
    mutable bool    m_lengthIsSize;

public:

    // Constructors / Destructor.
    StringBuffer();
    explicit StringBuffer(UPInt growSize);
    StringBuffer(const char* data);
    StringBuffer(const char* data, UPInt buflen);
    StringBuffer(const String& src);
    StringBuffer(const StringBuffer& src);
    explicit StringBuffer(const wchar_t* data);
    ~StringBuffer();


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
    void        operator =  (const String& src);
    void        operator =  (const StringBuffer& src);

    // Addition
    void        operator += (const String& src)      { append(src.toCString(),src.size()); }
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


//
// Wrapper for string data. The data must have a guaranteed
// lifespan throughout the usage of the wrapper. Not intended for
// cached usage. Not thread safe.
//
class StringDataPtr
{
public:
    StringDataPtr() : m_str(NULL), m_size(0) {}
    StringDataPtr(const StringDataPtr& p)
        : m_str(p.m_str), m_size(p.m_size) {}
    StringDataPtr(const char* pstr, UPInt sz)
        : m_str(pstr), m_size(sz) {}
    StringDataPtr(const char* pstr)
        : m_str(pstr), m_size((pstr != NULL) ? OVR_strlen(pstr) : 0) {}
    explicit StringDataPtr(const String& str)
        : m_str(str.toCString()), m_size(str.size()) {}
    template <typename T, int N>
    StringDataPtr(const T (&v)[N])
        : m_str(v), m_size(N) {}

public:
    const char* toCString() const { return m_str; }
    UPInt       size() const { return m_size; }
    bool        isEmpty() const { return size() == 0; }

    // value is a prefix of this string
    // Character's values are not compared.
    bool        isPrefix(const StringDataPtr& value) const
    {
        return toCString() == value.toCString() && size() >= value.size();
    }
    // value is a suffix of this string
    // Character's values are not compared.
    bool        isSuffix(const StringDataPtr& value) const
    {
        return toCString() <= value.toCString() && (end()) == (value.end());
    }

    // Find first character.
    // init_ind - initial index.
    SPInt       find(char c, UPInt init_ind = 0) const
    {
        for (UPInt i = init_ind; i < size(); ++i)
            if (m_str[i] == c)
                return static_cast<SPInt>(i);

        return -1;
    }

    // Find last character.
    // init_ind - initial index.
    SPInt       findLast(char c, UPInt init_ind = ~0) const
    {
        if (init_ind == (UPInt)~0 || init_ind > size())
            init_ind = size();
        else
            ++init_ind;

        for (UPInt i = init_ind; i > 0; --i)
            if (m_str[i - 1] == c)
                return static_cast<SPInt>(i - 1);

        return -1;
    }

    // Create new object and trim size bytes from the left.
    StringDataPtr  trimLeft(UPInt size) const
    {
        // Limit trim size to the size of the string.
        size = Alg::PMin(this->size(), size);

        return StringDataPtr(toCString() + size, this->size() - size);
    }
    // Create new object and trim size bytes from the right.
    StringDataPtr  trimRight(UPInt size) const
    {
        // Limit trim to the size of the string.
        size = Alg::PMin(this->size(), size);

        return StringDataPtr(toCString(), this->size() - size);
    }

    // Create new object, which contains next token.
    // Useful for parsing.
    StringDataPtr nextToken(char separator = ':') const
    {
        UPInt cur_pos = 0;
        const char* cur_str = toCString();

        for (; cur_pos < size() && cur_str[cur_pos]; ++cur_pos)
        {
            if (cur_str[cur_pos] == separator)
            {
                break;
            }
        }

        return StringDataPtr(toCString(), cur_pos);
    }

    // Trim size bytes from the left.
    StringDataPtr& trimLeft(UPInt size)
    {
        // Limit trim size to the size of the string.
        size = Alg::PMin(this->size(), size);
        m_str += size;
        m_size -= size;

        return *this;
    }
    // Trim size bytes from the right.
    StringDataPtr& trimRight(UPInt size)
    {
        // Limit trim to the size of the string.
        size = Alg::PMin(this->size(), size);
        m_size -= size;

        return *this;
    }

    const char* begin() const { return toCString(); }
    const char* end() const { return toCString() + size(); }

    // Hash functor used string data pointers
    struct HashFunctor
    {
        UPInt operator()(const StringDataPtr& data) const
        {
            return String::BernsteinHashFunction(data.toCString(), data.size());
        }
    };

    bool operator== (const StringDataPtr& data) const
    {
        return (OVR_strncmp(m_str, data.m_str, data.m_size) == 0);
    }

protected:
    const char* m_str;
    UPInt       m_size;
};

} // OVR
