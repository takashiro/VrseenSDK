/************************************************************************************

Filename    :   VrCommon.cpp
Content     :
Created     :
Authors     :

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

*************************************************************************************/

#include "VrCommon.h"

#include <pthread.h>
#include <sched.h>
#include <unistd.h>			// for usleep
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>

#include "MemBuffer.h"
#include "Android/GlUtils.h"		// for egl declarations
#include "Android/LogUtils.h"

namespace NervGear {

void LogMatrix( const char * title, const Matrix4f & m )
{
	LOG( "%s:", title );
	for ( int i = 0; i < 4; i++ )
	{
		LOG("%6.3f %6.3f %6.3f %6.3f", m.M[i][0], m.M[i][1], m.M[i][2], m.M[i][3] );
	}
}

#if 0
static Vector3f	MatrixOrigin( const Matrix4f & m )
{
	return Vector3f( -m.M[0][3], -m.M[1][3], -m.M[2][3] );
}
static Vector3f	MatrixForward( const Matrix4f & m )
{
	return Vector3f( -m.M[2][0], -m.M[2][1], -m.M[2][2] );
}
#endif

void SortStringArray(VArray<VString> &strings)
{

    if (strings.size() <= 1) {
        return;
	}

    qsort(&strings[0], strings.size(), sizeof( VString ), [](const void *a, const void *b){
        const VString *sa = ( VString * )a;
        const VString *sb = ( VString * )b;
        return sa->icompare( *sb );
    });
}

// DirPath should by a directory with a trailing slash.
// Returns all files in all search paths, as unique relative paths.
// Subdirectories will have a trailing slash.
// All files and directories that start with . are skipped.
StringHash< VString > RelativeDirectoryFileList( const VArray< VString > & searchPaths, const char * RelativeDirPath )
{
	//Check each of the mirrors in searchPaths and build up a list of unique strings
	StringHash< VString >	uniqueStrings;

	const int numSearchPaths = searchPaths.length();
	for ( int index = 0; index < numSearchPaths; ++index )
	{
		const VString fullPath = searchPaths[index] + VString( RelativeDirPath );

		DIR * dir = opendir( fullPath.toCString() );
		if ( dir != NULL )
		{
			struct dirent * entry;
			while ( ( entry = readdir( dir ) ) != NULL )
			{
				if ( entry->d_name[ 0 ] == '.' )
				{
					continue;
				}
				if ( entry->d_type == DT_DIR )
				{
					VString s( RelativeDirPath );
					s += entry->d_name;
					s += "/";
                    uniqueStrings.insert( s, s );
				}
				else if ( entry->d_type == DT_REG )
				{
					VString s( RelativeDirPath );
					s += entry->d_name;
                    uniqueStrings.insert( s, s );
				}
			}
			closedir( dir );
		}
	}

	return uniqueStrings;
}

// DirPath should by a directory with a trailing slash.
// Returns all files in the directory, already prepended by root.
// Subdirectories will have a trailing slash.
// All files and directories that start with . are skipped.
VArray<VString> DirectoryFileList( const char * DirPath )
{
	VArray<VString>	strings;

	DIR * dir = opendir( DirPath );
	if ( dir != NULL )
	{
		struct dirent * entry;
		while ( ( entry = readdir( dir ) ) != NULL )
		{
			if ( entry->d_name[ 0 ] == '.' )
			{
				continue;
			}
			if ( entry->d_type == DT_DIR )
			{
				VString s( DirPath );
				s += entry->d_name;
				s += "/";
				strings.append( s );
			}
			else if ( entry->d_type == DT_REG )
			{
				VString s( DirPath );
				s += entry->d_name;
				strings.append( s );
			}
		}
		closedir( dir );
	}

	SortStringArray( strings );

	return strings;
}

bool HasPermission(VString fileOrDirName, mode_t mode )
{
    int len = fileOrDirName.size();
    if ( fileOrDirName.at(len - 1) != '/' )
	{	// directory ends in a slash
		int	end = len - 1;
        for ( ; end > 0 && fileOrDirName.at(end) != '/'; end-- )
			;
        fileOrDirName = VString(fileOrDirName.data(), end);
	}
    return access( fileOrDirName.toCString(), mode ) == 0;
}

bool FileExists(const VString &filename)
{
	struct stat st;
    int result = stat( filename.toCString(), &st );
	return result == 0;
}

void MakePath( const VString &dirPath, mode_t mode )
{
	char path[ 256 ];
	char * currentChar = NULL;

    OVR_sprintf( path, sizeof( path ), "%s", dirPath.toCString() );

	for ( currentChar = path + 1; *currentChar; ++currentChar )
	{
		if ( *currentChar == '/' )
		{
			*currentChar = 0;
			DIR * checkDir = opendir( path );
			if ( checkDir == NULL )
			{
				mkdir( path, mode );
			}
			else
			{
				closedir( checkDir );
			}
			*currentChar = '/';
		}
	}
}


// Returns true if head equals check plus zero or more characters.
bool MatchesHead( const char * head, const char * check )
{
	const int l = strlen( head );
    return 0 == strncmp(head, check, l);
}

float LinearRangeMapFloat( float inValue, float inStart, float inEnd, float outStart, float outEnd )
{
	float outValue = inValue;
	if( fabsf(inEnd - inStart) < Mathf::SmallestNonDenormal )
	{
		return 0.5f*(outStart + outEnd);
	}
	outValue -= inStart;
	outValue /= (inEnd - inStart);
	outValue *= (outEnd - outStart);
	outValue += outStart;
	if( fabsf( outValue ) < Mathf::SmallestNonDenormal )
	{
		return 0.0f;
	}
	return outValue;
}

}	// namespace NervGear

