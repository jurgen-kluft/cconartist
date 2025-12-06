#ifndef DECODER_INTERFACE_H
#define DECODER_INTERFACE_H

#include "cconartist/decoder_interface.h"

#if defined(_WIN32)
#    define EXPORT __declspec(dllexport)
#else
#    define EXPORT __attribute__((visibility("default")))
#endif

inline unsigned short read_u16_be(const unsigned char* data) { return (((unsigned short)data[0] << 8) | (unsigned short)data[1]); }
inline unsigned short read_u16_le(const unsigned char* data) { return ((unsigned short)data[0] | (unsigned short)(data[1] << 8)); }
inline unsigned int   read_u32_be(const unsigned char* data) { return (((unsigned int)data[0] << 24) | ((unsigned int)data[1] << 16) | ((unsigned int)data[2] << 8) | (unsigned int)data[3]); }
inline unsigned int   read_u32_le(const unsigned char* data) { return ((unsigned int)data[0] | ((unsigned int)data[1] << 8) | ((unsigned int)data[2] << 16) | ((unsigned int)data[3] << 24)); }

inline short read_i16_be(const unsigned char* data) { return (short)(((unsigned short)data[0] << 8) | (unsigned short)data[1]); }
inline short read_i16_le(const unsigned char* data) { return (short)((unsigned short)data[0] | (unsigned short)(data[1] << 8)); }
inline int   read_i32_be(const unsigned char* data) { return (int)(((unsigned int)data[0] << 24) | ((unsigned int)data[1] << 16) | ((unsigned int)data[2] << 8) | (unsigned int)data[3]); }
inline int   read_i32_le(const unsigned char* data) { return (int)((unsigned int)data[0] | ((unsigned int)data[1] << 8) | ((unsigned int)data[2] << 16) | ((unsigned int)data[3] << 24)); }

#endif  // DECODER_INTERFACE_H
