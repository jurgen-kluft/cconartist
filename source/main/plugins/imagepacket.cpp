
#include "../cpp/value_unit.cpp"
#include "../cpp/user_types.cpp"

#include <string>
#include <ctime>

#include "decoder_interface.h"

#ifdef __cplusplus
extern "C"
{
#endif

    struct stream_image_t
    {
        unsigned short m_image_type;  // 0 = JPEG, 1 = PNG
        unsigned short m_image_bpp;
        unsigned short m_image_width;
        unsigned short m_image_height;
        unsigned int   m_total_size;
    };

    struct image_t
    {
        unsigned int    m_image_type;
        unsigned int    m_total_size;
        unsigned int    m_total_block_count;
        unsigned int    m_received_block_count;
        stream_image_t* m_stream_image;
        unsigned char*  m_stream_image_data;
    };

    void decoder_write_to_stream(decoder_context_t* ctx, const unsigned char* packet_data, unsigned int packet_size)
    {
        image_packet_header_t* packet_hdr = (image_packet_header_t*)packet_data;
        const uint8_t*         mac        = packet_hdr->m_mac;

        image_t* image = (image_t*)ctx->m_user_context0;
        if (image == nullptr)
        {
            image_t* image = ctx->m_main_heap->_new<image_t>();
            if (image == nullptr)
                return;

            const uint64_t user_id      = ((uint64_t)mac[0] << 40) | ((uint64_t)mac[1] << 32) | ((uint64_t)mac[2] << 24) | ((uint64_t)mac[3] << 16) | ((uint64_t)mac[4] << 8) | (uint64_t)mac[5];
            const uint64_t current_time = static_cast<uint64_t>(std::time(nullptr));

            image->m_image_type        = packet_hdr->m_image_type;
            image->m_total_size        = packet_hdr->m_image_total_size;
            image->m_total_block_count = packet_hdr->m_image_block_count;
            uint8_t* stream_data;
            ctx->m_stream->allocate_vardata((user_id << 16) | ID_IMAGE, current_time, stream_data, (uint32_t)sizeof(stream_image_t) + image->m_total_size);
            image->m_stream_image               = (stream_image_t*)stream_data;
            image->m_stream_image->m_image_type = packet_hdr->m_image_type;
            image->m_stream_image->m_total_size = packet_hdr->m_image_total_size;
            image->m_stream_image_data          = stream_data + sizeof(stream_image_t);
        }

        // Copy this data into the image data at the correct offset
        memcpy(image->m_stream_image_data + packet_hdr->m_image_data_offset, packet_data + sizeof(image_packet_header_t), packet_hdr->m_image_data_size);

        // Does this image now have all its blocks?
        image->m_received_block_count += 1;
        if (image->m_received_block_count == image->m_total_block_count)
        {
            //void* data = (void*)((unsigned char*)image->m_stream_image);

            // Deallocate the image object
            ctx->m_main_heap->_delete(image);
        }

    }

    decoder_ui_element_t* decoder_build_ui_element(decoder_context_t* ctx, const unsigned char* stream_data, unsigned int stream_data_size)
    {
        const stream_image_t* stream_image = (const stream_image_t*)stream_data;

        decoder_ui_element_t* ui_element = ctx->m_ui_heap->_new<decoder_ui_element_t>();

        // Allocate one UI item for the image
        decoder_ui_image_item_t* image_item = ctx->m_ui_heap->_new<decoder_ui_image_item_t>();
        image_item->m_type                  = UIItemImage;
        image_item->m_key                   = ctx->m_ui_heap->string_allocate("Image");
        image_item->m_key_len               = (unsigned short)strlen(image_item->m_key);
        image_item->m_data_size             = stream_image->m_total_size;
        image_item->m_data                  = (unsigned char*)stream_image + sizeof(stream_image_t);

        ui_element->m_count = 1;
        ui_element->m_items = (decoder_ui_item_t*)image_item;

        return ui_element;
    }

    void decoder_initialize(decoder_context_t *ctx)
    {

    }

#ifdef __cplusplus
}
#endif
