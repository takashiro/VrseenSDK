#include "ModelFile.h"

#include <math.h>
#include "VBasicmath.h"
#include "VAlgorithm.h"
#include "VArray.h"
#include "VBuffer.h"
#include "VBinaryStream.h"
#include "VString.h"
#include "VJson.h"
#include "MappedFile.h"
#include "VPath.h"
#include "VEglDriver.h"

#include <sstream>

#include "3rdparty/minizip/unzip.h"
#include "VTexture.h"
#include "ModelRender.h"

// Verbose log, redefine this as LOG() to get lots more info dumped
#define LOGV(...)

#define MEMORY_MAPPED	1

NV_NAMESPACE_BEGIN

namespace StringUtils
{

    // Convert a string to a common type.
    //

    template< typename _type_ > inline size_t StringTo( _type_ & value, const char * string ) { return 0; }

    template< typename _type_ > inline size_t StringTo( _type_ * valueArray, const int count, const char * string )
    {
        size_t length = 0;
        length += strspn( string + length, "{ \t\n\r" );
        for ( int i = 0; i < count; i++ )
        {
            length += StringTo< _type_ >( valueArray[i], string + length );
        }
        length += strspn( string + length, "} \t\n\r" );
        return length;
    }

    template< typename _type_ > inline size_t StringTo( VArray< _type_ > & valueArray, const char * string )
    {
        size_t length = 0;
        length += strspn( string + length, "{ \t\n\r" );
        for ( ; ; )
        {
            _type_ value;
            size_t s = StringTo< _type_ >( value, string + length );
            if ( s == 0 ) break;
            valueArray.append( value );
            length += s;
        }
        length += strspn( string + length, "} \t\n\r" );
        return length;
    }

    // specializations

    template<> inline size_t StringTo( short &          value, const char * str ) { char * endptr; value = (short) strtol( str, &endptr, 10 ); return endptr - str; }
    template<> inline size_t StringTo( unsigned short & value, const char * str ) { char * endptr; value = (unsigned short) strtoul( str, &endptr, 10 ); return endptr - str; }
    template<> inline size_t StringTo( int &            value, const char * str ) { char * endptr; value = strtol( str, &endptr, 10 ); return endptr - str; }
    template<> inline size_t StringTo( unsigned int &   value, const char * str ) { char * endptr; value = strtoul( str, &endptr, 10 ); return endptr - str; }
    template<> inline size_t StringTo( float &          value, const char * str ) { char * endptr; value = strtof( str, &endptr ); return endptr - str; }
    template<> inline size_t StringTo( double &         value, const char * str ) { char * endptr; value = strtod( str, &endptr ); return endptr - str; }

    template<> inline size_t StringTo( V2Vectf & value, const char * string ) { return StringTo( &value.x, 2, string ); }
    template<> inline size_t StringTo( V2Vectd & value, const char * string ) { return StringTo( &value.x, 2, string ); }
    template<> inline size_t StringTo( V2Vecti & value, const char * string ) { return StringTo( &value.x, 2, string ); }

    template<> inline size_t StringTo( V3Vectf & value, const char * string ) { return StringTo( &value.x, 3, string ); }
    template<> inline size_t StringTo( V3Vectd & value, const char * string ) { return StringTo( &value.x, 3, string ); }
    template<> inline size_t StringTo( V3Vecti & value, const char * string ) { return StringTo( &value.x, 3, string ); }

    template<> inline size_t StringTo( V4Vectf & value, const char * string ) { return StringTo( &value.x, 4, string ); }
    template<> inline size_t StringTo( V4Vectd & value, const char * string ) { return StringTo( &value.x, 4, string ); }
    template<> inline size_t StringTo( V4Vecti & value, const char * string ) { return StringTo( &value.x, 4, string ); }

    template<> inline size_t StringTo( VR4Matrixf & value, const char * string ) { return StringTo( &value.M[0][0], 16, string ); }
    template<> inline size_t StringTo( VR4Matrixd & value, const char * string ) { return StringTo( &value.M[0][0], 16, string ); }

    template<> inline size_t StringTo( VQuatf &    value, const char * string ) { return StringTo( &value.x, 4, string ); }
    template<> inline size_t StringTo( VQuatd &    value, const char * string ) { return StringTo( &value.x, 4, string ); }

    template<> inline size_t StringTo( VPlanef &   value, const char * string ) { return StringTo( &value.N.x, 4, string ); }
    template<> inline size_t StringTo( VPlaned &   value, const char * string ) { return StringTo( &value.N.x, 4, string ); }

    template<> inline size_t StringTo( VBoxf & value, const char * string ) { return StringTo( value.b, 2, string ); }
    template<> inline size_t StringTo( VBoxd & value, const char * string ) { return StringTo( value.b, 2, string ); }

} // StringUtils

ModelFile::ModelFile() :
	UsingSrgbTextures( false )
{
}

ModelFile::~ModelFile()
{
	vInfo("Destroying ModelFileModel " << FileName);

    for (const ModelTexture &texture : Textures) {
        glDeleteTextures(1, &texture.texid.id());
    }

	for ( int j = 0; j < Def.surfaces.length(); j++ )
	{
		const_cast<VGlGeometry *>(&Def.surfaces[j].geo)->destroy();
	}
}

SurfaceDef * ModelFile::FindNamedSurface( const char * name ) const
{
	for ( int j = 0; j < Def.surfaces.length(); j++ )
	{
		const SurfaceDef & sd = Def.surfaces[j];
		if ( sd.surfaceName.icompare( name ) == 0 )
		{
			vInfo("Found named surface " << name);
			return const_cast<SurfaceDef*>(&sd);
		}
	}
	vInfo("Did not find named surface " << name);
	return NULL;
}

const ModelTexture * ModelFile::FindNamedTexture( const char * name ) const
{
	for ( int i = 0; i < Textures.length(); i++ )
	{
		const ModelTexture & st = Textures[i];
		if ( st.name.icompare( name ) == 0 )
		{
			vInfo("Found named texture " << name);
			return &st;
		}
	}
	vInfo("Did not find named texture " << name);
	return NULL;
}

const ModelJoint * ModelFile::FindNamedJoint( const char *name ) const
{
	for ( int i = 0; i < Joints.length(); i++ )
	{
		const ModelJoint & joint = Joints[i];
		if ( joint.name.icompare( name ) == 0 )
		{
			vInfo("Found named joint " << name);
			return &joint;
		}
	}
	vInfo("Did not find named joint " << name);
	return NULL;
}

const ModelTag * ModelFile::FindNamedTag(const VString &name) const
{
	for ( int i = 0; i < Tags.length(); i++ )
	{
		const ModelTag & tag = Tags[i];
		if ( tag.name.icompare( name ) == 0 )
        {
            vInfo("Found named tag " << name);
			return &tag;
		}
    }
    vInfo("Did not find named tag " << name);
	return NULL;
}

VBoxf ModelFile::GetBounds() const
{
    VBoxf modelBounds;
	modelBounds.Clear();
	for ( int j = 0; j < Def.surfaces.length(); j++ )
	{
		const SurfaceDef & sd = Def.surfaces[j];
		modelBounds.AddPoint( sd.cullingBounds.b[0] );
		modelBounds.AddPoint( sd.cullingBounds.b[1] );
	}
	return modelBounds;
}

NV_NAMESPACE_END
