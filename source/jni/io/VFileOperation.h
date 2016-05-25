#pragma once

#include "VAbstractFile.h"
#include "VString.h"

#include <sys/stat.h>
#include <errno.h>

NV_NAMESPACE_BEGIN
enum ImageFilter
{
    IMAGE_FILTER_NEAREST,
    IMAGE_FILTER_LINEAR,
    IMAGE_FILTER_CUBIC
};

//实现文件具体操作的类，VSysFile通过指针授权给父类调用该类的函数
class VFileOperation : public VAbstractFile, std::fstream
{
protected:

    // 文件名
    VString m_fileName;

    // 文件打开标识
    bool m_opened;

    // 文件打开模式
    int m_openFlag;
    // 文件操作错误码
    int m_errorCode;
    // 前一个文件操作
    int m_lastOp;

#ifdef OVR_FILE_VERIFY_SEEK_ERRORS
    //UByte*      pFileTestBuffer;
    unsigned char* pFileTestBuffer;
    unsigned FileTestLength;
    unsigned TestPos; // File pointer position during tests.
#endif

public:
    VFileOperation()
        : m_fileName("")
        , m_opened(false)
    {

#ifdef OVR_FILE_VERIFY_SEEK_ERRORS
        pFileTestBuffer =0;
        FileTestLength =0;
        TestPos =0;
#endif
    }
    // 两种形式的构造函数VString和const char * 形式给出文件名
    VFileOperation(const VString& m_fileName, int flags);

    VFileOperation(const char* pfileName, int flags);

    ~VFileOperation()
    {
        if (m_opened) {
            close();
        }
    }

    virtual const std::string filePath() override;

    bool isOpened() override;
    bool isWritable() override;

    // 文件位置、长度
    int tell() override;
    long long tell64() override;
    int length() override;
    long long length64() override;
    int errorCode() override;

    //  文件读写
    int write(const uchar *pbuffer, int numBytes) override;
    int read(uchar *pbuffer, int numBytes) override;
    int skipBytes(int numBytes) override;
    int bytesAvailable() override;
    bool bufferFlush() override;
    int seek(int offset, std::ios_base::seekdir origin) override;
    long long seek64(long long offset, std::ios_base::seekdir origin) override;

    int copyStream(VAbstractFile *pStream, int byteSize) override;
    bool close() override;

    // Uncompressed .pvr textures are much more efficient to load than bmp/tga/etc.
    // Use when generating thumbnails, etc.
    // Use stb_image_write.h for conventional files.
    static void        Write32BitPvrTexture( const char * fileName, const unsigned char * texture, int width, int height );

    // The returned buffer should be freed with free()
    // If srgb is true, the resampling will be gamma correct, otherwise it is just sumOf4 >> 2
    static unsigned char * QuarterImageSize( const unsigned char * src, const int width, const int height, const bool srgb );

    static unsigned char * ScaleImageRGBA( const unsigned char * src, const int width, const int height, const int newWidth, const int newHeight, const ImageFilter filter );

private:
    void fileInit();
};

NV_NAMESPACE_END

