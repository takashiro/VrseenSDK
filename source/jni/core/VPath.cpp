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

NV_NAMESPACE_END
