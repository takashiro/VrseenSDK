#include "sensor/Profile.h"
#include "sensor/Device.h"
#include "VDevice.h"
#include "VAlgorithm.h"
#include "VGlShader.h"
#include "VEglDriver.h"

#include "VLensDistortion.h"


NV_NAMESPACE_BEGIN

static float EvalCatmullRomSpline ( float const *K, float scaledVal, int NumSegments )
{
    float scaledValFloor = floorf ( scaledVal );
    scaledValFloor = std::max ( 0.0f, std::min ( (float)(NumSegments-1), scaledValFloor ) );
    float t = scaledVal - scaledValFloor;
    int k = (int)scaledValFloor;

    float p0 = 0.0f;
    float p1 = 0.0f;
    float m0 = 0.0f;
    float m1 = 0.0f;

    if (k == 0)
    {
        p0 = 1.0f;
        m0 = K[1] - K[0];
        p1 = K[1];
        m1 = 0.5f * ( K[2] - K[0] );
    }
    else if (k < NumSegments-2)
    {
        p0 = K[k];
        m0 = 0.5f * ( K[k+1] - K[k-1] );
        p1 = K[k+1];
        m1 = 0.5f * ( K[k+2] - K[k] );
    }
    else if (k == NumSegments-2)
    {
        p0 = K[k];
        m0 = 0.5f * ( K[k+1] - K[k-1] );
        p1 = K[k+1];
        m1 = K[k+1] - K[k];
    }
    else if (k == NumSegments-1)
    {
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


static float DistortionFnScaleRadiusSquared(const VLensDistortion& lens,float rsq)
{
    float scale = 1.0f;
    switch ( lens.equation )
    {
        case Distortion_Poly4:
            scale = ( lens.kArray[0] + rsq * ( lens.kArray[1] + rsq * ( lens.kArray[2] + rsq * lens.kArray[3] ) ) );
            break;
        case Distortion_RecipPoly4:
            scale = 1.0f / ( lens.kArray[0] + rsq * ( lens.kArray[1] + rsq * ( lens.kArray[2] + rsq * lens.kArray[3] ) ) );
            break;
        case Distortion_CatmullRom10: {

            const int NumSegments = 11;
            vAssert( NumSegments <= lens.MaxCoefficients );
            float scaledRsq = (float)(NumSegments-1) * rsq / ( lens.maxR * lens.maxR );
            scale = EvalCatmullRomSpline ( lens.kArray, scaledRsq, NumSegments );
        } break;

        case Distortion_CatmullRom20: {

            const int NumSegments = 21;
            vAssert(NumSegments <= lens.MaxCoefficients);
            float scaledRsq = (float)(NumSegments-1) * rsq / ( lens.maxR * lens.maxR );
            scale = EvalCatmullRomSpline ( lens.kArray, scaledRsq, NumSegments );
        } break;

        default:
            vAssert(false);
            break;
    }
    return scale;
}

static V3Vectf DistortionFnScaleRadiusSquaredChroma (const VLensDistortion& lens,float rsq)
{
    float scale = DistortionFnScaleRadiusSquared (lens, rsq );
    V3Vectf scaleRGB;
    scaleRGB.x = scale * ( 1.0f + lens.chromaticAberration[0] + rsq * lens.chromaticAberration[1] );     // Red
    scaleRGB.y = scale;                                                                        // Green
    scaleRGB.z = scale * ( 1.0f + lens.chromaticAberration[2] + rsq * lens.chromaticAberration[3] );     // Blue
    return scaleRGB;
}

static void WarpTexCoordChroma( const VDevice* device, const float in[2],
                                float red[2], float green[2], float blue[2] ) {
    float theta[2];
    for ( int i = 0; i < 2; i++ ) {
        const float unit = in[i];
        const float ndc = 2.0f * ( unit - 0.5f );
        const float pixels = ndc * device->heightbyPixels * 0.5f;
        const float meters = pixels * device->widthbyMeters / device->widthbyPixels;
        const float tanAngle = meters / device->lens.centMetersPerTanAngler;
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

static bool VectorHitsCursor( const V2Vectf & v )
{
    if ( fabs( v.y ) > 0.017f )
    {
        return false;
    }
    if ( fabs( v.x ) > 0.070f )
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


    const float aspect = device->widthbyPixels * 0.5 / device->heightbyPixels;

    const float	horizontalShiftLeftMeters =  -(( device->lensDistance / 2 ) - ( device->widthbyMeters / 4 )) + device->xxOffsetbyMeters;
    const float horizontalShiftRightMeters = (( device->lensDistance / 2 ) - ( device->widthbyMeters / 4 )) + device->xxOffsetbyMeters;
    const float	horizontalShiftViewLeft = 2 * aspect * horizontalShiftLeftMeters / device->widthbyMeters;
    const float	horizontalShiftViewRight = 2 * aspect * horizontalShiftRightMeters / device->widthbyMeters;

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

int VLensDistortion::xxGridNum = 32;
int VLensDistortion::yyGridNum = 32;

VGlGeometry VLensDistortion::createDistortionGrid(const VDevice* device,const int numSlicesPerEye, const float fovScale,
                                                   const bool cursorOnly)
{

    VGlGeometry geometry;
    const int totalX = (xxGridNum+1)*2;

    if (xxGridNum < 1 || yyGridNum < 1 ) return geometry;

    const float * bufferVerts = &((float *)CreateVertexBuffer(device,xxGridNum,yyGridNum))[0];

    // Identify which verts would be inside the cursor plane
    bool * vertInCursor = NULL;
    if ( cursorOnly )
    {
        vertInCursor = new bool[totalX*(yyGridNum+1)];
        for ( int y = 0; y <= yyGridNum; y++ )
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


    VEglDriver::glGenVertexArraysOES( 1, &geometry.vertexArrayObject );
    VEglDriver::glBindVertexArrayOES( geometry.vertexArrayObject );

    const int attribCount = 10;
    const int sliceTess = xxGridNum / numSlicesPerEye;

    const int vertexCount = 2*numSlicesPerEye*(sliceTess+1)*(yyGridNum+1);
    const int floatCount = vertexCount * attribCount;
    float * tessVertices = new float[floatCount];

    const int indexCount = 2*xxGridNum*yyGridNum*6;
    unsigned short * tessIndices = new unsigned short[indexCount];
    const int indexBytes = indexCount * sizeof( *tessIndices );

    int	index = 0;
    int	verts = 0;

    for ( int eye = 0; eye < 2; eye++ )
    {
        for ( int slice = 0; slice < numSlicesPerEye; slice++ )
        {
            const int vertBase = verts;

            for ( int y = 0; y <= yyGridNum; y++ )
            {
                const float	yf = (float)y / (float)yyGridNum;
                for ( int x = 0; x <= sliceTess; x++ )
                {
                    const int sx = slice * sliceTess + x;
                    const float	xf = (float)sx / (float)xxGridNum;
                    float * v = &tessVertices[attribCount * ( vertBase + y * (sliceTess+1) + x ) ];
                    v[0] = -1.0 + eye + xf;
                    v[1] = yf*2.0f - 1.0f;

                    // Copy the offsets from the file
                    for ( int i = 0 ; i < 6; i++ )
                    {
                        v[2+i] = fovScale * bufferVerts
                        [(y*(xxGridNum+1)*2+sx + eye * (xxGridNum+1))*6+i];
                    }

                    v[8] = (float)x / sliceTess;

                    if ( 0 && ( y == 0 || y == yyGridNum || sx == 0 || sx == xxGridNum ) )
                    {
                        v[9] = 0.0f;	// fade to black at edge
                    }
                    else
                    {
                        v[9] = 1.0f;
                    }
                }
            }
            verts += (yyGridNum+1)*(sliceTess+1);


            for ( int x = 0; x < sliceTess; x++ )
            {
                for ( int y = 0; y < yyGridNum; y++ )
                {
                    if ( vertInCursor )
                    {	// skip this quad if none of the verts are in the cursor region
                        const int xx = x + eye * (xxGridNum+1) + slice * sliceTess;
                        if ( 0 ==
                             vertInCursor[ y * totalX + xx ]
                             + vertInCursor[ y * totalX + xx + 1 ]
                             + vertInCursor[ (y + 1) * totalX + xx ]
                             + vertInCursor[ (y + 1) * totalX + xx + 1 ] )
                        {
                            continue;
                        }
                    }


                    if ( (slice*sliceTess+x <  xxGridNum/2) ^ (y < (yyGridNum/2)) )
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


    VEglDriver::glBindVertexArrayOES( 0 );

    delete[] vertInCursor;

    free( (void *)bufferVerts );
    bufferVerts = NULL;

    return geometry;
}

VLensDistortion::VLensDistortion()
{
    for ( int i = 0; i < MaxCoefficients; i++ )
    {
        kArray[i] = 0.0f;
        invKArray[i] = 0.0f;
    }
    equation = Distortion_RecipPoly4;
    kArray[0] = 1.0f;
    invKArray[0] = 1.0f;
    maxR = 1.0f;
    maxInvR = 1.0f;
    chromaticAberration[0] = -0.006f;
    chromaticAberration[1] = 0.0f;
    chromaticAberration[2] = 0.014f;
    chromaticAberration[3] = 0.0f;
    centMetersPerTanAngler = 0.043875f;
}

void VLensDistortion::initDistortionParmsByMobileType(PhoneTypeEnum type)
{
    switch( type )
    {
        case HMD_GALAXY_S4:
            equation = Distortion_RecipPoly4;
            centMetersPerTanAngler = 0.043875f;
            kArray[0] = 0.756f;
            kArray[1] = -0.266f;
            kArray[2] = -0.389f;
            kArray[3] = 0.158f;
            break;

        case HMD_GALAXY_S5:

            equation = Distortion_CatmullRom10;
            centMetersPerTanAngler     = 0.037f;
            kArray[0]                          = 1.0f;
            kArray[1]                          = 1.021f;
            kArray[2]                          = 1.051f;
            kArray[3]                          = 1.086f;
            kArray[4]                          = 1.128f;
            kArray[5]                          = 1.177f;
            kArray[6]                          = 1.232f;
            kArray[7]                          = 1.295f;
            kArray[8]                          = 1.368f;
            kArray[9]                          = 1.452f;
            kArray[10]                         = 1.560f;
            break;

        case HMD_GALAXY_S5_WQHD:

            equation = Distortion_CatmullRom10;
            centMetersPerTanAngler     = 0.037f;
            kArray[0]                          = 1.0f;
            kArray[1]                          = 1.021f;
            kArray[2]                          = 1.051f;
            kArray[3]                          = 1.086f;
            kArray[4]                          = 1.128f;
            kArray[5]                          = 1.177f;
            kArray[6]                          = 1.232f;
            kArray[7]                          = 1.295f;
            kArray[8]                          = 1.368f;
            kArray[9]                          = 1.452f;
            kArray[10]                         = 1.560f;
            break;

        default:
        case HMD_NOTE_4:

            equation = Distortion_CatmullRom10;
            centMetersPerTanAngler     = 0.0365f;
            kArray[0]                          = 1.0f;
            kArray[1]                          = 1.029f;
            kArray[2]                          = 1.0565f;
            kArray[3]                          = 1.088f;
            kArray[4]                          = 1.127f;
            kArray[5]                          = 1.175f;
            kArray[6]                          = 1.232f;
            kArray[7]                          = 1.298f;
            kArray[8]                          = 1.375f;
            kArray[9]                          = 1.464f;
            kArray[10]                         = 1.570f;
            break;
    }
}

NV_NAMESPACE_END
