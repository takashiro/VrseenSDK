#include "VBinaryFile.h"
#include "VSysFile.h"

namespace NervGear
{

VBinaryFile::~VBinaryFile()
{
    if ( m_allocated )
    {
//        OVR_FREE( const_cast< UByte* >( m_data ) );
        delete[] m_data;
    }
}

//VBinaryReader::VBinaryReader( const char * path, const char ** perror ) :
//    m_data( NULL ),
//    m_size( 0 ),
//    m_offset( 0 ),
//    m_allocated( true )
//{
//    VSysFile f;
//    if ( !f.open( path, File::Open_Read, File::ReadOnly ) )
//    {
//        if ( perror != NULL )
//        {
//            *perror = "Failed to open file";
//        }
//        return;
//    }

//    m_size = f.length();
//    m_data = (UByte*) OVR_ALLOC( m_size + 1 );
//    int bytes = f.read( (UByte *)m_data, m_size );
//    if ( bytes != m_size && perror != NULL )
//    {
//        *perror = "Failed to read file";
//    }
//    f.close();
//}

VBinaryFile::VBinaryFile(const VString path, const char **error )
    : m_data(nullptr)
    , m_size(0)
    , m_offset(0)
    , m_allocated(true)
{
    VSysFile f;
    if (!f.open(path, VFile::Open_Read)) {
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
    f.fileClose();
}

} // namespace NervGear
