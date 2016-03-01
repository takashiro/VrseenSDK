#pragma once

#include "vglobal.h"

#include "Types.h"
#include "Allocator.h"
#include "UTF8Util.h"
#include "Atomic.h"
#include "Std.h"
#include "Alg.h"

#include <string>

NV_NAMESPACE_BEGIN

// ***** Classes

class VString;
class VStringBuffer;


//-----------------------------------------------------------------------------------
// ***** String Class

// String is UTF8 based string class with copy-on-write implementation
// for assignment.

class VString
{
protected:

    enum FlagConstants
    {
        //Flag_GetLength      = 0x7FFFFFFF,
        // This flag is set if GetLength() == GetSize() for a string.
        // Avoid extra scanning is Substring and indexing logic.
        Flag_LengthIsSizeShift   = (sizeof(uint)*8 - 1)
    };


    // Internal structure to hold string data
    struct DataDesc
    {
        // Number of bytes. Will be the same as the number of chars if the characters
        // are ascii, may not be equal to number of chars in case string data is UTF8.
        uint   m_size;
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

        static uint lengthFlagBit()     { return uint(1) << Flag_LengthIsSizeShift; }
        uint       size() const         { return m_size & ~lengthFlagBit() ; }
        uint       lengthFlag()  const  { return m_size & lengthFlagBit(); }
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
        uint     heapTypeBits;
    };
    typedef union {
        DataDesc* pData;
        uint     heapTypeBits;
    } DataDescUnion;

    inline HeapType    heapType() const { return (HeapType) (heapTypeBits & HT_Mask); }

    inline DataDesc*   data() const
    {
        DataDescUnion u;
        u.pData    = pData;
        u.heapTypeBits = (u.heapTypeBits & ~(uint)HT_Mask);
        return u.pData;
    }

    inline void        setData(DataDesc* pdesc)
    {
        HeapType ht = heapType();
        pData = pdesc;
        OVR_ASSERT((heapTypeBits & HT_Mask) == 0);
        heapTypeBits |= ht;
    }


    DataDesc*   allocData(uint size, uint lengthIsSize);
    DataDesc*   allocDataCopy1(uint size, uint lengthIsSize,
                               const char* pdata, uint copySize);
    DataDesc*   allocDataCopy2(uint size, uint lengthIsSize,
                               const char* pdata1, uint copySize1,
                               const char* pdata2, uint copySize2);
public:
    // Constructors / Destructors.
    VString();
    VString(const char* data);
    VString(const std::string &data);
    VString(const char* data1, const char* pdata2, const char* pdata3 = 0);
    VString(const char* data, uint buflen);
    VString(const VString& src);
    VString(const VStringBuffer& src);
    explicit VString(const wchar_t* data);

    // Destructor (Captain Obvious guarantees!)
    ~VString()
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
    uint       size() const         { return data()->size() ; }
    // Tells whether or not the string is empty
    bool        isEmpty() const         { return size() == 0; }

    // Returns  number of characters
    uint       length() const;

    // Returns  character at the specified index
    UInt32      at(uint index) const;
    UInt32      firstCharAt(uint index, const char** offset) const;
    UInt32      nextChar(const char** offset) const;

    // Appends a character
    void        append(UInt32 ch);

    // Append a string
    void        append(const wchar_t* pstr, int len = -1);
    void        append(const char* putf8str, int utf8StrSz = -1);

    // Assigns string with known size.
    void        assign(const char* putf8str, uint size);

    //  Resize the string to the new size
//  void        Resize(UPInt _size);

    // Removes the character at posAt
    void        remove(uint posAt, int len = 1);

    // Returns a String that's a substring of this.
    //  -start is the index of the first UTF8 character you want to include.
    //  -end is the index one past the last UTF8 character you want to include.
    VString		mid(uint start, uint end) const;

    VString		left(uint count) const { return mid(0, count); }
    VString		right(uint count) const { return mid(length() - count, length()); }

    // Case-conversion
    VString		toUpper() const;
    VString		toLower() const;

    // Inserts substr at posAt
    VString&		insert(const char* substr, uint posAt, int len = -1);

    // Inserts character at posAt
    uint       insert(UInt32 c, uint posAt);

    // Inserts substr at posAt, which is an index of a character (not byte).
    // Of size is specified, it is in bytes.
//  String&    Insert(const UInt32* substr, UPInt posAt, SPInt size = -1);

    // Get Byte index of the character at position = index
    uint       byteIndex(uint index) const { return (uint)UTF8Util::GetByteIndex(index, data()->data); }

    void		stripTrailing(const char * str);

    // Utility: case-insensitive string compare.  stricmp() & strnicmp() are not
    // ANSI or POSIX, do not seem to appear in Linux.
    static int OVR_STDCALL   CompareNoCase(const char* a, const char* b);
    static int OVR_STDCALL   CompareNoCase(const char* a, const char* b, int len);

    // Hash function, case-insensitive
    static uint OVR_STDCALL BernsteinHashFunctionCIS(const void* pdataIn, uint size, uint seed = 5381);

    // Hash function, case-sensitive
    static uint OVR_STDCALL BernsteinHashFunction(const void* pdataIn, uint size, uint seed = 5381);


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

    VString  protocol() const;    // Returns protocol, if any, with trailing '://'.
    VString  path() const;        // Returns path with trailing '/'.
    VString  fileName() const;    // Returns filename, including extension.
    VString  extension() const;   // Returns extension with a dot.

    void    stripProtocol();        // Strips front protocol, if any, from the string.
    void    stripExtension();       // Strips off trailing extension.


    // Operators
    // Assignment
    void        operator =  (const char* str);
    void        operator =  (const wchar_t* str);
    void        operator =  (const VString& src);
    void        operator =  (const VStringBuffer& src);

    // Addition
    void        operator += (const VString& src);
    void        operator += (const char* psrc)       { append(psrc); }
    void        operator += (const wchar_t* psrc)    { append(psrc); }
    void        operator += (char  ch)               { append(ch); }
    VString      operator +  (const char* str) const;
    VString      operator +  (const VString& src)  const;

    // Comparison
    bool        operator == (const VString& str) const
    {
        return (strcmp(data()->data, str.data()->data)== 0);
    }

    bool        operator != (const VString& str) const
    {
        return !operator == (str);
    }

    bool        operator == (const char* str) const
    {
        return strcmp(data()->data, str) == 0;
    }

    bool        operator != (const char* str) const
    {
        return !operator == (str);
    }

    bool        operator <  (const char* pstr) const
    {
        return strcmp(data()->data, pstr) < 0;
    }

    bool        operator <  (const VString& str) const
    {
        return *this < str.data()->data;
    }

    bool        operator >  (const char* pstr) const
    {
        return strcmp(data()->data, pstr) > 0;
    }

    bool        operator >  (const VString& str) const
    {
        return *this > str.data()->data;
    }

    int CompareNoCase(const char* pstr) const
    {
        return CompareNoCase(data()->data, pstr);
    }
    int CompareNoCase(const VString& str) const
    {
        return CompareNoCase(data()->data, str.toCString());
    }

    // Accesses raw bytes
    const char&     operator [] (int index) const
    {
        OVR_ASSERT(index >= 0 && (uint)index < size());
        return data()->data[index];
    }
    const char&     operator [] (uint index) const
    {
        OVR_ASSERT(index < size());
        return data()->data[index];
    }


    // Case insensitive keys are used to look up insensitive string in hash tables
    // for SWF files with version before SWF 7.
    struct NoCaseKey
    {
        const VString* pStr;
        NoCaseKey(const VString &str) : pStr(&str){};
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
        uint  operator()(const VString& data) const
        {
            uint  size = data.size();
            return VString::BernsteinHashFunction((const char*)data, size);
        }
    };
    // Case-insensitive hash functor used for strings. Supports additional
    // lookup based on NoCaseKey.
    struct NoCaseHashFunctor
    {
        uint  operator()(const VString& data) const
        {
            uint  size = data.size();
            return VString::BernsteinHashFunctionCIS((const char*)data, size);
        }
        uint  operator()(const NoCaseKey& data) const
        {
            uint  size = data.pStr->size();
            return VString::BernsteinHashFunctionCIS((const char*)data.pStr->toCString(), size);
        }
    };

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
    StringDataPtr(const char* pstr, uint sz)
        : m_str(pstr), m_size(sz) {}
    StringDataPtr(const char* pstr)
        : m_str(pstr), m_size((pstr != NULL) ? strlen(pstr) : 0) {}
    explicit StringDataPtr(const VString& str)
        : m_str(str.toCString()), m_size(str.size()) {}
    template <typename T, int N>
    StringDataPtr(const T (&v)[N])
        : m_str(v), m_size(N) {}

public:
    const char* toCString() const { return m_str; }
    uint       size() const { return m_size; }
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
    int       find(char c, uint init_ind = 0) const
    {
        for (uint i = init_ind; i < size(); ++i)
            if (m_str[i] == c)
                return static_cast<int>(i);

        return -1;
    }

    // Find last character.
    // init_ind - initial index.
    int       findLast(char c, uint init_ind = ~0) const
    {
        if (init_ind == (uint)~0 || init_ind > size())
            init_ind = size();
        else
            ++init_ind;

        for (uint i = init_ind; i > 0; --i)
            if (m_str[i - 1] == c)
                return static_cast<int>(i - 1);

        return -1;
    }

    // Create new object and trim size bytes from the left.
    StringDataPtr  trimLeft(uint size) const
    {
        // Limit trim size to the size of the string.
        size = Alg::PMin(this->size(), size);

        return StringDataPtr(toCString() + size, this->size() - size);
    }
    // Create new object and trim size bytes from the right.
    StringDataPtr  trimRight(uint size) const
    {
        // Limit trim to the size of the string.
        size = Alg::PMin(this->size(), size);

        return StringDataPtr(toCString(), this->size() - size);
    }

    // Create new object, which contains next token.
    // Useful for parsing.
    StringDataPtr nextToken(char separator = ':') const
    {
        uint cur_pos = 0;
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
    StringDataPtr& trimLeft(uint size)
    {
        // Limit trim size to the size of the string.
        size = Alg::PMin(this->size(), size);
        m_str += size;
        m_size -= size;

        return *this;
    }
    // Trim size bytes from the right.
    StringDataPtr& trimRight(uint size)
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
        uint operator()(const StringDataPtr& data) const
        {
            return VString::BernsteinHashFunction(data.toCString(), data.size());
        }
    };

    bool operator== (const StringDataPtr& data) const
    {
        return (OVR_strncmp(m_str, data.m_str, data.m_size) == 0);
    }

protected:
    const char* m_str;
    uint       m_size;
};

} // OVR
