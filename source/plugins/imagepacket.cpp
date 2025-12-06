
#include "../cpp/units.cpp"
#include "../cpp/sensor_types.cpp"

#include <string>

#include "decoder_interface.h"

#ifdef __cplusplus
extern "C"
{
#endif
    struct image_t
    {
        unsigned int    m_image_type;
        unsigned int    m_total_size;
        unsigned int    m_total_block_count;
        unsigned int    m_received_block_count;
        stream_image_t* m_stream_image;
        unsigned char*  m_stream_image_data;
    };

    void* write_to_stream(connection_context_t* ctx, const unsigned char* packet_data, unsigned int packet_size)
    {
        img_packet_header_t* packet_hdr = (img_packet_header_t*)packet_data;

        image_t* image = (image_t*)ctx->m_user_context0;
        if (image == nullptr)
        {
            image_t* image = ctx->m_connection_interface->heap_allocate<image_t>();
            if (image == nullptr)
                return nullptr;

            image->m_image_type                 = packet_hdr->m_image_type;
            image->m_total_size                 = packet_hdr->m_image_total_size;
            image->m_total_block_count          = packet_hdr->m_image_block_count;
            unsigned char* stream_data          = ctx->m_connection_interface->stream_allocate(sizeof(stream_image_t) + image->m_total_size);
            image->m_stream_image               = (stream_image_t*)stream_data;
            image->m_stream_image->m_image_type = packet_hdr->m_image_type;
            image->m_stream_image->m_total_size = packet_hdr->m_image_total_size;
            image->m_stream_image_data          = stream_data + sizeof(stream_image_t);
        }

        // Copy this data into the image data at the correct offset
        memcpy(image->m_stream_image_data + packet_hdr->m_image_data_offset, packet_data + sizeof(img_packet_header_t), packet_hdr->m_image_data_size);

        // Does this image now have all its blocks?
        image->m_received_block_count += 1;
        if (image->m_received_block_count == image->m_total_block_count)
        {
            void* data = (void*)((unsigned char*)image->m_stream_image);

            // Deallocate the image object
            ctx->m_connection_interface->heap_deallocate(image);
            return data;
        }

        return nullptr;
    }

    ui_element_t* build_ui_element(const unsigned char* stream_data, ui_builder_t* ui_builder)
    {
        stream_image_t* stream_image = (stream_image_t*)stream_data;

        ui_element_t* ui_element = ui_builder->heap_allocate<ui_element_t>();

        // Allocate one UI item for the image
        ui_image_item_t* image_item = ui_builder->heap_allocate<ui_image_item_t>();
        image_item->m_type         = UIItemImage;
        image_item->m_key          = ui_builder->string_allocate("Image");
        image_item->m_key_len      = (unsigned short)strlen(image_item->m_key);
        image_item->m_data_size    = stream_image->m_total_size;
        image_item->m_data         = (unsigned char*)stream_image + sizeof(stream_image_t);

        ui_element->m_count = 1;
        ui_element->m_items = (ui_item_t*)image_item;

        return ui_element;
    }

#ifdef __cplusplus
}
#endif
