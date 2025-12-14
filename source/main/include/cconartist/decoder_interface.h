#ifndef __CCONARTIST_DECODER_INTERFACE_H__
#define __CCONARTIST_DECODER_INTERFACE_H__

#include "cconartist/user_types.h"
#include "cconartist/value_unit.h"

struct connection_context_t;

// The format of an image packet received over the wire
// Image data is split into multiple packets for transmission
struct image_packet_header_t
{
    unsigned char  m_mac[6];        // MAC address
    unsigned char  m_signature[4];  // 'I','M','G', 4th byte is version
    unsigned short m_image_type;    // 0 = JPEG, 1 = PNG
    unsigned short m_image_bpp;
    unsigned short m_image_width;
    unsigned short m_image_height;
    unsigned int   m_image_block_count;
    unsigned int   m_image_block_index;
    unsigned int   m_image_data_offset;
    unsigned int   m_image_data_size;
    unsigned int   m_image_total_size;
};

class decoder_allocator_t
{
public:
    void *allocate(unsigned int size) { return v_allocate(size); }
    void  deallocate(void *ptr) { v_deallocate(ptr); }

    template <typename T>
    inline T *allocate_array(unsigned int count)
    {
        void *mem = v_allocate(sizeof(T) * count);
        return new (mem) T[count];
    }

    template <typename T>
    inline T *_new()
    {
        void *mem = v_allocate(sizeof(T));
        return new (mem) T();
    }

    template <typename T>
    inline void _delete(T *obj)
    {
        obj->~T();
        v_deallocate(obj);
    }

    inline char *string_allocate(const char *str)
    {
        unsigned int len      = strlen(str);
        char        *str_copy = (char *)v_allocate((unsigned int)(len + 1));
        memcpy(str_copy, str, len);
        str_copy[len] = '\0';
        return str_copy;
    }

    inline char *string_allocate(const char *str, int str_len)
    {
        char *str_copy = (char *)v_allocate((unsigned int)(str_len + 1));
        memcpy(str_copy, str, str_len);
        str_copy[str_len] = '\0';
        return str_copy;
    }

protected:
    virtual void *v_allocate(unsigned int size) = 0;
    virtual void  v_deallocate(void *ptr)       = 0;
};

class decoder_stream_interface_t
{
public:
    bool allocate_vardata(uint64_t user_id, uint64_t time, uint8_t *&data, uint32_t data_size) { return v_allocate_vardata(user_id, time, data, data_size); }
    bool allocate_fixdata(uint64_t user_id, uint64_t time, uint8_t *&data, uint32_t data_size) { return v_allocate_fixdata(user_id, time, data, data_size); }
    void write_var_data(uint64_t user_id, uint64_t time, uint8_t const *data, uint32_t data_size) { v_write_var_data(user_id, time, data, data_size); }
    void write_fix_data(uint64_t user_id, uint64_t time, uint8_t const *data, uint32_t data_size) { v_write_fix_data(user_id, time, data, data_size); }
    void write_u8(uint64_t user_id, uint64_t time, uint8_t value) { v_write_u8(user_id, time, value); }
    void write_u16(uint64_t user_id, uint64_t time, uint16_t value) { v_write_u16(user_id, time, value); }
    void write_u32(uint64_t user_id, uint64_t time, uint32_t value) { v_write_u32(user_id, time, value); }
    void write_f32(uint64_t user_id, uint64_t time, float value) { v_write_f32(user_id, time, value); }
    void write_f64(uint64_t user_id, uint64_t time, double value) { v_write_f64(user_id, time, value); }

protected:
    virtual bool v_allocate_vardata(uint64_t user_id, uint64_t time, uint8_t *&data, uint32_t data_size)    = 0;
    virtual bool v_allocate_fixdata(uint64_t user_id, uint64_t time, uint8_t *&data, uint32_t data_size)    = 0;
    virtual void v_write_var_data(uint64_t user_id, uint64_t time, uint8_t const *data, uint32_t data_size) = 0;
    virtual void v_write_fix_data(uint64_t user_id, uint64_t time, uint8_t const *data, uint32_t data_size) = 0;
    virtual void v_write_u8(uint64_t user_id, uint64_t time, uint8_t value)                                 = 0;
    virtual void v_write_u16(uint64_t user_id, uint64_t time, uint16_t value)                               = 0;
    virtual void v_write_u32(uint64_t user_id, uint64_t time, uint32_t value)                               = 0;
    virtual void v_write_f32(uint64_t user_id, uint64_t time, float value)                                  = 0;
    virtual void v_write_f64(uint64_t user_id, uint64_t time, double value)                                 = 0;
};

// Note: Every connection (TCP connection, UDP connection) must have its own context.
struct decoder_context_t
{
    decoder_allocator_t        *m_temp;
    decoder_allocator_t        *m_main_heap;
    decoder_allocator_t        *m_ui_heap;
    decoder_stream_interface_t *m_stream;
    void                       *m_user_context0;
    void                       *m_user_context1;
    int                         m_user_data0;
    int                         m_user_data1;
};

// This is for a decoder plugin to initialize its internal state
typedef void (*decoder_initialize_fn)(decoder_context_t *ctx);

// Write to stream, will return non-null when the full packet has been received, the pointer is pointing into the
// memory mapped stream.
typedef void (*decoder_write_to_stream_fn)(decoder_context_t *ctx, const unsigned char *packet_data, unsigned int packet_size);

// UI decoder functions

enum EDecoderImageType
{
    ImageTypeJPEG       = 120,
    ImageTypePNG        = 121,
    ImageTypeRGBA32     = 122,
    ImageTypeRGB565     = 123,
    ImageTypeGreyScale8 = 124,
};

enum EDecoderUIItemType
{
    UIItemInvalid = 0,
    UIItemText    = 1,
    UIItemImage   = 2,
};

struct decoder_ui_item_t
{
    unsigned short m_type;
    unsigned short m_i0;
    unsigned short m_i1;
    unsigned short m_i2;
    void          *m_d0;
    void          *m_d1;
};

struct decoder_ui_text_item_t
{
    unsigned short m_type;
    unsigned short m_key_len;
    unsigned short m_value_len;
    unsigned short m_dummy;
    const char    *m_key;
    const char    *m_value;
};

struct decoder_ui_image_item_t
{
    unsigned short m_type;
    unsigned short m_key_len;
    unsigned int   m_data_size;
    const char    *m_key;
    unsigned char *m_data;
};

struct decoder_ui_element_t
{
    int                m_count;
    int                m_dummy;
    decoder_ui_item_t *m_items;
};

typedef void *(*decoder_alloc_fn_t)(unsigned int size, unsigned char alignment);
typedef void *(*decoder_dealloc_fn_t)(unsigned int size, unsigned char alignment);
typedef const char *(*decoder_alloc_string_fn_t)(const char *str);

// Note: UI functions do not have to deallocate, since all memory will be freed
//       at the end of a frame (e.g. frame allocator strategy)

typedef decoder_ui_element_t *(*decoder_build_ui_element_fn)(decoder_context_t *context, uint64_t user_id, const unsigned char *stream_data, unsigned int stream_data_size);

#endif  // __CCONARTIST_DECODER_PROPERTY_H__
