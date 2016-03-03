#include "VPath.h"

NV_NAMESPACE_BEGIN

bool VPath::isAbsolute() const
{
    // Treat empty strings as absolute.
    if (isEmpty()) {
        return true;
    }

    // Fist character of '/' or '\\' means absolute url.
    if (startsWith('/') || startsWith('\\')) {
        return true;
    }

    const VChar *iter = data() + 1;
    while (*iter != '\0') {
        // Treat a colon followed by a slash as absolute.
        if (*iter == ':') {
            iter++;
            // Protocol or windows drive. Absolute.
            if (*iter == '/' || *iter == '\\') {
                return true;
            }
        } else if (*iter == '/' || *iter == '\\') {
            // Not a first character (else 'if' above the loop would have caught it).
            // Must be a relative url.
            break;
        }
        iter++;
    }

    // Now it's a relative path.
    return false;
}

bool VPath::hasExtension() const
{
    uint i = size() - 1;
    forever {
        if (at(i) == '/' || at(i) == '\\') {
            return false;
        }

        if (at(i) == '.') {
            return true;
        }

        if (i > 0) {
            i--;
        } else {
            return false;
        }
    }
    return false;
}

VString VPath::extension() const
{
    uint i = size() - 1;
    forever {
        if (at(i) == '/' || at(i) == '\\') {
            return VString();
        }

        if (at(i) == '.') {
            return mid(i + 1);
        }

        if (i > 0) {
            i--;
        } else {
            return VString();
        }
    }
    return VString();
}

VString VPath::fileName() const
{
    uint i = size() - 1;
    forever {
        if (at(i) == '/' || at(i) == '\\') {
            return mid(i + 1);
        }

        if (i > 0) {
            i--;
        } else {
            return VString();
        }
    }
    return VString();
}

VString VPath::baseName() const
{
    VString fileName = this->fileName();
    if (fileName.isEmpty()) {
        return fileName;
    }

    uint i = fileName.size() - 1;
    forever {
        if (fileName.at(i) == '.') {
            return fileName.mid(0, i);
        }

        if (i > 0) {
            i--;
        } else {
            return fileName;
        }
    }
    return fileName;
}

NV_NAMESPACE_END
