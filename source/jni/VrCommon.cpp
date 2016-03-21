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
#include <VString.h>
#include "MemBuffer.h"
#include "Android/GlUtils.h"		// for egl declarations
#include "Android/LogUtils.h"
#include <algorithm>

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

//void SortStringArray(VArray<VString> &strings)
//{
//
//    if (strings.size() <= 1) {
//        return;
//	}
//
//    qsort(&strings[0], strings.size(), sizeof( VString ), [](const void *a, const void *b){
//        const VString *sa = ( VString * )a;
//        const VString *sb = ( VString * )b;
//        return sa->icompare( *sb );
//    });
//}

// DirPath should by a directory with a trailing slash.
// Returns all files in all search paths, as unique relative paths.
// Subdirectories will have a trailing slash.
// All files and directories that start with . are skipped.
//StringHash< VString > RelativeDirectoryFileList( const VArray< VString > & searchPaths, const char * RelativeDirPath )
//{
//	//Check each of the mirrors in searchPaths and build up a list of unique strings
//	StringHash< VString >	uniqueStrings;
//
//	const int numSearchPaths = searchPaths.length();
//	for ( int index = 0; index < numSearchPaths; ++index )
//	{
//		const VString fullPath = searchPaths[index] + VString( RelativeDirPath );
//
//		DIR * dir = opendir( fullPath.toCString() );
//		if ( dir != NULL )
//		{
//			struct dirent * entry;
//			while ( ( entry = readdir( dir ) ) != NULL )
//			{
//				if ( entry->d_name[ 0 ] == '.' )
//				{
//					continue;
//				}
//				if ( entry->d_type == DT_DIR )
//				{
//					VString s( RelativeDirPath );
//					s += entry->d_name;
//					s += "/";
//                    uniqueStrings.insert( s, s );
//				}
//				else if ( entry->d_type == DT_REG )
//				{
//					VString s( RelativeDirPath );
//					s += entry->d_name;
//                    uniqueStrings.insert( s, s );
//				}
//			}
//			closedir( dir );
//		}
//	}
//
//	return uniqueStrings;
//}


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

