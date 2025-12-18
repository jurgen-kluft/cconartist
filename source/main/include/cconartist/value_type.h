#ifndef __CCONARTIST_DECODER_VALUE_TYPE_H__
#define __CCONARTIST_DECODER_VALUE_TYPE_H__

namespace nvaluetype
{
    typedef unsigned char enum_t;
    enum
    {
        TypeU8,
        TypeU16,
        TypeU32,
        TypeU64,
        TypeS8,
        TypeS16,
        TypeS32,
        TypeS64,
        TypeF32,
        TypeF64,
        TypeU1,
        TypeU2,
        TypeU3,
        TypeU4,
        TypeData,
        TypeString,
    };
}  // namespace nvaluetype

#endif  // __CCONARTIST_DECODER_VALUE_TYPE_H__
