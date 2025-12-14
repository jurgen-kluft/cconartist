#ifndef __CCONARTIST_DECODER_VALUE_TYPE_H__
#define __CCONARTIST_DECODER_VALUE_TYPE_H__

enum EValueType
{
    ValueType_U8         = 0x0008,
    ValueType_U16        = 0x0010,
    ValueType_U32        = 0x0020,
    ValueType_U64        = 0x0040,
    ValueType_S8         = 0x0108,
    ValueType_S16        = 0x0110,
    ValueType_S32        = 0x0120,
    ValueType_S64        = 0x0140,
    ValueType_F32        = 0x0220,
    ValueType_F64        = 0x0240,
    ValueType_U1         = 0x0001,
    ValueType_U2         = 0x0002,
    ValueType_U3         = 0x0003,
    ValueType_U4         = 0x0004,
    ValueType_BinaryData = 0x1000,
    ValueType_TextData   = 0x2000,
};

#endif  // __CCONARTIST_DECODER_VALUE_TYPE_H__
