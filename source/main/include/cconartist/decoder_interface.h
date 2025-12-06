#ifndef __CCONARTIST_DECODER_INTERFACE_H__
#define __CCONARTIST_DECODER_INTERFACE_H__

#include "cconartist/sensor_types.h"
#include "cconartist/units.h"

// The format of an image packet received over the wire
// Image data is split into multiple packets for transmission
struct img_packet_header_t
{
    unsigned char  m_mac[6];        // MAC address
    unsigned char  m_signature[4];  // 'I','M','G', 4th byte is version
    unsigned short m_dummy;
    unsigned int   m_image_type;  // 0 = JPEG, 1 = PNG
    unsigned int   m_image_block_count;
    unsigned int   m_image_block_index;
    unsigned int   m_image_data_offset;
    unsigned int   m_image_data_size;
    unsigned int   m_image_total_size;
};

struct stream_image_t
{
    unsigned int m_image_type;
    unsigned int m_total_size;  // total size of image data that follows this header
};

struct stream_json_t
{
    unsigned int m_json_size;  // total size of json data that follows this header
};

class connection_interface_t
{
public:
    unsigned char *stream_allocate(unsigned int size) { return v_stream_allocate(size); }
    void          *heap_malloc(unsigned int size) { return v_heap_malloc(size); }
    void           heap_free(void *ptr) { v_heap_free(ptr); }

    template <typename T>
    inline T *heap_allocate()
    {
        void *mem = v_heap_malloc(sizeof(T));
        return new (mem) T();
    }

    template <typename T>
    inline void heap_deallocate(T *obj)
    {
        obj->~T();
        v_heap_free(obj);
    }

protected:
    virtual unsigned char *v_stream_allocate(unsigned int size) = 0;
    virtual void          *v_heap_malloc(unsigned int size)     = 0;
    virtual void           v_heap_free(void *ptr)               = 0;
};

// Note: Every connection must have its own context, TCP connection, UDP connection, etc.
struct connection_context_t
{
    connection_interface_t *m_connection_interface;
    void                   *m_user_context0;
    void                   *m_user_context1;
    int                     m_user_data0;
    int                     m_user_data1;
};

// Write to stream, will return non-null when the full packet has been received, the pointer is poiting into the
// memory mapped stream.
typedef void *(*write_to_stream_fn)(connection_context_t *ctx, const unsigned char *packet_data, unsigned int packet_size);

// UI decoder functions

enum EImageType
{
    ImageTypeJPEG       = 120,
    ImageTypePNG        = 121,
    ImageTypeRGBA32     = 122,
    ImageTypeRGB565     = 123,
    ImageTypeGreyScale8 = 124,
};

enum EUIItemType
{
    UIItemInvalid = 0,
    UIItemText    = 1,
    UIItemImage   = 2,
};

struct ui_item_t
{
    unsigned short m_type;
    unsigned short m_i0;
    unsigned short m_i1;
    unsigned short m_i2;
    void          *m_d0;
    void          *m_d1;
};

struct ui_text_item_t
{
    unsigned short m_type;
    unsigned short m_key_len;
    unsigned short m_value_len;
    unsigned short m_dummy;
    const char    *m_key;
    const char    *m_value;
};

struct ui_image_item_t
{
    unsigned short m_type;
    unsigned short m_key_len;
    unsigned int   m_data_size;
    const char    *m_key;
    unsigned char *m_data;
};

struct ui_element_t
{
    int        m_count;
    int        m_dummy;
    ui_item_t *m_items;
};

typedef void *(*decoder_alloc_fn_t)(unsigned int size, unsigned char alignment);
typedef void *(*decoder_dealloc_fn_t)(unsigned int size, unsigned char alignment);
typedef const char *(*decoder_alloc_string_fn_t)(const char *str);

// Note: UI functions do not have to deallocate, since all memory will be freed
//       at the end of a frame (e.g. frame allocator strategy)

class ui_builder_t
{
public:
    void *heap_malloc(unsigned int size) { return v_heap_malloc(size); }
    void  heap_free(void *ptr) { v_heap_free(ptr); }

    inline char *string_allocate(const char *str)
    {
        unsigned int len      = strlen(str);
        char        *str_copy = (char *)v_heap_malloc((unsigned int)(len + 1));
        memcpy(str_copy, str, len);
        str_copy[len] = '\0';
        return str_copy;
    }

    inline char *string_allocate(const char *str, int str_len)
    {
        char *str_copy = (char *)v_heap_malloc((unsigned int)(str_len + 1));
        memcpy(str_copy, str, str_len);
        str_copy[str_len] = '\0';
        return str_copy;
    }

    template <typename T>
    inline T *heap_allocate()
    {
        void *mem = v_heap_malloc(sizeof(T));
        return new (mem) T();
    }

    template <typename T>
    inline void heap_object_deallocate(T *obj)
    {
        obj->~T();
        v_heap_free(obj);
    }

    template <typename T>
    inline T *heap_allocate_array(unsigned int count)
    {
        void *mem = v_heap_malloc(sizeof(T) * count);
        return (T *)mem;
    }

protected:
    virtual void *v_heap_malloc(unsigned int size) = 0;
    virtual void  v_heap_free(void *ptr)           = 0;
};

typedef ui_element_t *(*build_ui_element_fn)(connection_context_t *context, const unsigned char *stream_data, ui_builder_t *decode_ui);

#endif  // __CCONARTIST_DECODER_PROPERTY_H__
