
#pragma once

#include "vglobal.h"

#include "OVR.h"
#include "VStringHash.h"
#include "VStandardPath.h"

NV_NAMESPACE_BEGIN

// Debug tool
void LogMatrix( const char * title, const VR4Matrixf & m );

inline V3Vectf GetViewMatrixPosition( VR4Matrixf const & m )
{
#if 1
	return m.Inverted().GetTranslation();
#else
	// This is much cheaper if the view matrix is a pure rotation plus translation.
    return V3Vectf(	m.M[0][0] * m.M[0][3] + m.M[1][0] * m.M[1][3] + m.M[2][0] * m.M[2][3],
						m.M[0][1] * m.M[0][3] + m.M[1][1] * m.M[1][3] + m.M[2][1] * m.M[2][3],
						m.M[0][2] * m.M[0][3] + m.M[1][2] * m.M[1][3] + m.M[2][2] * m.M[2][3] );
#endif
}

inline V3Vectf GetViewMatrixForward( VR4Matrixf const & m )
{
    return V3Vectf( -m.M[2][0], -m.M[2][1], -m.M[2][2] ).Normalized();
}

// Returns true if the folder has specified permission
//bool HasPermission(VString fileOrDirName, mode_t mode );

// Returns true if the file exists
//bool FileExists(const VString &filename );

//void SortStringArray( VArray<VString> & strings );

//StringHash< VString > RelativeDirectoryFileList( const VArray< VString > & searchPaths, const char * RelativeDirPath );

// DirPath should by a directory with a trailing slash.
// Returns all files in the directory, already prepended by root.
// Subdirectories will have a trailing slash.
// All files and directories that start with . are skipped.
//VArray<VString> DirectoryFileList( const char * DirPath );

// Creates all the intermediate directories if they don't exist
//void MakePath(const VString &dirPath, mode_t mode );

// Returns true if head equals check plus zero or more characters.
bool MatchesHead( const char * head, const char * check );

float LinearRangeMapFloat( float inValue, float inStart, float inEnd, float outStart, float outEnd );

NV_NAMESPACE_END


