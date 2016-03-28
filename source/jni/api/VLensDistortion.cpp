#include "sensor/Profile.h"
#include "sensor/Device.h"
#include "VDevice.h"
#include "VAlgorithm.h"
#include "VGlShader.h"
#include "VGlOperation.h"

#include "VLensDistortion.h"


NV_NAMESPACE_BEGIN

static float EvalCatmullRomSpline ( float const *K, float scaledVal, int NumSegments )
{
    //int const NumSegments = LensConfig::NumCoefficients;

    float scaledValFloor = floorf ( scaledVal );
    scaledValFloor = std::max ( 0.0f, std::min ( (float)(NumSegments-1), scaledValFloor ) );
    float t = scaledVal - scaledValFloor;
    int k = (int)scaledValFloor;

    float p0 = 0.0f;
    float p1 = 0.0f;
    float m0 = 0.0f;
    float m1 = 0.0f;

    if (k == 0)
    {   // Curve starts at 1.0 with gradient K[1]-K[0]
        p0 = 1.0f;
        m0 = K[1] - K[0];
        p1 = K[1];
        m1 = 0.5f * ( K[2] - K[0] );
    }
    else if (k < NumSegments-2)
    {   // General case
        p0 = K[k];
        m0 = 0.5f * ( K[k+1] - K[k-1] );
        p1 = K[k+1];
        m1 = 0.5f * ( K[k+2] - K[k] );
    }
    else if (k == NumSegments-2)
    {   // Last tangent is just the slope of the last two points.
        p0 = K[k];
        m0 = 0.5f * ( K[k+1] - K[k-1] );
        p1 = K[k+1];
        m1 = K[k+1] - K[k];
    }
    else if (k == NumSegments-1)
    {   // Beyond the last segment it's just a straight line
        p0 = K[k];
        m0 = K[k] - K[k-1];
        p1 = p0 + m0;
        m1 = m0;
    }

    float omt = 1.0f - t;
    float res  = ( p0 * ( 1.0f + 2.0f *   t ) + m0 *   t ) * omt * omt
                 + ( p1 * ( 1.0f + 2.0f * omt ) - m1 * omt ) *   t *   t;

    return res;
}

// The result is a scaling applied to the distance.
static float DistortionFnScaleRadiusSquared(const VLensDistortion& lens,float rsq)
{
    float scale = 1.0f;
    switch ( lens.Eqn )
    {
        case Distortion_Poly4:
            // This version is deprecated! Prefer one of the other two.
            scale = ( lens.K[0] + rsq * ( lens.K[1] + rsq * ( lens.K[2] + rsq * lens.K[3] ) ) );
            break;
        case Distortion_RecipPoly4:
            scale = 1.0f / ( lens.K[0] + rsq * ( lens.K[1] + rsq * ( lens.K[2] + rsq * lens.K[3] ) ) );
            break;
        case Distortion_CatmullRom10: {
            // A Catmull-Rom spline through the values 1.0, K[1], K[2] ... K[10]
            // evenly spaced in R^2 from 0.0 to MaxR^2
            // K[0] controls the slope at radius=0.0, rather than the actual value.
            const int NumSegments = 11;
            OVR_ASSERT ( NumSegments <= lens.MaxCoefficients );
            float scaledRsq = (float)(NumSegments-1) * rsq / ( lens.MaxR * lens.MaxR );
            scale = EvalCatmullRomSpline ( lens.K, scaledRsq, NumSegments );
        } break;

        case Distortion_CatmullRom20: {
            // A Catmull-Rom spline through the values 1.0, K[1], K[2] ... K[20]
            // evenly spaced in R^2 from 0.0 to MaxR^2
            // K[0] controls the slope at radius=0.0, rather than the actual value.
            const int NumSegments = 21;
            OVR_ASSERT ( NumSegments <= lens.MaxCoefficients );
            float scaledRsq = (float)(NumSegments-1) * rsq / ( lens.MaxR * lens.MaxR );
            scale = EvalCatmullRomSpline ( lens.K, scaledRsq, NumSegments );
        } break;

        default:
            OVR_ASSERT ( false );
            break;
    }
    return scale;
}
// x,y,z components map to r,g,b
static V3Vectf DistortionFnScaleRadiusSquaredChroma (const VLensDistortion& lens,float rsq)
{
    float scale = DistortionFnScaleRadiusSquared (lens, rsq );
    V3Vectf scaleRGB;
    scaleRGB.x = scale * ( 1.0f + lens.ChromaticAberration[0] + rsq * lens.ChromaticAberration[1] );     // Red
    scaleRGB.y = scale;                                                                        // Green
    scaleRGB.z = scale * ( 1.0f + lens.ChromaticAberration[2] + rsq * lens.ChromaticAberration[3] );     // Blue
    return scaleRGB;
}

static void WarpTexCoordChroma( const VDevice* device, const float in[2],
                                float red[2], float green[2], float blue[2] ) {
    float theta[2];
    for ( int i = 0; i < 2; i++ ) {
        const float unit = in[i];
        const float ndc = 2.0f * ( unit - 0.5f );
        const float pixels = ndc * device->heightPixels * 0.5f;
        const float meters = pixels * device->widthMeters / device->widthPixels;
        const float tanAngle = meters / device->lens.MetersPerTanAngleAtCenter;
        theta[i] = tanAngle;
    }

    const float rsq = theta[0] * theta[0] + theta[1] * theta[1];

    const V3Vectf chromaScale = DistortionFnScaleRadiusSquaredChroma (device->lens,rsq);

    for ( int i = 0; i < 2; i++ ) {
        red[i] = chromaScale[0] * theta[i];
        green[i] = chromaScale[1] * theta[i];
        blue[i] = chromaScale[2] * theta[i];
    }
}

// Assume the cursor is a small region around the center of projection,
// extended horizontally to allow it to be at different 3D depths.
// While this will be around the center of each screen half on Note 4,
// smaller phone screens will be offset, and an automated distortion
// calibration might also not be completely symmetric.
//
// There is a hazard here, because the area up to the next vertex
// past this will be drawn, so the density of tesselation effects
// the area that the cursor will show in.
static bool VectorHitsCursor( const V2Vectf & v )
{
    if ( fabs( v.y ) > 0.017f ) // +/- 1 degree vertically
    {
        return false;
    }
    if ( fabs( v.x ) > 0.070f ) // +/- 4 degree horizontally
    {
        return false;
    }
    return true;
}

static void* CreateVertexBuffer( const VDevice* device,
                                 int tessellationsX, int tessellationsY)
{
    const int vertexCount = 2 * ( tessellationsX + 1 ) * ( tessellationsY + 1 );
    void*	buf = malloc(4 * vertexCount * 6 );

    // the centers are offset horizontal in each eye
    const float aspect = device->widthPixels * 0.5 / device->heightPixels;

    const float	horizontalShiftLeftMeters =  -(( device->lensSeparation / 2 ) - ( device->widthMeters / 4 )) + device->horizontalOffsetMeters;
    const float horizontalShiftRightMeters = (( device->lensSeparation / 2 ) - ( device->widthMeters / 4 )) + device->horizontalOffsetMeters;
    const float	horizontalShiftViewLeft = 2 * aspect * horizontalShiftLeftMeters / device->widthMeters;
    const float	horizontalShiftViewRight = 2 * aspect * horizontalShiftRightMeters / device->widthMeters;

    for ( int eye = 0; eye < 2; eye++ )
    {
        for ( int y = 0; y <= tessellationsY; y++ )
        {
            const float	yf = (float)y / (float)tessellationsY;
            for ( int x = 0; x <= tessellationsX; x++ )
            {
                int	vertNum = y * ( tessellationsX+1 ) * 2 +
                                 eye * (tessellationsX+1) + x;
                const float	xf = (float)x / (float)tessellationsX;
                float * v = &((float *)buf)[vertNum*6];
                const float inTex[2] = { ( eye ? horizontalShiftViewLeft : horizontalShiftViewRight ) +
                                         xf *aspect + (1.0f-aspect) * 0.5f, yf };
                WarpTexCoordChroma( device, inTex, &v[0], &v[2], &v[4] );
            }
        }
    }
    return buf;
}

int VLensDistortion::tessellationsX = 32;
int VLensDistortion::tessellationsY = 32;

VGlGeometry VLensDistortion::CreateTessellatedMesh(const VDevice* device,const int numSlicesPerEye, const float fovScale,
                                                   const bool cursorOnly)
{
    VGlOperation glOperation;
    VGlGeometry geometry;
    const int totalX = (tessellationsX+1)*2;

    if (tessellationsX < 1 || tessellationsY < 1 ) return geometry;

    const float * bufferVerts = &((float *)CreateVertexBuffer(device,tessellationsX,tessellationsY))[0];

    // Identify which verts would be inside the cursor plane
    bool * vertInCursor = NULL;
    if ( cursorOnly )
    {
        vertInCursor = new bool[totalX*(tessellationsY+1)];
        for ( int y = 0; y <= tessellationsY; y++ )
        {
            for ( int x = 0; x < totalX; x++ )
            {
                const int vertIndex = (y*totalX+x );
                V2Vectf	v;
                for ( int i = 0 ; i < 2; i++ )
                {
                    v[i] = fovScale * bufferVerts[vertIndex*6+i];
                }
                vertInCursor[ vertIndex ] = VectorHitsCursor( v );
            }
        }
    }

    // build a VertexArrayObject
    glOperation.glGenVertexArraysOES( 1, &geometry.vertexArrayObject );
    glOperation.glBindVertexArrayOES( geometry.vertexArrayObject );

    const int attribCount = 10;
    const int sliceTess = tessellationsX / numSlicesPerEye;

    const int vertexCount = 2*numSlicesPerEye*(sliceTess+1)*(tessellationsY+1);
    const int floatCount = vertexCount * attribCount;
    float * tessVertices = new float[floatCount];

    const int indexCount = 2*tessellationsX*tessellationsY*6;
    unsigned short * tessIndices = new unsigned short[indexCount];
    const int indexBytes = indexCount * sizeof( *tessIndices );

    int	index = 0;
    int	verts = 0;

    for ( int eye = 0; eye < 2; eye++ )
    {
        for ( int slice = 0; slice < numSlicesPerEye; slice++ )
        {
            const int vertBase = verts;

            for ( int y = 0; y <= tessellationsY; y++ )
            {
                const float	yf = (float)y / (float)tessellationsY;
                for ( int x = 0; x <= sliceTess; x++ )
                {
                    const int sx = slice * sliceTess + x;
                    const float	xf = (float)sx / (float)tessellationsX;
                    float * v = &tessVertices[attribCount * ( vertBase + y * (sliceTess+1) + x ) ];
                    v[0] = -1.0 + eye + xf;
                    v[1] = yf*2.0f - 1.0f;

                    // Copy the offsets from the file
                    for ( int i = 0 ; i < 6; i++ )
                    {
                        v[2+i] = fovScale * bufferVerts
                        [(y*(tessellationsX+1)*2+sx + eye * (tessellationsX+1))*6+i];
                    }

                    v[8] = (float)x / sliceTess;
                    // Enable this to allow fading at the edges.
                    // Samsung recommends not doing this, because it could cause
                    // visible differences in pixel wear on the screen over long
                    // periods of time.
                    if ( 0 && ( y == 0 || y == tessellationsY || sx == 0 || sx == tessellationsX ) )
                    {
                        v[9] = 0.0f;	// fade to black at edge
                    }
                    else
                    {
                        v[9] = 1.0f;
                    }
                }
            }
            verts += (tessellationsY+1)*(sliceTess+1);

            // The order of triangles doesn't matter for tiled rendering,
            // but when we can do direct rendering to the screen, we want the
            // order to follow the raster order to minimize the chance
            // of tear lines.
            //
            // This can be checked by quartering the number of indexes, and
            // making sure that the drawn pixels are the first pixels that
            // the raster will scan.
            for ( int x = 0; x < sliceTess; x++ )
            {
                for ( int y = 0; y < tessellationsY; y++ )
                {
                    if ( vertInCursor )
                    {	// skip this quad if none of the verts are in the cursor region
                        const int xx = x + eye * (tessellationsX+1) + slice * sliceTess;
                        if ( 0 ==
                             vertInCursor[ y * totalX + xx ]
                             + vertInCursor[ y * totalX + xx + 1 ]
                             + vertInCursor[ (y + 1) * totalX + xx ]
                             + vertInCursor[ (y + 1) * totalX + xx + 1 ] )
                        {
                            continue;
                        }
                    }

                    // flip the triangulation in opposite corners
                    if ( (slice*sliceTess+x <  tessellationsX/2) ^ (y < (tessellationsY/2)) )
                    {
                        tessIndices[index+0] = vertBase + y * (sliceTess+1) + x;
                        tessIndices[index+1] = vertBase + y * (sliceTess+1) + x + 1;
                        tessIndices[index+2] = vertBase + (y+1) * (sliceTess+1) + x + 1;

                        tessIndices[index+3] = vertBase + y * (sliceTess+1) + x;
                        tessIndices[index+4] = vertBase + (y+1) * (sliceTess+1) + x + 1;
                        tessIndices[index+5] = vertBase + (y+1) * (sliceTess+1) + x;
                    }
                    else
                    {
                        tessIndices[index+0] = vertBase + y * (sliceTess+1) + x;
                        tessIndices[index+1] = vertBase + y * (sliceTess+1) + x + 1;
                        tessIndices[index+2] = vertBase + (y+1) * (sliceTess+1) + x;

                        tessIndices[index+3] = vertBase + (y+1) * (sliceTess+1) + x;
                        tessIndices[index+4] = vertBase + y * (sliceTess+1) + x + 1;
                        tessIndices[index+5] = vertBase + (y+1) * (sliceTess+1) + x + 1;
                    }
                    index += 6;
                }
            }
        }
    }

    geometry.vertexCount = vertexCount;
    geometry.indexCount = index;

    glGenBuffers( 1, &geometry.vertexBuffer );
    glBindBuffer( GL_ARRAY_BUFFER, geometry.vertexBuffer );
    glBufferData( GL_ARRAY_BUFFER, floatCount * sizeof(*tessVertices), (void *)tessVertices, GL_STATIC_DRAW );
    delete[] tessVertices;

    glGenBuffers( 1, &geometry.indexBuffer );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, geometry.indexBuffer );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, indexBytes, (void *)tessIndices, GL_STATIC_DRAW );
    delete[] tessIndices;


    glEnableVertexAttribArray( VERTEX_POSITION );
    glVertexAttribPointer( VERTEX_POSITION, 2, GL_FLOAT, false, attribCount * sizeof( float ), (void *)( 0 * sizeof( float ) ) );

    glEnableVertexAttribArray( VERTEX_NORMAL );
    glVertexAttribPointer( VERTEX_NORMAL, 2, GL_FLOAT, false, attribCount * sizeof( float ), (void *)( 2 * sizeof( float ) ) );

    glEnableVertexAttribArray( VERTEX_UVC0 );
    glVertexAttribPointer( VERTEX_UVC0, 2, GL_FLOAT, false, attribCount * sizeof( float ), (void *)( 4 * sizeof( float ) ) );

    glEnableVertexAttribArray( VERTEX_TANGENT );
    glVertexAttribPointer( VERTEX_TANGENT, 2, GL_FLOAT, false, attribCount * sizeof( float ), (void *)( 6 * sizeof( float ) ) );

    glEnableVertexAttribArray( VERTEX_UVC1 );
    glVertexAttribPointer( VERTEX_UVC1, 2, GL_FLOAT, false, attribCount * sizeof( float ), (void *)( 8 * sizeof( float ) ) );


    glOperation.glBindVertexArrayOES( 0 );

    delete[] vertInCursor;

    free( (void *)bufferVerts );
    bufferVerts = NULL;

    return geometry;
}

VLensDistortion::VLensDistortion()
{
    for ( int i = 0; i < MaxCoefficients; i++ )
    {
        K[i] = 0.0f;
        InvK[i] = 0.0f;
    }
    Eqn = Distortion_RecipPoly4;
    K[0] = 1.0f;
    InvK[0] = 1.0f;
    MaxR = 1.0f;
    MaxInvR = 1.0f;
    ChromaticAberration[0] = -0.006f;
    ChromaticAberration[1] = 0.0f;
    ChromaticAberration[2] = 0.014f;
    ChromaticAberration[3] = 0.0f;
    MetersPerTanAngleAtCenter = 0.043875f;
}

void VLensDistortion::initLensByPhoneType(PhoneTypeEnum type)
{
    switch( type )
    {
        case HMD_GALAXY_S4:			// Galaxy S4 in Samsung's holder
            Eqn = Distortion_RecipPoly4;
            MetersPerTanAngleAtCenter = 0.043875f;
            K[0] = 0.756f;
            K[1] = -0.266f;
            K[2] = -0.389f;
            K[3] = 0.158f;
            break;

        case HMD_GALAXY_S5:      // Galaxy S5 1080 paired with version 2 lenses
            // Tuned for S5 DK2 with lens version 2 for E3 2014 (06-06-14)
            Eqn = Distortion_CatmullRom10;
            MetersPerTanAngleAtCenter     = 0.037f;
            K[0]                          = 1.0f;
            K[1]                          = 1.021f;
            K[2]                          = 1.051f;
            K[3]                          = 1.086f;
            K[4]                          = 1.128f;
            K[5]                          = 1.177f;
            K[6]                          = 1.232f;
            K[7]                          = 1.295f;
            K[8]                          = 1.368f;
            K[9]                          = 1.452f;
            K[10]                         = 1.560f;
            break;

        case HMD_GALAXY_S5_WQHD:            // Galaxy S5 1440 paired with version 2 lenses
            // Tuned for S5 DK2 with lens version 2 for E3 2014 (06-06-14)
            Eqn = Distortion_CatmullRom10;
            MetersPerTanAngleAtCenter     = 0.037f;
            K[0]                          = 1.0f;
            K[1]                          = 1.021f;
            K[2]                          = 1.051f;
            K[3]                          = 1.086f;
            K[4]                          = 1.128f;
            K[5]                          = 1.177f;
            K[6]                          = 1.232f;
            K[7]                          = 1.295f;
            K[8]                          = 1.368f;
            K[9]                          = 1.452f;
            K[10]                         = 1.560f;
            break;

        default:
        case HMD_NOTE_4:      // Note 4
            // GearVR (Note 4)
            Eqn = Distortion_CatmullRom10;
            MetersPerTanAngleAtCenter     = 0.0365f;
            K[0]                          = 1.0f;
            K[1]                          = 1.029f;
            K[2]                          = 1.0565f;
            K[3]                          = 1.088f;
            K[4]                          = 1.127f;
            K[5]                          = 1.175f;
            K[6]                          = 1.232f;
            K[7]                          = 1.298f;
            K[8]                          = 1.375f;
            K[9]                          = 1.464f;
            K[10]                         = 1.570f;
            break;
    }
}

NV_NAMESPACE_END
