#ifndef VIMAGELOADER_H
#define VIMAGELOADER_H

#include "VReferenceCounted.h"
#include "VImage.h"
#include "VPath.h"
#include "VFile.h"

namespace NervGear {

//! Class which is able to create a image from a file.
/** If you want the Irrlicht Engine be able to load textures of
currently unsupported file formats (e.g .gif), then implement
this and add your new Surface loader with
IVideoDriver::addExternalImageLoader() to the engine. */
class VImageLoader : public virtual VReferenceCounted
{
public:

    //! Check if the file might be loaded by this class
    /** Check is based on the file extension (e.g. ".tga")
    \param filename Name of file to check.
    \return True if file seems to be loadable. */
    virtual bool isALoadableFileExtension(const VPath& filename) const = 0;

    //! Check if the file might be loaded by this class
    /** Check might look into the file.
    \param file File handle to check.
    \return True if file seems to be loadable. */
    virtual bool isALoadableFileFormat(VFile* file) const = 0;

    //! Creates a surface from the file
    /** \param file File handle to check.
    \return Pointer to newly created image, or 0 upon error. */
    virtual VImage* loadImage(VFile* file) const = 0;
};

}


#endif // VIMAGELOADER_H

