#ifndef VIMAGEMANAGER_H
#define VIMAGEMANAGER_H
#include "VImageCommonLoader.h"
#include "VImageKtxLoader.h"
#include "VImagePvrLoader.h"


namespace NervGear {

class VImageManager{

public:
    VImageManager();

    ~VImageManager();


    VImage* loadImage(const VPath &filename) const;

    VArray<VImageLoader*> ImageLoaderList;





};


}



#endif // VIMAGEMANAGER_H

