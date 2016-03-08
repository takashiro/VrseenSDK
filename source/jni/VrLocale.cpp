/************************************************************************************

Filename    :   VrLocale.cpp
Content     :   Localization and internationalization (i18n) functionality.
Created     :   11/24/2914
Authors     :   Jonathan Wright

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

*************************************************************************************/

#include "VrLocale.h"

#include "Array.h"
#include "Std.h"
#include "android/JniUtils.h"
#include "android/LogUtils.h"
#include "VLog.h"

namespace NervGear {

char const *	VrLocale::LOCALIZED_KEY_PREFIX = "@string/";
size_t			VrLocale::LOCALIZED_KEY_PREFIX_LEN = strlen( LOCALIZED_KEY_PREFIX );
jclass			VrLocale::VrActivityClass;

bool VrLocale::GetString( JNIEnv* jni, jobject activityObject, char const * key, char const * defaultOut, VString & out )
{
    if (jni == NULL) {
        vWarn("OVR_ASSERT jni = NULL!");
	}
    if (activityObject == NULL) {
        vWarn("OVR_ASSERT ctivityObject = NULL!");
	}

	//LOG( "Localizing key '%s'", key );
	// if the key doesn't start with KEY_PREFIX then it's not a valid key, just return
	// the key itself as the output text.
	if ( strstr( key, LOCALIZED_KEY_PREFIX ) != key )
	{
		out = defaultOut;
        vInfo("no prefix, localized to '%s'" << out);
		return true;
	}

	char const * realKey = key + LOCALIZED_KEY_PREFIX_LEN;

    jmethodID const getLocalizedStringId = JniUtils::GetMethodID( jni, VrActivityClass, "getLocalizedString", "(Ljava/lang/String;)Ljava/lang/String;" );
	if ( getLocalizedStringId != NULL )
	{
		JavaString keyObj( jni, realKey );
        jstring jstr = static_cast<jstring>(jni->CallObjectMethod(activityObject, getLocalizedStringId, keyObj.toJString()));
        if (!jni->ExceptionOccurred()) {
            out = JniUtils::Convert(jni, jstr);

            if (out.isEmpty()) {
				out = defaultOut;
                vInfo("key not found, localized to '%s'" << out);
				return false;
			}
			return true;
		}
	}

	out = "JAVAERROR";
    OVR_ASSERT(false);
	return false;
}

//==============================
// VrLocale::MakeStringIdFromANSI
// Turns an arbitray ansi string into a string id.
// - Deletes any character that is not a space, letter or number.
// - Turn spaces into underscores.
// - Ignore contiguous spaces.
VString VrLocale::MakeStringId(const VString &str)
{
	enum eLastOutputType
	{
		LO_LETTER,
		LO_DIGIT,
		LO_SPACE,
		LO_PUNCTUATION,
		LO_MAX
	};
	eLastOutputType lastOutputType = LO_MAX;
	VString out = LOCALIZED_KEY_PREFIX;
    int i = 0;
    int n = str.length();
    if (str.startsWith(out)) {
        i += out.length();
        n -= out.length();
    }
    for (; i < n; i++) {
        char16_t c = str.at(i);
		if ( ( c >= '0' && c <= '9' ) )
		{
			if ( i == 0 )
			{
				// string identifiers in Android cannot start with a number because they
				// are also encoded as Java identifiers, so output an underscore first.
				out.append( '_' );
			}
			out.append( c );
			lastOutputType = LO_DIGIT;
		}
		else if ( ( c >= 'a' && c <= 'z' ) )
		{
			// just output the character
			out.append( c );
			lastOutputType = LO_LETTER;
		}
		else if ( ( c >= 'A' && c <= 'Z' ) )
		{
			// just output the character as lowercase
			out.append( c + 32 );
			lastOutputType = LO_LETTER;
		}
		else if ( c == 0x20 )
		{
			if ( lastOutputType != LO_SPACE )
			{
				out.append( '_' );
				lastOutputType = LO_SPACE;
			}
			continue;
		}
		// ignore everything else
	}
	return out;
}

//==============================
// Supports up to 9 arguments and %s format only
VString private_GetXliffFormattedString( const VString & inXliffStr, ... )
{
	// format spec looks like: %1$s - we expect at least 3 chars after %
	const int MIN_NUM_EXPECTED_FORMAT_CHARS = 3;

	// If the passed in string is shorter than minimum expected xliff formatting, just return it
	if ( static_cast< int >( inXliffStr.size() ) <= MIN_NUM_EXPECTED_FORMAT_CHARS )
	{
		return inXliffStr;
	}

	// Buffer that holds formatted return string
    VString retStrBuffer;

    std::u32string ucs4 = inXliffStr.toUcs4();
    std::u32string::iterator p = ucs4.begin();
    forever {
        uint32_t charCode = *p;
        p++;
        if (charCode == '\0') {
			break;
        } else if (charCode == '%') {
			// We found the start of the format specifier
			// Now check that there are at least three more characters which contain the format specification
			Array< uint32_t > formatSpec;
			for ( int count = 0; count < MIN_NUM_EXPECTED_FORMAT_CHARS; ++count )
			{
                uint32_t formatCharCode = *p;
                p++;
				formatSpec.append( formatCharCode );
			}

			OVR_ASSERT( formatSpec.size() >= MIN_NUM_EXPECTED_FORMAT_CHARS );

			uint32_t desiredArgIdxChar = formatSpec.at( 0 );
			uint32_t dollarThing = formatSpec.at( 1 );
			uint32_t specifier = formatSpec.at( 2 );

			// Checking if it has supported xliff format specifier
			if( ( desiredArgIdxChar >= '1' && desiredArgIdxChar <= '9' ) &&
				( dollarThing == '$' ) &&
				( specifier == 's' ) )
			{
				// Found format valid specifier, so processing entire format specifier.
				int desiredArgIdxint = desiredArgIdxChar - '0';

				va_list args;
				va_start( args, inXliffStr );

				// Loop till desired argument is found.
				for( int j = 0; ; ++j )
				{
					const char* tempArg = va_arg( args, const char* );
					if( j == ( desiredArgIdxint - 1 ) ) // found desired argument
					{
                        retStrBuffer.append(tempArg);
						break;
					}
				}

				va_end(args);
			}
			else
			{
                LOG( "%s has invalid xliff format - has unsupported format specifier.", inXliffStr.toCString() );
				return inXliffStr;
			}
		}
		else
		{
			retStrBuffer.append( charCode );
		}
	}

    return retStrBuffer;
}

VString VrLocale::GetXliffFormattedString( const VString & inXliffStr, const char * arg1 )
{
	return private_GetXliffFormattedString( inXliffStr, arg1 );
}

VString VrLocale::GetXliffFormattedString( const VString & inXliffStr, const char * arg1, const char * arg2 )
{
	return private_GetXliffFormattedString( inXliffStr, arg1, arg2 );
}

VString VrLocale::GetXliffFormattedString( const VString & inXliffStr, const char * arg1, const char * arg2, const char * arg3 )
{
	return private_GetXliffFormattedString( inXliffStr, arg1, arg2, arg3 );
}

VString VrLocale::ToString( char const * fmt, float const f )
{
	char buffer[128];
	OVR_sprintf( buffer, 128, fmt, f );
    return buffer;
}

VString VrLocale::ToString( char const * fmt, int const i )
{
	char buffer[128];
	OVR_sprintf( buffer, 128, fmt, i );
    return buffer;
}

} // namespace NervGear
