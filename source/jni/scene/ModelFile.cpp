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

#include "unzip.h"
#include "GlTexture.h"
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

	for ( int i = 0; i < Textures.length(); i++ )
	{
		FreeTexture( Textures[i].texid );
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
//*******************//
//Model Loading Fixed//
//*******************//

//void ModelTextureRender(ModelFile& model, const VJson& render_model, const MaterialParms& materialParms)
//{
//    enum TextureOcclusion
//    {
//        TEXTURE_OCCLUSION_OPAQUE,
//        TEXTURE_OCCLUSION_PERFORATED,
//        TEXTURE_OCCLUSION_TRANSPARENT
//    };

//    VArray< GlTexture > glTextures;

//    const VJson &texture_array( render_model.value( "textures" ) );
//    if ( texture_array.isArray() )
//    {
//        const VJsonArray &textures = texture_array.toArray();
//        for (const VJson &texture : textures) {
//            if ( texture.isObject() )
//            {
//                const VString name = texture.value( "name" ).toString();

//                // Try to match the texture names with the already loaded texture
//                // and create a default texture if the texture file is missing.
//                int i = 0;
//                for ( ; i < model.Textures.length(); i++ )
//                {
//                    if ( model.Textures[i].name.icompare(name) == 0 )
//                    {
//                        break;
//                    }
//                }
//                if ( i == model.Textures.length() )
//                {
//                    LOG( "texture %s defaulted", name.toUtf8().data() );
//                    // Create a default texture.
//                    LoadModelFileTexture( model, name.toUtf8().data(), NULL, 0, materialParms );
//                }
//                glTextures.append( model.Textures[i].texid );

//                const VString usage = texture.value( "usage" ).toString();
//                if ( usage == "diffuse" )
//                {
//                    if ( materialParms.EnableDiffuseAniso == true )
//                    {
//                        MakeTextureAniso( model.Textures[i].texid, 2.0f );
//                    }
//                }
//                else if ( usage == "emissive" )
//                {
//                    if ( materialParms.EnableEmissiveLodClamp == true )
//                    {
//                        // LOD clamp lightmap textures to avoid light bleeding
//                        MakeTextureLodClamped( model.Textures[i].texid, 1 );
//                    }
//                }
//                /*
//                const String occlusion = texture.GetChildStringByName( "occlusion" );

//                TextureOcclusion textureOcclusion = TEXTURE_OCCLUSION_OPAQUE;
//                if ( occlusion == "opaque" )			{ textureOcclusion = TEXTURE_OCCLUSION_OPAQUE; }
//                else if ( occlusion == "perforated" )	{ textureOcclusion = TEXTURE_OCCLUSION_PERFORATED; }
//                else if ( occlusion == "transparent" )	{ textureOcclusion = TEXTURE_OCCLUSION_TRANSPARENT; }
//                */
//            }
//        }
//    }
//    return ;
//}

//void ModelJointsRender(ModelFile& model, const VJson& render_model)
//{
//    const VJson &joint_array( render_model.value( "joints" ) );
//    if ( joint_array.isArray() )
//    {
//        model.Joints.clear();

//        const VJsonArray &joints = joint_array.toArray();
//        for ( const VJson &joint : joints ) {
//            if ( joint.isObject() )
//            {
//                const uint index = model.Joints.allocBack();
//                model.Joints[index].index = index;
//                model.Joints[index].name = joint.value( "name" ).toString();
//                StringUtils::StringTo( model.Joints[index].transform, joint.value( "transform" ).toStdString().c_str() );
//                model.Joints[index].animation = MODEL_JOINT_ANIMATION_NONE;
//                const std::string animation = joint.value( "animation" ).toStdString();
//                if ( animation == "none" )			{ model.Joints[index].animation = MODEL_JOINT_ANIMATION_NONE; }
//                else if ( animation == "rotate" )	{ model.Joints[index].animation = MODEL_JOINT_ANIMATION_ROTATE; }
//                else if ( animation == "sway" )		{ model.Joints[index].animation = MODEL_JOINT_ANIMATION_SWAY; }
//                else if ( animation == "bob" )		{ model.Joints[index].animation = MODEL_JOINT_ANIMATION_BOB; }
//                model.Joints[index].parameters.x = joint.value( "parmX" ).toDouble();
//                model.Joints[index].parameters.y = joint.value( "parmY" ).toDouble();
//                model.Joints[index].parameters.z = joint.value( "parmZ" ).toDouble();
//                model.Joints[index].timeOffset = joint.value( "timeOffset" ).toDouble();
//                model.Joints[index].timeScale = joint.value( "timeScale" ).toDouble();
//            }
//        }
//    }
//    return ;
//}

//void ModelTagsRender(ModelFile& model, const VJson& render_model)
//{
//    const VJson tag_array( render_model.value( "tags" ) );
//    if ( tag_array.isArray() )
//    {
//        model.Tags.clear();

//        const VJsonArray &tags = tag_array.toArray();
//        for ( const VJson &tag : tags ) {
//            if ( tag.isObject() )
//            {
//                const uint index = model.Tags.allocBack();
//                model.Tags[index].name = tag.value( "name" ).toString();
//                StringUtils::StringTo( model.Tags[index].matrix, 		tag.value( "matrix" ).toStdString().c_str() );
//                StringUtils::StringTo( model.Tags[index].jointIndices, 	tag.value( "jointIndices" ).toStdString().c_str() );
//                StringUtils::StringTo( model.Tags[index].jointWeights, 	tag.value( "jointWeights" ).toStdString().c_str() );
//            }
//        }
//    }
//    return ;
//}

//void ModelSurfacesRender(ModelFile& model, const VJson& render_model,
//                         const char * modelsBin, const int modelsBinLength, const ModelGlPrograms & programs)
//{
//    const VBinaryFile bin( (const UByte *)modelsBin, modelsBinLength );

//    if ( modelsBin != NULL && bin.readUint() != 0x6272766F )
//    {
//        LOG( "LoadModelFileJson: bad binary file for %s", model.FileName.toCString() );
//        return;
//    }

//    const VJson surface_array( render_model.value( "surfaces" ) );
//    if ( surface_array.isArray() )
//    {
//        const VJsonArray &surfaces = surface_array.toArray();
//        for (const VJson &surface : surfaces) {
//            if ( surface.isObject() )
//            {
//                const uint index = model.Def.surfaces.allocBack();

//                //
//                // Source Meshes
//                //

//                const VJson &source(surface.value( "source" ));
//                if (source.isArray()) {
//                    const VJsonArray &elements = source.toArray();
//                    for (const VJson &e : elements) {
//                        if ( model.Def.surfaces[index].surfaceName.length() ) {
//                            model.Def.surfaces[index].surfaceName += ";";
//                        }
//                        model.Def.surfaces[index].surfaceName += e.toString();
//                    }
//                }

//                LOGV( "surface %s", model.Def.surfaces[index].surfaceName.toCString() );

//                //
//                // Surface Material
//                //

//                enum
//                {
//                    MATERIAL_TYPE_OPAQUE,
//                    MATERIAL_TYPE_PERFORATED,
//                    MATERIAL_TYPE_TRANSPARENT,
//                    MATERIAL_TYPE_ADDITIVE
//                } materialType = MATERIAL_TYPE_OPAQUE;

//                int diffuseTextureIndex = -1;
//                int normalTextureIndex = -1;
//                int specularTextureIndex = -1;
//                int emissiveTextureIndex = -1;
//                int reflectionTextureIndex = -1;

//                const VJson &material( surface.value( "material" ) );
//                if ( material.isObject() )
//                {
//                    const VString type = material.value( "type" ).toString();

//                    if ( type == "opaque" )				{ materialType = MATERIAL_TYPE_OPAQUE; }
//                    else if ( type == "perforated" )	{ materialType = MATERIAL_TYPE_PERFORATED; }
//                    else if ( type == "transparent" )	{ materialType = MATERIAL_TYPE_TRANSPARENT; }
//                    else if ( type == "additive" )		{ materialType = MATERIAL_TYPE_ADDITIVE; }

//                    diffuseTextureIndex		= material.value( "diffuse", -1 ).toInt();
//                    normalTextureIndex		= material.value( "normal", -1 ).toInt();
//                    specularTextureIndex	= material.value( "specular", -1 ).toInt();
//                    emissiveTextureIndex	= material.value( "emissive", -1 ).toInt();
//                    reflectionTextureIndex	= material.value( "reflection", -1 ).toInt();
//                }

//                //
//                // Surface Bounds
//                //

//                StringUtils::StringTo( model.Def.surfaces[index].cullingBounds, surface.value("bounds").toStdString().c_str() );

//                //
//                // Vertices
//                //

//                VertexAttribs attribs;

//                const VJson &vertices( surface.value( "vertices" ) );
//                if ( vertices.isObject() )
//                {
//                    const int vertexCount = std::min( vertices.value( "vertexCount" ).toInt(), 1024*1024 );
//                    // LOG( "%5d vertices", vertexCount );

//                    ReadModelArray( attribs.position,     vertices.value( "position" ).toStdString().c_str(),		bin, vertexCount );
//                    ReadModelArray( attribs.normal,       vertices.value( "normal" ).toStdString().c_str(),		bin, vertexCount );
//                    ReadModelArray( attribs.tangent,      vertices.value( "tangent" ).toStdString().c_str(),		bin, vertexCount );
//                    ReadModelArray( attribs.binormal,     vertices.value( "binormal" ).toStdString().c_str(),		bin, vertexCount );
//                    ReadModelArray( attribs.color,        vertices.value( "color" ).toStdString().c_str(),			bin, vertexCount );
//                    ReadModelArray( attribs.uv0,          vertices.value( "uv0" ).toStdString().c_str(),			bin, vertexCount );
//                    ReadModelArray( attribs.uv1,          vertices.value( "uv1" ).toStdString().c_str(),			bin, vertexCount );
//                    ReadModelArray( attribs.jointIndices, vertices.value( "jointIndices" ).toStdString().c_str(),	bin, vertexCount );
//                    ReadModelArray( attribs.jointWeights, vertices.value( "jointWeights" ).toStdString().c_str(),	bin, vertexCount );
//                }

//                //
//                // Triangles
//                //

//                VArray< ushort > indices;

//                const VJson &triangles( surface.value( "triangles" ) );
//                if ( triangles.isObject() )
//                {
//                    const int indexCount = std::min( triangles.value( "indexCount" ).toInt(), 1024 * 1024 * 1024 );
//                    // LOG( "%5d indices", indexCount );

//                    ReadModelArray( indices, triangles.value( "indices" ).toStdString().c_str(), bin, indexCount );
//                }

//                //
//                // Setup geometry, textures and render programs now that the vertex attributes are known.
//                //

//                model.Def.surfaces[index].geo.createGlGeometry( attribs, indices );

//                const char * materialTypeString = "opaque";
//                OVR_UNUSED( materialTypeString );	// we'll get warnings if the LOGV's compile out

//                // set up additional material flags for the surface
//                if ( materialType == MATERIAL_TYPE_PERFORATED )
//                {
//                    // Just blend because alpha testing is rather expensive.
//                    model.Def.surfaces[index].materialDef.gpuState.blendEnable = true;
//                    model.Def.surfaces[index].materialDef.gpuState.depthMaskEnable = false;
//                    model.Def.surfaces[index].materialDef.gpuState.blendSrc = GL_SRC_ALPHA;
//                    model.Def.surfaces[index].materialDef.gpuState.blendDst = GL_ONE_MINUS_SRC_ALPHA;
//                    materialTypeString = "perforated";
//                }
//                else if ( materialType == MATERIAL_TYPE_TRANSPARENT || materialParms.Transparent )
//                {
//                    model.Def.surfaces[index].materialDef.gpuState.blendEnable = true;
//                    model.Def.surfaces[index].materialDef.gpuState.depthMaskEnable = false;
//                    model.Def.surfaces[index].materialDef.gpuState.blendSrc = GL_SRC_ALPHA;
//                    model.Def.surfaces[index].materialDef.gpuState.blendDst = GL_ONE_MINUS_SRC_ALPHA;
//                    materialTypeString = "transparent";
//                }
//                else if ( materialType == MATERIAL_TYPE_ADDITIVE )
//                {
//                    model.Def.surfaces[index].materialDef.gpuState.blendEnable = true;
//                    model.Def.surfaces[index].materialDef.gpuState.depthMaskEnable = false;
//                    model.Def.surfaces[index].materialDef.gpuState.blendSrc = GL_ONE;
//                    model.Def.surfaces[index].materialDef.gpuState.blendDst = GL_ONE;
//                    materialTypeString = "additive";
//                }

//                const bool skinned = (	attribs.jointIndices.size() == attribs.position.size() &&
//                                        attribs.jointWeights.size() == attribs.position.size() );

//                if ( diffuseTextureIndex >= 0 && diffuseTextureIndex < glTextures.length() )
//                {
//                    model.Def.surfaces[index].materialDef.textures[0] = glTextures[diffuseTextureIndex];

//                    if ( emissiveTextureIndex >= 0 && emissiveTextureIndex < glTextures.length() )
//                    {
//                        model.Def.surfaces[index].materialDef.textures[1] = glTextures[emissiveTextureIndex];

//                        if (	normalTextureIndex >= 0 && normalTextureIndex < glTextures.length() &&
//                                specularTextureIndex >= 0 && specularTextureIndex < glTextures.length() &&
//                                reflectionTextureIndex >= 0 && reflectionTextureIndex < glTextures.length() )
//                        {
//                            // reflection mapped material;
//                            model.Def.surfaces[index].materialDef.textures[2] = glTextures[normalTextureIndex];
//                            model.Def.surfaces[index].materialDef.textures[3] = glTextures[specularTextureIndex];
//                            model.Def.surfaces[index].materialDef.textures[4] = glTextures[reflectionTextureIndex];

//                            model.Def.surfaces[index].materialDef.numTextures = 5;
//                            if ( skinned )
//                            {
//                                if ( programs.ProgSkinnedReflectionMapped == NULL )
//                                {
//                                    FAIL( "No ProgSkinnedReflectionMapped set");
//                                }
//                                model.Def.surfaces[index].materialDef.programObject = programs.ProgSkinnedReflectionMapped->program;
//                                model.Def.surfaces[index].materialDef.uniformMvp = programs.ProgSkinnedReflectionMapped->uniformModelViewProMatrix;
//                                model.Def.surfaces[index].materialDef.uniformModel = programs.ProgSkinnedReflectionMapped->uniformModelMatrix;
//                                model.Def.surfaces[index].materialDef.uniformView = programs.ProgSkinnedReflectionMapped->uniformViewMatrix;
//                                model.Def.surfaces[index].materialDef.uniformJoints = programs.ProgSkinnedReflectionMapped->uniformJoints;
//                                LOGV( "%s skinned reflection mapped material", materialTypeString );
//                            }
//                            else
//                            {
//                                if ( programs.ProgReflectionMapped == NULL )
//                                {
//                                    FAIL( "No ProgReflectionMapped set");
//                                }
//                                model.Def.surfaces[index].materialDef.programObject = programs.ProgReflectionMapped->program;
//                                model.Def.surfaces[index].materialDef.uniformMvp = programs.ProgReflectionMapped->uniformModelViewProMatrix;
//                                model.Def.surfaces[index].materialDef.uniformModel = programs.ProgReflectionMapped->uniformModelMatrix;
//                                model.Def.surfaces[index].materialDef.uniformView = programs.ProgReflectionMapped->uniformViewMatrix;
//                                LOGV( "%s reflection mapped material", materialTypeString );
//                            }
//                        }
//                        else
//                        {
//                            // light mapped material
//                            model.Def.surfaces[index].materialDef.numTextures = 2;
//                            if ( skinned )
//                            {
//                                if ( programs.ProgSkinnedLightMapped == NULL )
//                                {
//                                    FAIL( "No ProgSkinnedLightMapped set");
//                                }
//                                model.Def.surfaces[index].materialDef.programObject = programs.ProgSkinnedLightMapped->program;
//                                model.Def.surfaces[index].materialDef.uniformMvp = programs.ProgSkinnedLightMapped->uniformModelViewProMatrix;
//                                model.Def.surfaces[index].materialDef.uniformJoints = programs.ProgSkinnedLightMapped->uniformJoints;
//                                LOGV( "%s skinned light mapped material", materialTypeString );
//                            }
//                            else
//                            {
//                                if ( programs.ProgLightMapped == NULL )
//                                {
//                                    FAIL( "No ProgLightMapped set");
//                                }
//                                model.Def.surfaces[index].materialDef.programObject = programs.ProgLightMapped->program;
//                                model.Def.surfaces[index].materialDef.uniformMvp = programs.ProgLightMapped->uniformModelViewProMatrix;
//                                LOGV( "%s light mapped material", materialTypeString );
//                            }
//                        }
//                    }
//                    else
//                    {
//                        // diffuse only material
//                        model.Def.surfaces[index].materialDef.numTextures = 1;
//                        if ( skinned )
//                        {
//                            if ( programs.ProgSkinnedSingleTexture == NULL )
//                            {
//                                FAIL( "No ProgSkinnedSingleTexture set");
//                            }
//                            model.Def.surfaces[index].materialDef.programObject = programs.ProgSkinnedSingleTexture->program;
//                            model.Def.surfaces[index].materialDef.uniformMvp = programs.ProgSkinnedSingleTexture->uniformModelViewProMatrix;
//                            model.Def.surfaces[index].materialDef.uniformJoints = programs.ProgSkinnedSingleTexture->uniformJoints;
//                            LOGV( "%s skinned diffuse only material", materialTypeString );
//                        }
//                        else
//                        {
//                            if ( programs.ProgSingleTexture == NULL )
//                            {
//                                FAIL( "No ProgSingleTexture set");
//                            }
//                            model.Def.surfaces[index].materialDef.programObject = programs.ProgSingleTexture->program;
//                            model.Def.surfaces[index].materialDef.uniformMvp = programs.ProgSingleTexture->uniformModelViewProMatrix;
//                            LOGV( "%s diffuse only material", materialTypeString );
//                        }
//                    }
//                }
//                else if ( attribs.color.length() > 0 )
//                {
//                    // vertex color material
//                    model.Def.surfaces[index].materialDef.numTextures = 0;
//                    if ( skinned )
//                    {
//                        if ( programs.ProgSkinnedVertexColor == NULL )
//                        {
//                            FAIL( "No ProgSkinnedVertexColor set");
//                        }
//                        model.Def.surfaces[index].materialDef.programObject = programs.ProgSkinnedVertexColor->program;
//                        model.Def.surfaces[index].materialDef.uniformMvp = programs.ProgSkinnedVertexColor->uniformModelViewProMatrix;
//                        LOGV( "%s skinned vertex color material", materialTypeString );
//                    }
//                    else
//                    {
//                        if ( programs.ProgVertexColor == NULL )
//                        {
//                            FAIL( "No ProgVertexColor set");
//                        }
//                        model.Def.surfaces[index].materialDef.programObject = programs.ProgVertexColor->program;
//                        model.Def.surfaces[index].materialDef.uniformMvp = programs.ProgVertexColor->uniformModelViewProMatrix;
//                        LOGV( "%s vertex color material", materialTypeString );
//                    }
//                }
//                else
//                {
//                    // surface without texture or vertex colors
//                    model.Def.surfaces[index].materialDef.textures[0] = 0;
//                    model.Def.surfaces[index].materialDef.numTextures = 1;
//                    if ( skinned )
//                    {
//                        if ( programs.ProgSkinnedSingleTexture == NULL )
//                        {
//                            FAIL( "No ProgSkinnedSingleTexture set");
//                        }
//                        model.Def.surfaces[index].materialDef.programObject = programs.ProgSkinnedSingleTexture->program;
//                        model.Def.surfaces[index].materialDef.uniformMvp = programs.ProgSingleTexture->uniformModelViewProMatrix;
//                        LOGV( "%s skinned default texture material", materialTypeString );
//                    }
//                    else
//                    {
//                        if ( programs.ProgSingleTexture == NULL )
//                        {
//                            FAIL( "No ProgSingleTexture set");
//                        }
//                        model.Def.surfaces[index].materialDef.programObject = programs.ProgSingleTexture->program;
//                        model.Def.surfaces[index].materialDef.uniformMvp = programs.ProgSingleTexture->uniformModelViewProMatrix;
//                        LOGV( "%s default texture material", materialTypeString );
//                    }
//                }

//                if ( materialParms.PolygonOffset )
//                {
//                    model.Def.surfaces[index].materialDef.gpuState.polygonOffsetEnable = true;
//                    LOGV( "polygon offset material" );
//                }
//            }
//        }
//    }

//    return ;
//}

//void ModelRender(ModelFile& model, const VJson& models, const MaterialParms& materialParms,
//                 const char * modelsBin, const int modelsBinLength, const ModelGlPrograms & programs)
//{
//    const VJson &render_model = models.value( "render_model" );
//    if ( render_model.isObject() )
//    {
//        LOG( "loading render model.." );

//        // Render Model Textures
//        ModelTextureRender(model, render_model, materialParms);

//        // Render Model Joints
//        ModelJointsRender(model, render_model);

//        // Render Model Tags
//        ModelTagsRender(model, render_model);



//        //
//        // Render Model Surfaces
//        //

//        const VJson surface_array( render_model.value( "surfaces" ) );
//        if ( surface_array.isArray() )
//        {
//            const VJsonArray &surfaces = surface_array.toArray();
//            for (const VJson &surface : surfaces) {
//                if ( surface.isObject() )
//                {
//                    const uint index = model.Def.surfaces.allocBack();

//                    //
//                    // Source Meshes
//                    //

//                    const VJson &source(surface.value( "source" ));
//                    if (source.isArray()) {
//                        const VJsonArray &elements = source.toArray();
//                        for (const VJson &e : elements) {
//                            if ( model.Def.surfaces[index].surfaceName.length() ) {
//                                model.Def.surfaces[index].surfaceName += ";";
//                            }
//                            model.Def.surfaces[index].surfaceName += e.toString();
//                        }
//                    }

//                    LOGV( "surface %s", model.Def.surfaces[index].surfaceName.toCString() );

//                    //
//                    // Surface Material
//                    //

//                    enum
//                    {
//                        MATERIAL_TYPE_OPAQUE,
//                        MATERIAL_TYPE_PERFORATED,
//                        MATERIAL_TYPE_TRANSPARENT,
//                        MATERIAL_TYPE_ADDITIVE
//                    } materialType = MATERIAL_TYPE_OPAQUE;

//                    int diffuseTextureIndex = -1;
//                    int normalTextureIndex = -1;
//                    int specularTextureIndex = -1;
//                    int emissiveTextureIndex = -1;
//                    int reflectionTextureIndex = -1;

//                    const VJson &material( surface.value( "material" ) );
//                    if ( material.isObject() )
//                    {
//                        const VString type = material.value( "type" ).toString();

//                        if ( type == "opaque" )				{ materialType = MATERIAL_TYPE_OPAQUE; }
//                        else if ( type == "perforated" )	{ materialType = MATERIAL_TYPE_PERFORATED; }
//                        else if ( type == "transparent" )	{ materialType = MATERIAL_TYPE_TRANSPARENT; }
//                        else if ( type == "additive" )		{ materialType = MATERIAL_TYPE_ADDITIVE; }

//                        diffuseTextureIndex		= material.value( "diffuse", -1 ).toInt();
//                        normalTextureIndex		= material.value( "normal", -1 ).toInt();
//                        specularTextureIndex	= material.value( "specular", -1 ).toInt();
//                        emissiveTextureIndex	= material.value( "emissive", -1 ).toInt();
//                        reflectionTextureIndex	= material.value( "reflection", -1 ).toInt();
//                    }

//                    //
//                    // Surface Bounds
//                    //

//                    StringUtils::StringTo( model.Def.surfaces[index].cullingBounds, surface.value("bounds").toStdString().c_str() );

//                    //
//                    // Vertices
//                    //

//                    VertexAttribs attribs;

//                    const VJson &vertices( surface.value( "vertices" ) );
//                    if ( vertices.isObject() )
//                    {
//                        const int vertexCount = std::min( vertices.value( "vertexCount" ).toInt(), 1024*1024 );
//                        // LOG( "%5d vertices", vertexCount );

//                        ReadModelArray( attribs.position,     vertices.value( "position" ).toStdString().c_str(),		bin, vertexCount );
//                        ReadModelArray( attribs.normal,       vertices.value( "normal" ).toStdString().c_str(),		bin, vertexCount );
//                        ReadModelArray( attribs.tangent,      vertices.value( "tangent" ).toStdString().c_str(),		bin, vertexCount );
//                        ReadModelArray( attribs.binormal,     vertices.value( "binormal" ).toStdString().c_str(),		bin, vertexCount );
//                        ReadModelArray( attribs.color,        vertices.value( "color" ).toStdString().c_str(),			bin, vertexCount );
//                        ReadModelArray( attribs.uv0,          vertices.value( "uv0" ).toStdString().c_str(),			bin, vertexCount );
//                        ReadModelArray( attribs.uv1,          vertices.value( "uv1" ).toStdString().c_str(),			bin, vertexCount );
//                        ReadModelArray( attribs.jointIndices, vertices.value( "jointIndices" ).toStdString().c_str(),	bin, vertexCount );
//                        ReadModelArray( attribs.jointWeights, vertices.value( "jointWeights" ).toStdString().c_str(),	bin, vertexCount );
//                    }

//                    //
//                    // Triangles
//                    //

//                    VArray< ushort > indices;

//                    const VJson &triangles( surface.value( "triangles" ) );
//                    if ( triangles.isObject() )
//                    {
//                        const int indexCount = std::min( triangles.value( "indexCount" ).toInt(), 1024 * 1024 * 1024 );
//                        // LOG( "%5d indices", indexCount );

//                        ReadModelArray( indices, triangles.value( "indices" ).toStdString().c_str(), bin, indexCount );
//                    }

//                    //
//                    // Setup geometry, textures and render programs now that the vertex attributes are known.
//                    //

//                    model.Def.surfaces[index].geo.createGlGeometry( attribs, indices );

//                    const char * materialTypeString = "opaque";
//                    OVR_UNUSED( materialTypeString );	// we'll get warnings if the LOGV's compile out

//                    // set up additional material flags for the surface
//                    if ( materialType == MATERIAL_TYPE_PERFORATED )
//                    {
//                        // Just blend because alpha testing is rather expensive.
//                        model.Def.surfaces[index].materialDef.gpuState.blendEnable = true;
//                        model.Def.surfaces[index].materialDef.gpuState.depthMaskEnable = false;
//                        model.Def.surfaces[index].materialDef.gpuState.blendSrc = GL_SRC_ALPHA;
//                        model.Def.surfaces[index].materialDef.gpuState.blendDst = GL_ONE_MINUS_SRC_ALPHA;
//                        materialTypeString = "perforated";
//                    }
//                    else if ( materialType == MATERIAL_TYPE_TRANSPARENT || materialParms.Transparent )
//                    {
//                        model.Def.surfaces[index].materialDef.gpuState.blendEnable = true;
//                        model.Def.surfaces[index].materialDef.gpuState.depthMaskEnable = false;
//                        model.Def.surfaces[index].materialDef.gpuState.blendSrc = GL_SRC_ALPHA;
//                        model.Def.surfaces[index].materialDef.gpuState.blendDst = GL_ONE_MINUS_SRC_ALPHA;
//                        materialTypeString = "transparent";
//                    }
//                    else if ( materialType == MATERIAL_TYPE_ADDITIVE )
//                    {
//                        model.Def.surfaces[index].materialDef.gpuState.blendEnable = true;
//                        model.Def.surfaces[index].materialDef.gpuState.depthMaskEnable = false;
//                        model.Def.surfaces[index].materialDef.gpuState.blendSrc = GL_ONE;
//                        model.Def.surfaces[index].materialDef.gpuState.blendDst = GL_ONE;
//                        materialTypeString = "additive";
//                    }

//                    const bool skinned = (	attribs.jointIndices.size() == attribs.position.size() &&
//                                            attribs.jointWeights.size() == attribs.position.size() );

//                    if ( diffuseTextureIndex >= 0 && diffuseTextureIndex < glTextures.length() )
//                    {
//                        model.Def.surfaces[index].materialDef.textures[0] = glTextures[diffuseTextureIndex];

//                        if ( emissiveTextureIndex >= 0 && emissiveTextureIndex < glTextures.length() )
//                        {
//                            model.Def.surfaces[index].materialDef.textures[1] = glTextures[emissiveTextureIndex];

//                            if (	normalTextureIndex >= 0 && normalTextureIndex < glTextures.length() &&
//                                    specularTextureIndex >= 0 && specularTextureIndex < glTextures.length() &&
//                                    reflectionTextureIndex >= 0 && reflectionTextureIndex < glTextures.length() )
//                            {
//                                // reflection mapped material;
//                                model.Def.surfaces[index].materialDef.textures[2] = glTextures[normalTextureIndex];
//                                model.Def.surfaces[index].materialDef.textures[3] = glTextures[specularTextureIndex];
//                                model.Def.surfaces[index].materialDef.textures[4] = glTextures[reflectionTextureIndex];

//                                model.Def.surfaces[index].materialDef.numTextures = 5;
//                                if ( skinned )
//                                {
//                                    if ( programs.ProgSkinnedReflectionMapped == NULL )
//                                    {
//                                        FAIL( "No ProgSkinnedReflectionMapped set");
//                                    }
//                                    model.Def.surfaces[index].materialDef.programObject = programs.ProgSkinnedReflectionMapped->program;
//                                    model.Def.surfaces[index].materialDef.uniformMvp = programs.ProgSkinnedReflectionMapped->uniformModelViewProMatrix;
//                                    model.Def.surfaces[index].materialDef.uniformModel = programs.ProgSkinnedReflectionMapped->uniformModelMatrix;
//                                    model.Def.surfaces[index].materialDef.uniformView = programs.ProgSkinnedReflectionMapped->uniformViewMatrix;
//                                    model.Def.surfaces[index].materialDef.uniformJoints = programs.ProgSkinnedReflectionMapped->uniformJoints;
//                                    LOGV( "%s skinned reflection mapped material", materialTypeString );
//                                }
//                                else
//                                {
//                                    if ( programs.ProgReflectionMapped == NULL )
//                                    {
//                                        FAIL( "No ProgReflectionMapped set");
//                                    }
//                                    model.Def.surfaces[index].materialDef.programObject = programs.ProgReflectionMapped->program;
//                                    model.Def.surfaces[index].materialDef.uniformMvp = programs.ProgReflectionMapped->uniformModelViewProMatrix;
//                                    model.Def.surfaces[index].materialDef.uniformModel = programs.ProgReflectionMapped->uniformModelMatrix;
//                                    model.Def.surfaces[index].materialDef.uniformView = programs.ProgReflectionMapped->uniformViewMatrix;
//                                    LOGV( "%s reflection mapped material", materialTypeString );
//                                }
//                            }
//                            else
//                            {
//                                // light mapped material
//                                model.Def.surfaces[index].materialDef.numTextures = 2;
//                                if ( skinned )
//                                {
//                                    if ( programs.ProgSkinnedLightMapped == NULL )
//                                    {
//                                        FAIL( "No ProgSkinnedLightMapped set");
//                                    }
//                                    model.Def.surfaces[index].materialDef.programObject = programs.ProgSkinnedLightMapped->program;
//                                    model.Def.surfaces[index].materialDef.uniformMvp = programs.ProgSkinnedLightMapped->uniformModelViewProMatrix;
//                                    model.Def.surfaces[index].materialDef.uniformJoints = programs.ProgSkinnedLightMapped->uniformJoints;
//                                    LOGV( "%s skinned light mapped material", materialTypeString );
//                                }
//                                else
//                                {
//                                    if ( programs.ProgLightMapped == NULL )
//                                    {
//                                        FAIL( "No ProgLightMapped set");
//                                    }
//                                    model.Def.surfaces[index].materialDef.programObject = programs.ProgLightMapped->program;
//                                    model.Def.surfaces[index].materialDef.uniformMvp = programs.ProgLightMapped->uniformModelViewProMatrix;
//                                    LOGV( "%s light mapped material", materialTypeString );
//                                }
//                            }
//                        }
//                        else
//                        {
//                            // diffuse only material
//                            model.Def.surfaces[index].materialDef.numTextures = 1;
//                            if ( skinned )
//                            {
//                                if ( programs.ProgSkinnedSingleTexture == NULL )
//                                {
//                                    FAIL( "No ProgSkinnedSingleTexture set");
//                                }
//                                model.Def.surfaces[index].materialDef.programObject = programs.ProgSkinnedSingleTexture->program;
//                                model.Def.surfaces[index].materialDef.uniformMvp = programs.ProgSkinnedSingleTexture->uniformModelViewProMatrix;
//                                model.Def.surfaces[index].materialDef.uniformJoints = programs.ProgSkinnedSingleTexture->uniformJoints;
//                                LOGV( "%s skinned diffuse only material", materialTypeString );
//                            }
//                            else
//                            {
//                                if ( programs.ProgSingleTexture == NULL )
//                                {
//                                    FAIL( "No ProgSingleTexture set");
//                                }
//                                model.Def.surfaces[index].materialDef.programObject = programs.ProgSingleTexture->program;
//                                model.Def.surfaces[index].materialDef.uniformMvp = programs.ProgSingleTexture->uniformModelViewProMatrix;
//                                LOGV( "%s diffuse only material", materialTypeString );
//                            }
//                        }
//                    }
//                    else if ( attribs.color.length() > 0 )
//                    {
//                        // vertex color material
//                        model.Def.surfaces[index].materialDef.numTextures = 0;
//                        if ( skinned )
//                        {
//                            if ( programs.ProgSkinnedVertexColor == NULL )
//                            {
//                                FAIL( "No ProgSkinnedVertexColor set");
//                            }
//                            model.Def.surfaces[index].materialDef.programObject = programs.ProgSkinnedVertexColor->program;
//                            model.Def.surfaces[index].materialDef.uniformMvp = programs.ProgSkinnedVertexColor->uniformModelViewProMatrix;
//                            LOGV( "%s skinned vertex color material", materialTypeString );
//                        }
//                        else
//                        {
//                            if ( programs.ProgVertexColor == NULL )
//                            {
//                                FAIL( "No ProgVertexColor set");
//                            }
//                            model.Def.surfaces[index].materialDef.programObject = programs.ProgVertexColor->program;
//                            model.Def.surfaces[index].materialDef.uniformMvp = programs.ProgVertexColor->uniformModelViewProMatrix;
//                            LOGV( "%s vertex color material", materialTypeString );
//                        }
//                    }
//                    else
//                    {
//                        // surface without texture or vertex colors
//                        model.Def.surfaces[index].materialDef.textures[0] = 0;
//                        model.Def.surfaces[index].materialDef.numTextures = 1;
//                        if ( skinned )
//                        {
//                            if ( programs.ProgSkinnedSingleTexture == NULL )
//                            {
//                                FAIL( "No ProgSkinnedSingleTexture set");
//                            }
//                            model.Def.surfaces[index].materialDef.programObject = programs.ProgSkinnedSingleTexture->program;
//                            model.Def.surfaces[index].materialDef.uniformMvp = programs.ProgSingleTexture->uniformModelViewProMatrix;
//                            LOGV( "%s skinned default texture material", materialTypeString );
//                        }
//                        else
//                        {
//                            if ( programs.ProgSingleTexture == NULL )
//                            {
//                                FAIL( "No ProgSingleTexture set");
//                            }
//                            model.Def.surfaces[index].materialDef.programObject = programs.ProgSingleTexture->program;
//                            model.Def.surfaces[index].materialDef.uniformMvp = programs.ProgSingleTexture->uniformModelViewProMatrix;
//                            LOGV( "%s default texture material", materialTypeString );
//                        }
//                    }

//                    if ( materialParms.PolygonOffset )
//                    {
//                        model.Def.surfaces[index].materialDef.gpuState.polygonOffsetEnable = true;
//                        LOGV( "polygon offset material" );
//                    }
//                }
//            }
//        }
//    }

//    return ;
//}

//void ModelFileJsonLoader( ModelFile & model,
//                        const char * modelsJson, const int modelsJsonLength,
//                        const char * modelsBin, const int modelsBinLength,
//                        const ModelGlPrograms & programs, const MaterialParms & materialParms )
//{
//    LOG( "parsing %s", model.FileName.toCString() );

//    const char * error = NULL;
//    VJson models = VJson::Parse(modelsJson);
//    if ( models.isNull() )
//    {
//        WARN( "LoadModelFileJson: Error loading %s : %s", model.FileName.toCString(), error );
//        return;
//    }

//    if (models.isObject())
//    {
//        ModelRender(model, models, materialParms);
//    }
//    return ;
//}






//-----------------------------------------------------------------------------
//	Model Loading
//-----------------------------------------------------------------------------

void LoadModelFileTexture( ModelFile & model, const char * textureName,
							const char * buffer, const int size, const MaterialParms & materialParms )
{
	ModelTexture tex;
    tex.name = VPath(textureName).baseName();
    int width;
    int height;
    tex.texid = LoadTextureFromBuffer( textureName,buffer, size,
			materialParms.UseSrgbTextureFormats ? TextureFlags_t( TEXTUREFLAG_USE_SRGB ) : TextureFlags_t(),
			width, height );

	// LOG( ( tex.texid.target == GL_TEXTURE_CUBE_MAP ) ? "GL_TEXTURE_CUBE_MAP: %s" : "GL_TEXTURE_2D: %s", textureName );

	// file name metadata for enabling clamp mode
	// Used for sky sides in Tuscany.
	if ( strstr( textureName, "_c." ) )
	{
		MakeTextureClamped( tex.texid );
	}

	model.Textures.append( tex );
}

template< typename _type_ >
void ReadModelArray( VArray< _type_ > & out, const char * string, VBinaryStream &bin, const int numElements)
{
    if (string != NULL && string[0] != '\0' && numElements > 0) {
        if (!bin.read(out, numElements)) {
            StringUtils::StringTo(out, string);
		}
	}
}

void LoadModelFileJson(ModelFile &model,
                        std::istream &modelsJson,
                        VBinaryStream &modelsBin,
						const ModelGlPrograms & programs, const MaterialParms & materialParms )
{
	vInfo("parsing " << model.FileName);

    uint tag = 0;
    modelsBin >> tag;
    if (tag != 0x6272766F)
	{
		vInfo("LoadModelFileJson: bad binary file for " << model.FileName);
		return;
	}

	const char * error = NULL;
    VJson models;
    modelsJson >> models;
	if ( models.isNull() )
	{
		vWarn("LoadModelFileJson: Error loading " << model.FileName << " : " << error);
		return;
	}

	if ( models.isObject() )
	{
		//
		// Render Model
		//

		const VJson &render_model = models.value( "render_model" );
		if ( render_model.isObject() )
		{
			vInfo("loading render model..");

			//
			// Render Model Textures
			//

			enum TextureOcclusion
			{
				TEXTURE_OCCLUSION_OPAQUE,
				TEXTURE_OCCLUSION_PERFORATED,
				TEXTURE_OCCLUSION_TRANSPARENT
			};

			VArray< GlTexture > glTextures;

			const VJson &texture_array( render_model.value( "textures" ) );
			if ( texture_array.isArray() )
			{
                const VJsonArray &textures = texture_array.toArray();
				for (const VJson &texture : textures) {
					if ( texture.isObject() )
					{
                        const VString name = texture.value( "name" ).toString();

						// Try to match the texture names with the already loaded texture
						// and create a default texture if the texture file is missing.
						int i = 0;
						for ( ; i < model.Textures.length(); i++ )
						{
                            if ( model.Textures[i].name.icompare(name) == 0 )
							{
								break;
							}
						}
						if ( i == model.Textures.length() )
						{
                            vInfo("texture " << name << " defaulted");
							// Create a default texture.
                            LoadModelFileTexture( model, name.toUtf8().data(), NULL, 0, materialParms );
						}
						glTextures.append( model.Textures[i].texid );

                        const VString usage = texture.value( "usage" ).toString();
						if ( usage == "diffuse" )
						{
							if ( materialParms.EnableDiffuseAniso == true )
							{
								MakeTextureAniso( model.Textures[i].texid, 2.0f );
							}
						}
						else if ( usage == "emissive" )
						{
							if ( materialParms.EnableEmissiveLodClamp == true )
							{
								// LOD clamp lightmap textures to avoid light bleeding
								MakeTextureLodClamped( model.Textures[i].texid, 1 );
							}
						}
						/*
						const String occlusion = texture.GetChildStringByName( "occlusion" );

						TextureOcclusion textureOcclusion = TEXTURE_OCCLUSION_OPAQUE;
						if ( occlusion == "opaque" )			{ textureOcclusion = TEXTURE_OCCLUSION_OPAQUE; }
						else if ( occlusion == "perforated" )	{ textureOcclusion = TEXTURE_OCCLUSION_PERFORATED; }
						else if ( occlusion == "transparent" )	{ textureOcclusion = TEXTURE_OCCLUSION_TRANSPARENT; }
						*/
					}
				}
			}

			//
			// Render Model Joints
			//

			const VJson &joint_array( render_model.value( "joints" ) );
			if ( joint_array.isArray() )
			{
				model.Joints.clear();

                const VJsonArray &joints = joint_array.toArray();
				for ( const VJson &joint : joints ) {
					if ( joint.isObject() )
					{
						const uint index = model.Joints.allocBack();
						model.Joints[index].index = index;
                        model.Joints[index].name = joint.value( "name" ).toString();
                        StringUtils::StringTo( model.Joints[index].transform, joint.value( "transform" ).toStdString().c_str() );
						model.Joints[index].animation = MODEL_JOINT_ANIMATION_NONE;
                        const std::string animation = joint.value( "animation" ).toStdString();
						if ( animation == "none" )			{ model.Joints[index].animation = MODEL_JOINT_ANIMATION_NONE; }
						else if ( animation == "rotate" )	{ model.Joints[index].animation = MODEL_JOINT_ANIMATION_ROTATE; }
						else if ( animation == "sway" )		{ model.Joints[index].animation = MODEL_JOINT_ANIMATION_SWAY; }
						else if ( animation == "bob" )		{ model.Joints[index].animation = MODEL_JOINT_ANIMATION_BOB; }
						model.Joints[index].parameters.x = joint.value( "parmX" ).toDouble();
						model.Joints[index].parameters.y = joint.value( "parmY" ).toDouble();
						model.Joints[index].parameters.z = joint.value( "parmZ" ).toDouble();
						model.Joints[index].timeOffset = joint.value( "timeOffset" ).toDouble();
						model.Joints[index].timeScale = joint.value( "timeScale" ).toDouble();
					}
				}
			}

			//
			// Render Model Tags
			//

			const VJson tag_array( render_model.value( "tags" ) );
			if ( tag_array.isArray() )
			{
				model.Tags.clear();

                const VJsonArray &tags = tag_array.toArray();
				for ( const VJson &tag : tags ) {
					if ( tag.isObject() )
					{
						const uint index = model.Tags.allocBack();
                        model.Tags[index].name = tag.value( "name" ).toString();
                        StringUtils::StringTo( model.Tags[index].matrix, 		tag.value( "matrix" ).toStdString().c_str() );
                        StringUtils::StringTo( model.Tags[index].jointIndices, 	tag.value( "jointIndices" ).toStdString().c_str() );
                        StringUtils::StringTo( model.Tags[index].jointWeights, 	tag.value( "jointWeights" ).toStdString().c_str() );
					}
				}
			}

			//
			// Render Model Surfaces
			//

			const VJson surface_array( render_model.value( "surfaces" ) );
			if ( surface_array.isArray() )
			{
                const VJsonArray &surfaces = surface_array.toArray();
				for (const VJson &surface : surfaces) {
					if ( surface.isObject() )
					{
						const uint index = model.Def.surfaces.allocBack();

						//
						// Source Meshes
						//

						const VJson &source(surface.value( "source" ));
						if (source.isArray()) {
                            const VJsonArray &elements = source.toArray();
							for (const VJson &e : elements) {
                                if ( model.Def.surfaces[index].surfaceName.length() ) {
									model.Def.surfaces[index].surfaceName += ";";
								}
                                model.Def.surfaces[index].surfaceName += e.toString();
							}
						}

						LOGV( "surface %s", model.Def.surfaces[index].surfaceName.toCString() );

						//
						// Surface Material
						//

						enum
						{
							MATERIAL_TYPE_OPAQUE,
							MATERIAL_TYPE_PERFORATED,
							MATERIAL_TYPE_TRANSPARENT,
							MATERIAL_TYPE_ADDITIVE
						} materialType = MATERIAL_TYPE_OPAQUE;

						int diffuseTextureIndex = -1;
						int normalTextureIndex = -1;
						int specularTextureIndex = -1;
						int emissiveTextureIndex = -1;
						int reflectionTextureIndex = -1;

						const VJson &material( surface.value( "material" ) );
						if ( material.isObject() )
						{
                            const VString type = material.value( "type" ).toString();

							if ( type == "opaque" )				{ materialType = MATERIAL_TYPE_OPAQUE; }
							else if ( type == "perforated" )	{ materialType = MATERIAL_TYPE_PERFORATED; }
							else if ( type == "transparent" )	{ materialType = MATERIAL_TYPE_TRANSPARENT; }
							else if ( type == "additive" )		{ materialType = MATERIAL_TYPE_ADDITIVE; }

							diffuseTextureIndex		= material.value( "diffuse", -1 ).toInt();
							normalTextureIndex		= material.value( "normal", -1 ).toInt();
							specularTextureIndex	= material.value( "specular", -1 ).toInt();
							emissiveTextureIndex	= material.value( "emissive", -1 ).toInt();
							reflectionTextureIndex	= material.value( "reflection", -1 ).toInt();
						}

						//
						// Surface Bounds
						//

                        StringUtils::StringTo( model.Def.surfaces[index].cullingBounds, surface.value("bounds").toStdString().c_str() );

						//
						// Vertices
						//

						VertexAttribs attribs;

						const VJson &vertices( surface.value( "vertices" ) );
						if ( vertices.isObject() )
						{
                            const int vertexCount = std::min( vertices.value( "vertexCount" ).toInt(), 1024*1024 );
                            vInfo(vertexCount << "vertices");

                            ReadModelArray( attribs.position,     vertices.value( "position" ).toStdString().c_str(),		modelsBin, vertexCount );
                            ReadModelArray( attribs.normal,       vertices.value( "normal" ).toStdString().c_str(),		modelsBin, vertexCount );
                            ReadModelArray( attribs.tangent,      vertices.value( "tangent" ).toStdString().c_str(),		modelsBin, vertexCount );
                            ReadModelArray( attribs.binormal,     vertices.value( "binormal" ).toStdString().c_str(),		modelsBin, vertexCount );
                            ReadModelArray( attribs.color,        vertices.value( "color" ).toStdString().c_str(),			modelsBin, vertexCount );
                            ReadModelArray( attribs.uvCoordinate0,          vertices.value( "uv0" ).toStdString().c_str(),			modelsBin, vertexCount );
                            ReadModelArray( attribs.uvCoordinate1,          vertices.value( "uv1" ).toStdString().c_str(),			modelsBin, vertexCount );
                            ReadModelArray( attribs.motionIndices, vertices.value( "jointIndices" ).toStdString().c_str(),	modelsBin, vertexCount );
                            ReadModelArray( attribs.motionWeight, vertices.value( "jointWeights" ).toStdString().c_str(),	modelsBin, vertexCount );
						}

						//
						// Triangles
						//

                        VArray< ushort > indices;

						const VJson &triangles( surface.value( "triangles" ) );
						if ( triangles.isObject() )
						{
                            const int indexCount = std::min( triangles.value( "indexCount" ).toInt(), 1024 * 1024 * 1024 );
                            vInfo(indexCount << "indices");

                            ReadModelArray( indices, triangles.value( "indices" ).toStdString().c_str(), modelsBin, indexCount );
						}

						//
						// Setup geometry, textures and render programs now that the vertex attributes are known.
						//

						model.Def.surfaces[index].geo.createGlGeometry( attribs, indices );

						const char * materialTypeString = "opaque";
						NV_UNUSED( materialTypeString );	// we'll get warnings if the LOGV's compile out

						// set up additional material flags for the surface
						if ( materialType == MATERIAL_TYPE_PERFORATED )
						{
							// Just blend because alpha testing is rather expensive.
							model.Def.surfaces[index].materialDef.gpuState.blendEnable = true;
							model.Def.surfaces[index].materialDef.gpuState.depthMaskEnable = false;
							model.Def.surfaces[index].materialDef.gpuState.blendSrc = GL_SRC_ALPHA;
							model.Def.surfaces[index].materialDef.gpuState.blendDst = GL_ONE_MINUS_SRC_ALPHA;
							materialTypeString = "perforated";
						}
						else if ( materialType == MATERIAL_TYPE_TRANSPARENT || materialParms.Transparent )
						{
							model.Def.surfaces[index].materialDef.gpuState.blendEnable = true;
							model.Def.surfaces[index].materialDef.gpuState.depthMaskEnable = false;
							model.Def.surfaces[index].materialDef.gpuState.blendSrc = GL_SRC_ALPHA;
							model.Def.surfaces[index].materialDef.gpuState.blendDst = GL_ONE_MINUS_SRC_ALPHA;
							materialTypeString = "transparent";
						}
						else if ( materialType == MATERIAL_TYPE_ADDITIVE )
						{
							model.Def.surfaces[index].materialDef.gpuState.blendEnable = true;
							model.Def.surfaces[index].materialDef.gpuState.depthMaskEnable = false;
							model.Def.surfaces[index].materialDef.gpuState.blendSrc = GL_ONE;
							model.Def.surfaces[index].materialDef.gpuState.blendDst = GL_ONE;
							materialTypeString = "additive";
						}

                        const bool skinned = (	attribs.motionIndices.size() == attribs.position.size() &&
                                                attribs.motionWeight.size() == attribs.position.size() );

						if ( diffuseTextureIndex >= 0 && diffuseTextureIndex < glTextures.length() )
						{
							model.Def.surfaces[index].materialDef.textures[0] = glTextures[diffuseTextureIndex];

							if ( emissiveTextureIndex >= 0 && emissiveTextureIndex < glTextures.length() )
							{
								model.Def.surfaces[index].materialDef.textures[1] = glTextures[emissiveTextureIndex];

								if (	normalTextureIndex >= 0 && normalTextureIndex < glTextures.length() &&
										specularTextureIndex >= 0 && specularTextureIndex < glTextures.length() &&
										reflectionTextureIndex >= 0 && reflectionTextureIndex < glTextures.length() )
								{
									// reflection mapped material;
									model.Def.surfaces[index].materialDef.textures[2] = glTextures[normalTextureIndex];
									model.Def.surfaces[index].materialDef.textures[3] = glTextures[specularTextureIndex];
									model.Def.surfaces[index].materialDef.textures[4] = glTextures[reflectionTextureIndex];

									model.Def.surfaces[index].materialDef.numTextures = 5;
									if ( skinned )
									{
										if ( programs.ProgSkinnedReflectionMapped == NULL )
										{
											vFatal("No ProgSkinnedReflectionMapped set");
										}
										model.Def.surfaces[index].materialDef.programObject = programs.ProgSkinnedReflectionMapped->program;
										model.Def.surfaces[index].materialDef.uniformMvp = programs.ProgSkinnedReflectionMapped->uniformModelViewProMatrix;
										model.Def.surfaces[index].materialDef.uniformModel = programs.ProgSkinnedReflectionMapped->uniformModelMatrix;
										model.Def.surfaces[index].materialDef.uniformView = programs.ProgSkinnedReflectionMapped->uniformViewMatrix;
										model.Def.surfaces[index].materialDef.uniformJoints = programs.ProgSkinnedReflectionMapped->uniformJoints;
										LOGV( "%s skinned reflection mapped material", materialTypeString );
									}
									else
									{
										if ( programs.ProgReflectionMapped == NULL )
										{
											vFatal("No ProgReflectionMapped set");
										}
										model.Def.surfaces[index].materialDef.programObject = programs.ProgReflectionMapped->program;
										model.Def.surfaces[index].materialDef.uniformMvp = programs.ProgReflectionMapped->uniformModelViewProMatrix;
										model.Def.surfaces[index].materialDef.uniformModel = programs.ProgReflectionMapped->uniformModelMatrix;
										model.Def.surfaces[index].materialDef.uniformView = programs.ProgReflectionMapped->uniformViewMatrix;
										LOGV( "%s reflection mapped material", materialTypeString );
									}
								}
								else
								{
									// light mapped material
									model.Def.surfaces[index].materialDef.numTextures = 2;
									if ( skinned )
									{
										if ( programs.ProgSkinnedLightMapped == NULL )
										{
											vFatal("No ProgSkinnedLightMapped set");
										}
										model.Def.surfaces[index].materialDef.programObject = programs.ProgSkinnedLightMapped->program;
										model.Def.surfaces[index].materialDef.uniformMvp = programs.ProgSkinnedLightMapped->uniformModelViewProMatrix;
										model.Def.surfaces[index].materialDef.uniformJoints = programs.ProgSkinnedLightMapped->uniformJoints;
										LOGV( "%s skinned light mapped material", materialTypeString );
									}
									else
									{
										if ( programs.ProgLightMapped == NULL )
										{
											vFatal("No ProgLightMapped set");
										}
										model.Def.surfaces[index].materialDef.programObject = programs.ProgLightMapped->program;
										model.Def.surfaces[index].materialDef.uniformMvp = programs.ProgLightMapped->uniformModelViewProMatrix;
										LOGV( "%s light mapped material", materialTypeString );
									}
								}
							}
							else
							{
								// diffuse only material
								model.Def.surfaces[index].materialDef.numTextures = 1;
								if ( skinned )
								{
									if ( programs.ProgSkinnedSingleTexture == NULL )
									{
										vFatal("No ProgSkinnedSingleTexture set");
									}
									model.Def.surfaces[index].materialDef.programObject = programs.ProgSkinnedSingleTexture->program;
									model.Def.surfaces[index].materialDef.uniformMvp = programs.ProgSkinnedSingleTexture->uniformModelViewProMatrix;
									model.Def.surfaces[index].materialDef.uniformJoints = programs.ProgSkinnedSingleTexture->uniformJoints;
									LOGV( "%s skinned diffuse only material", materialTypeString );
								}
								else
								{
									if ( programs.ProgSingleTexture == NULL )
									{
										vFatal("No ProgSingleTexture set");
									}
									model.Def.surfaces[index].materialDef.programObject = programs.ProgSingleTexture->program;
									model.Def.surfaces[index].materialDef.uniformMvp = programs.ProgSingleTexture->uniformModelViewProMatrix;
									LOGV( "%s diffuse only material", materialTypeString );
								}
							}
						}
						else if ( attribs.color.length() > 0 )
						{
							// vertex color material
							model.Def.surfaces[index].materialDef.numTextures = 0;
							if ( skinned )
							{
								if ( programs.ProgSkinnedVertexColor == NULL )
								{
									vFatal("No ProgSkinnedVertexColor set");
								}
								model.Def.surfaces[index].materialDef.programObject = programs.ProgSkinnedVertexColor->program;
								model.Def.surfaces[index].materialDef.uniformMvp = programs.ProgSkinnedVertexColor->uniformModelViewProMatrix;
								LOGV( "%s skinned vertex color material", materialTypeString );
							}
							else
							{
								if ( programs.ProgVertexColor == NULL )
								{
									vFatal("No ProgVertexColor set");
								}
								model.Def.surfaces[index].materialDef.programObject = programs.ProgVertexColor->program;
								model.Def.surfaces[index].materialDef.uniformMvp = programs.ProgVertexColor->uniformModelViewProMatrix;
								LOGV( "%s vertex color material", materialTypeString );
							}
						}
						else
						{
							// surface without texture or vertex colors
							model.Def.surfaces[index].materialDef.textures[0] = 0;
							model.Def.surfaces[index].materialDef.numTextures = 1;
							if ( skinned )
							{
								if ( programs.ProgSkinnedSingleTexture == NULL )
								{
									vFatal("No ProgSkinnedSingleTexture set");
								}
								model.Def.surfaces[index].materialDef.programObject = programs.ProgSkinnedSingleTexture->program;
								model.Def.surfaces[index].materialDef.uniformMvp = programs.ProgSingleTexture->uniformModelViewProMatrix;
								LOGV( "%s skinned default texture material", materialTypeString );
							}
							else
							{
								if ( programs.ProgSingleTexture == NULL )
								{
									vFatal("No ProgSingleTexture set");
								}
								model.Def.surfaces[index].materialDef.programObject = programs.ProgSingleTexture->program;
								model.Def.surfaces[index].materialDef.uniformMvp = programs.ProgSingleTexture->uniformModelViewProMatrix;
								LOGV( "%s default texture material", materialTypeString );
							}
						}

						if ( materialParms.PolygonOffset )
						{
							model.Def.surfaces[index].materialDef.gpuState.polygonOffsetEnable = true;
							LOGV( "polygon offset material" );
						}
					}
				}
			}
		}

		//
		// Collision Model
		//

		const VJson &collision_model( models.value( "collision_model" ) );
		if ( collision_model.isArray() )
		{
			LOGV( "loading collision model.." );

            const VJsonArray &polytopes = collision_model.toArray();
			for (const VJson &polytope : polytopes) {
				const uint index = model.Collisions.Polytopes.allocBack();

				if ( polytope.isObject() )
				{
                    model.Collisions.Polytopes[index].Name = polytope.value( "name" ).toString();
                    StringUtils::StringTo( model.Collisions.Polytopes[index].Planes, polytope.value( "planes" ).toStdString().c_str() );
				}
			}
		}

		//
		// Ground Collision Model
		//

		const VJson &ground_collision_model( models.value( "ground_collision_model" ) );
		if ( ground_collision_model.isArray() )
		{
			LOGV( "loading ground collision model.." );

            const VJsonArray &polytopes = ground_collision_model.toArray();
			for (const VJson &polytope : polytopes) {
				const uint index = model.GroundCollisions.Polytopes.allocBack();

				if ( polytope.isObject() )
				{
                    model.GroundCollisions.Polytopes[index].Name = polytope.value( "name" ).toString();
                    StringUtils::StringTo( model.GroundCollisions.Polytopes[index].Planes, polytope.value( "planes" ).toStdString().c_str() );
				}
			}
		}

		//
		// Ray-Trace Model
		//

		const VJson &raytrace_model( models.value( "raytrace_model" ) );
		if ( raytrace_model.isObject() )
		{
			LOGV( "loading ray-trace model.." );

			ModelTrace &traceModel = model.TraceModel;

			traceModel.header.numVertices	= raytrace_model.value( "numVertices" ).toInt();
			traceModel.header.numUvs		= raytrace_model.value( "numUvs" ).toInt();
			traceModel.header.numIndices	= raytrace_model.value( "numIndices" ).toInt();
			traceModel.header.numNodes		= raytrace_model.value( "numNodes" ).toInt();
			traceModel.header.numLeafs		= raytrace_model.value( "numLeafs" ).toInt();
			traceModel.header.numOverflow	= raytrace_model.value( "numOverflow" ).toInt();

            StringUtils::StringTo( traceModel.header.bounds, raytrace_model.value( "bounds" ).toStdString().c_str() );

            ReadModelArray( traceModel.vertices, raytrace_model.value( "vertices" ).toStdString().c_str(), modelsBin, traceModel.header.numVertices );
            ReadModelArray( traceModel.uvs, raytrace_model.value( "uvs" ).toStdString().c_str(), modelsBin, traceModel.header.numUvs );
            ReadModelArray( traceModel.indices, raytrace_model.value( "indices" ).toStdString().c_str(), modelsBin, traceModel.header.numIndices );

            if (!modelsBin.read(traceModel.nodes, traceModel.header.numNodes)) {
				const VJson &nodes_array( raytrace_model.value( "nodes" ) );
				if ( nodes_array.isArray() )
				{
                    const VJsonArray &nodes = nodes_array.toArray();
					for (const VJson &node : nodes) {
						const uint index = traceModel.nodes.allocBack();

						if ( node.isObject() )
						{
							traceModel.nodes[index].data = (vuint32) node.value( "data" ).toDouble();
							traceModel.nodes[index].dist = node.value( "dist" ).toDouble();
						}
					}
				}
			}

            if (!modelsBin.read(traceModel.leafs, traceModel.header.numLeafs)) {
				const VJson &leafs_array( raytrace_model.value( "leafs" ) );
				if ( leafs_array.isArray() )
				{
                    const VJsonArray &leafs = leafs_array.toArray();
					for (const VJson &leaf : leafs) {
						const uint index = traceModel.leafs.allocBack();

						if ( leaf.isObject() )
						{
                            StringUtils::StringTo( traceModel.leafs[index].triangles, RT_KDTREE_MAX_LEAF_TRIANGLES, leaf.value( "triangles" ).toStdString().c_str() );
                            StringUtils::StringTo( traceModel.leafs[index].ropes, 6, leaf.value( "ropes" ).toStdString().c_str() );
                            StringUtils::StringTo( traceModel.leafs[index].bounds, leaf.value( "bounds" ).toStdString().c_str() );
						}
					}
				}
			}

            ReadModelArray( traceModel.overflow, raytrace_model.value( "overflow" ).toStdString().c_str(), modelsBin, traceModel.header.numOverflow );
		}
	}

    if (!modelsBin.atEnd()) {
		vWarn("failed to properly read binary file");
	}
}

static ModelFile * LoadModelFile( unzFile zfp, const char * fileName,
								const char * fileData, const int fileDataLength,
								const ModelGlPrograms & programs,
								const MaterialParms & materialParms )
{


	ModelFile * modelPtr = new ModelFile;
	ModelFile & model = *modelPtr;

	model.FileName = fileName;
	model.UsingSrgbTextures = materialParms.UseSrgbTextureFormats;

	if ( !zfp )
	{
		vWarn("Error: can't load " << fileName);
		return modelPtr;
	}

	// load all texture files and locate the model files

    std::stringstream modelsJson;
    VBuffer modelsBin;

	for ( int ret = unzGoToFirstFile( zfp ); ret == UNZ_OK; ret = unzGoToNextFile( zfp ) )
	{
		unz_file_info finfo;
		char entryName[256];
		unzGetCurrentFileInfo( zfp, &finfo, entryName, sizeof( entryName ), NULL, 0, NULL, 0 );
		LOGV( "zip level: %ld, file: %s", finfo.compression_method, entryName );

		if ( unzOpenCurrentFile( zfp ) != UNZ_OK )
		{
			vWarn("Failed to open " << entryName << " from " << fileName);
			continue;
		}

		const int size = finfo.uncompressed_size;
		char * buffer = NULL;

		if ( finfo.compression_method == 0 && fileData != NULL )
		{
			buffer = (char *)fileData + unzGetCurrentFileZStreamPos64( zfp );
		}
		else
		{
			buffer = new char[size + 1];
			buffer[size] = '\0';	// always zero terminate text files

			if ( unzReadCurrentFile( zfp, buffer, size ) != size )
			{
				vWarn("Failed to read " << entryName << " from " << fileName);
				delete [] buffer;
				continue;
			}
		}

		// assume a 3 character extension
		const size_t entryLength = strlen( entryName );
		const char * extension = ( entryLength >= 4 ) ? &entryName[entryLength - 4] : entryName;

		if ( strcasecmp( entryName, "models.json" ) == 0 )
		{
			// save this for parsing
            modelsJson.write(buffer, size);
		}
		else if ( strcasecmp( entryName, "models.bin" ) == 0 )
		{
			// save this for parsing
            vint64 count = 0;
            do {
                count += modelsBin.write(buffer + count, size - count);
            } while (count < size);
		}
		else if (	strcasecmp( extension, ".pvr" ) == 0 ||
					strcasecmp( extension, ".ktx" ) == 0 )
		{
			// only support .pvr and .ktx containers for now
			LoadModelFileTexture( model, entryName, buffer, size, materialParms );
		}
		else
		{
			// ignore other files
			vInfo("Ignoring " << entryName);
		}

		unzCloseCurrentFile( zfp );
	}
	unzClose( zfp );

    VBinaryStream bin(&modelsBin);
    LoadModelFileJson(model, modelsJson, bin, programs, materialParms);

	return modelPtr;
}

#if MEMORY_MAPPED

// Memory-mapped wrapper for ZLIB/MiniZip

struct zlib_mmap_opaque
{
	MappedFile		file;
	MappedView		view;
    const uchar *	data;
    const uchar *	ptr;
	int				len;
	int				left;
};

static voidpf ZCALLBACK mmap_fopen_file_func( voidpf opaque, const char *, int )
{
	return opaque;
}

static uLong ZCALLBACK mmap_fread_file_func( voidpf opaque, voidpf, void * buf, uLong size )
{
	zlib_mmap_opaque *state = (zlib_mmap_opaque *)opaque;

	if ( (int)size <= 0 || state->left < (int)size )
	{
		return 0;
	}

	memcpy( buf, state->ptr, size );
	state->ptr += size;
	state->left -= size;

	return size;
}

static uLong ZCALLBACK mmap_fwrite_file_func( voidpf, voidpf, const void *, uLong )
{
	return 0;
}

static long ZCALLBACK mmap_ftell_file_func( voidpf opaque, voidpf )
{
	zlib_mmap_opaque *state = (zlib_mmap_opaque *)opaque;

	return state->len - state->left;
}

static long ZCALLBACK mmap_fseek_file_func( voidpf opaque, voidpf, uLong offset, int origin )
{
	zlib_mmap_opaque *state = (zlib_mmap_opaque *)opaque;

	switch ( origin ) {
		case SEEK_SET:
			if ( (int)offset < 0 || (int)offset > state->len )
			{
				return 0;
			}
			state->ptr = state->data + offset;
			state->left = state->len - offset;
			break;
		case SEEK_CUR:
			if ( (int)offset < 0 || (int)offset > state->left )
			{
				return 0;
			}
			state->ptr += offset;
			state->left -= offset;
			break;
		case SEEK_END:
			state->ptr = state->data + state->len;
			state->left = 0;
			break;
	}

	return 0;
}

static int ZCALLBACK mmap_fclose_file_func( voidpf, voidpf )
{
	return 0;
}

static int ZCALLBACK mmap_ferror_file_func( voidpf, voidpf )
{
	return 0;
}

static void mem_set_opaque( zlib_mmap_opaque & opaque, const unsigned char * data, int len )
{
	opaque.data = data;
	opaque.len = len;
	opaque.ptr = data;
	opaque.left = len;
}

static bool mmap_open_opaque( const char * fileName, zlib_mmap_opaque & opaque )
{
	// If unable to open the ZIP file,
	if ( !opaque.file.openRead( fileName, true, true ) )
	{
		vWarn("Couldn't open " << fileName);
		return false;
	}

	int len = (int)opaque.file.length();
	if ( len <= 0 )
	{
		vWarn("len = " << len);
		return false;
	}
	if ( !opaque.view.open( &opaque.file ) )
	{
		vWarn("View open failed");
		return false;
	}
	if ( !opaque.view.mapView( 0, len ) )
	{
		vWarn("MapView failed");
		return false;
	}

	opaque.data = opaque.view.front();
	opaque.len = len;
	opaque.ptr = opaque.data;
	opaque.left = len;

	return true;
}

static unzFile open_opaque( zlib_mmap_opaque & zlib_opaque, const char * fileName )
{
	zlib_filefunc_def zlib_file_funcs =
	{
		mmap_fopen_file_func,
		mmap_fread_file_func,
		mmap_fwrite_file_func,
		mmap_ftell_file_func,
		mmap_fseek_file_func,
		mmap_fclose_file_func,
		mmap_ferror_file_func,
		&zlib_opaque
	};

	return unzOpen2( fileName, &zlib_file_funcs );
}

ModelFile * LoadModelFileFromMemory( const char * fileName,
		const void * buffer, int bufferLength,
		const ModelGlPrograms & programs,
		const MaterialParms & materialParms )
{
	// Open the .ModelFile file as a zip.
    vInfo("LoadModelFileFromMemory " << fileName << bufferLength);

	zlib_mmap_opaque zlib_opaque;

	mem_set_opaque( zlib_opaque, (const unsigned char *)buffer, bufferLength );

	unzFile zfp = open_opaque( zlib_opaque, fileName );
	if ( !zfp )
	{
		return new ModelFile( fileName );
	}

	vInfo("LoadModelFileFromMemory zfp = " << zfp);

	return LoadModelFile( zfp, fileName, (char *)buffer, bufferLength, programs, materialParms );
}

ModelFile * LoadModelFile( const char * fileName,
		const ModelGlPrograms & programs,
		const MaterialParms & materialParms )
{
	zlib_mmap_opaque zlib_opaque;

	vInfo("LoadModelFile " << fileName);

	// Map and open the zip file
	if ( !mmap_open_opaque( fileName, zlib_opaque ) )
	{
		return new ModelFile( fileName );
	}

	unzFile zfp = open_opaque( zlib_opaque, fileName );
	if ( !zfp )
	{
		return new ModelFile( fileName );
	}

	return LoadModelFile( zfp, fileName, (char *)zlib_opaque.data, zlib_opaque.len, programs, materialParms );
}

#else	// !MEMORY_MAPPED

struct mzBuffer_t
{
	unsigned char * mzBufferBase;
	int				mzBufferLength;
	int				mzBufferPos;
};

static voidpf mz_open_file_func( voidpf opaque, const char* filename, int mode )
{
	mzBuffer_t * buffer = (mzBuffer_t *)opaque;
	buffer->mzBufferPos = 0;
	return (void *)1;
}

static uLong mz_read_file_func( voidpf opaque, voidpf stream, void* buf, uLong size )
{
	mzBuffer_t * buffer = (mzBuffer_t *)opaque;
	const int numCopyBytes = NervGear::VAlgorithm::Min( buffer->mzBufferLength - buffer->mzBufferPos, (int)size );
	memcpy( buf, buffer->mzBufferBase + buffer->mzBufferPos, numCopyBytes );
	buffer->mzBufferPos += numCopyBytes;
	return numCopyBytes;
}

static uLong mz_write_file_func( voidpf opaque, voidpf stream, const void* buf, uLong size )
{
	return -1;
}

static int mz_close_file_func( voidpf opaque, voidpf stream )
{
	return 0;
}

static int mz_testerror_file_func( voidpf opaque, voidpf stream )
{
    int ret = -1;
    if ( stream != NULL )
    {
        ret = 0;
    }
    return ret;
}

static long mz_tell_file_func( voidpf opaque, voidpf stream )
{
	mzBuffer_t * buffer = (mzBuffer_t *)opaque;
	return buffer->mzBufferLength;
}

static long mz_seek_file_func( voidpf opaque, voidpf stream, uLong offset, int origin )
{
	mzBuffer_t * buffer = (mzBuffer_t *)opaque;
	switch ( origin )
	{
		case ZLIB_FILEFUNC_SEEK_CUR :
			buffer->mzBufferPos = NervGear::VAlgorithm::Min( buffer->mzBufferLength, buffer->mzBufferPos + (int)offset );
			break;
		case ZLIB_FILEFUNC_SEEK_END :
			buffer->mzBufferPos = NervGear::VAlgorithm::Min( buffer->mzBufferLength, buffer->mzBufferLength - (int)offset );
			break;
		case ZLIB_FILEFUNC_SEEK_SET :
			buffer->mzBufferPos = NervGear::VAlgorithm::Min( buffer->mzBufferLength, (int)offset );
			break;
		default:
			return -1;
	}
	return 0;
}

ModelFile * LoadModelFileFromMemory( const char * fileName,
		const void * buffer, int bufferLength,
		const ModelGlPrograms & programs,
		const MaterialParms & materialParms )
{
	// Open the .ModelFile file as a zip.
    vInfo("LoadModelFileFromMemory " << fileName << bufferLength);

	mzBuffer_t mzBuffer;
	mzBuffer.mzBufferBase = (unsigned char *)buffer;
	mzBuffer.mzBufferLength = bufferLength;
	mzBuffer.mzBufferPos = 0;

	zlib_filefunc_def filefunc;
	filefunc.zopen_file = mz_open_file_func;
	filefunc.zread_file = mz_read_file_func;
	filefunc.zwrite_file = mz_write_file_func;
	filefunc.ztell_file = mz_tell_file_func;
	filefunc.zseek_file = mz_seek_file_func;
	filefunc.zclose_file = mz_close_file_func;
	filefunc.zerror_file = mz_testerror_file_func;
	filefunc.opaque = &mzBuffer;

	unzFile zfp = unzOpen2( fileName, &filefunc );

	vInfo("OpenZipForMemory = " << zfp);

	return LoadModelFile( zfp, fileName, NULL, 0, programs, materialParms );
}

ModelFile * LoadModelFile( const char * fileName,
		const ModelGlPrograms & programs, const MaterialParms & materialParms )
{
	// Open the .ModelFile file as a zip.
	unzFile zfp = unzOpen( fileName );
	return LoadModelFile( zfp, fileName, NULL, 0, programs, materialParms );
}

#endif	// MEMORY_MAPPED

NV_NAMESPACE_END
