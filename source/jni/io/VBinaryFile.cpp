#include "VBinaryFile.h"

namespace NervGear
{

VBinaryFile::~VBinaryFile()
{
    if ( m_allocated ) {
        delete[] m_data;
    }
}

VBinaryFile::VBinaryFile(const VString path, const char **error )
    : m_data(nullptr)
    , m_size(0)
    , m_offset(0)
    , m_allocated(true)
{
    VSysFile f;
    if (!f.open(path, VAbstractFile::Open_Read)) {
        if (error != nullptr) {
            *error = "fail to open file!";
        }
        return ;
    }
    m_size = f.length();
    m_data = reinterpret_cast<uchar*>(new char[m_size + 1]);
    int byteNum = f.read(const_cast<uchar*>(m_data), m_size);
    if (byteNum != m_size && error != nullptr) {
        *error = "Failed to read file";
    }
    f.close();
}

} // namespace NervGear
