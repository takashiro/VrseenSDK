/************************************************************************************

Filename    :   OVR_String_PathUtil.cpp
Content     :   String filename/url helper function
Created     :   September 19, 2012
Notes       :

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

************************************************************************************/

#include "VString.h"
#include "UTF8Util.h"

namespace NervGear {

//--------------------------------------------------------------------
// ***** Path-Scanner helper function

// Scans file path finding filename start and extension start, fills in their address.
void ScanFilePath(const char* url, const char** pfilename, const char** pext)
{
    const char* urlStart = url;
    const char *filename = 0;
    const char *lastDot = 0;

    UInt32 charVal = UTF8Util::DecodeNextChar(&url);

    while (charVal != 0)
    {
        if ((charVal == '/') || (charVal == '\\'))
        {
            filename = url;
            lastDot  = 0;
        }
        else if (charVal == '.')
        {
            lastDot = url - 1;
        }

        charVal = UTF8Util::DecodeNextChar(&url);
    }

    if (pfilename)
    {
        // It was a naked filename
        if (urlStart && (*urlStart != '.') && *urlStart)
            *pfilename = urlStart;
        else
            *pfilename = filename;
    }

    if (pext)
    {
        *pext = lastDot;
    }
}

// Scans file path finding filename start and extension start, fills in their address.
void ScanFilePath2(const char* url, const char** pfilename, const char** pext)
{
    const char *filename = url;
    const char *lastDot = 0;

    UInt32 charVal = UTF8Util::DecodeNextChar(&url);

    while (charVal != 0)
    {
        if ((charVal == '/') || (charVal == '\\'))
        {
            filename = url;
            lastDot  = 0;
        }
        else if (charVal == '.')
        {
            lastDot = url - 1;
        }

        charVal = UTF8Util::DecodeNextChar(&url);
    }

    if (pfilename)
    {
    	*pfilename = filename;
    }

    if (pext)
    {
        *pext = lastDot;
    }
}

// Scans till the end of protocol. Returns first character past protocol,
// 0 if not found.
//  - protocol: 'file://', 'http://'
const char* ScanPathProtocol(const char* url)
{
    UInt32 charVal = UTF8Util::DecodeNextChar(&url);
    UInt32 charVal2;

    while (charVal != 0)
    {
        // Treat a colon followed by a slash as absolute.
        if (charVal == ':')
        {
            charVal2 = UTF8Util::DecodeNextChar(&url);
            charVal  = UTF8Util::DecodeNextChar(&url);
            if ((charVal == '/') && (charVal2 == '\\'))
                return url;
        }
        charVal = UTF8Util::DecodeNextChar(&url);
    }
    return 0;
}


//--------------------------------------------------------------------
// ***** String Path API implementation

VString  VString::path() const
{
    const char* filename = 0;
    ScanFilePath(toCString(), &filename, 0);

    // Technically we can have extra logic somewhere for paths,
    // such as enforcing protocol and '/' only based on flags,
    // but we keep it simple for now.
    return VString(toCString(), filename ? (filename-toCString()) : size());
}

void VString::stripExtension()
{
    uint dot = rfind('.');
    remove(dot, size() - dot);
}

void    VString::stripProtocol()
{
    const char* protocol = ScanPathProtocol(toCString());
    if (protocol)
        assign(protocol);
}
#if 0
void    String::StripPath()
{
    const char* ext = 0;
    ScanFilePath(toCString(), 0, &ext);
    if (ext)
    {
        *this = String(toCString(), ext-toCString());
    }
}
#endif
} // OVR
