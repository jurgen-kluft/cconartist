#include "../cpp/value_unit.cpp"
#include "../cpp/user_types.cpp"

#include <string>
#include <ctime>

#include "decoder_interface.h"

#ifdef __cplusplus
extern "C"
{
#endif

    struct sensor_packet_t
    {
        unsigned char m_length;   // packet length = length * 2
        unsigned char m_version;  // packet version
        unsigned char m_mac[6];   // device MAC address
    };

    struct sensor_value_t
    {
        unsigned char m_type;  // EUserType
        unsigned char m_h;     // high byte of value
        unsigned char m_l;     // low byte of value
    };

    decoder_ui_element_t* decoder_build_ui_element(decoder_context_t* ctx, uint64_t user_id, uint16_t user_type, const unsigned char* stream_data, unsigned int stream_data_size)
    {
        sensor_packet_t const* packet     = (sensor_packet_t const*)stream_data;
        const int              length     = packet->m_length * 2;
        const int              num_values = (length - sizeof(sensor_packet_t)) / sizeof(sensor_value_t);

        // Create UI element for this received sensor packet
        decoder_ui_element_t* ui_element = ctx->m_ui_heap->_new<decoder_ui_element_t>();
        ui_element->m_count              = num_values + 3;  // +3 for Length, Version and MAC address
        decoder_ui_item_t* ui_items      = ctx->m_ui_heap->allocate_array<decoder_ui_item_t>(ui_element->m_count);
        ui_element->m_items              = ui_items;

        int item_index = 0;

        // First UI item is the packet length
        decoder_ui_text_item_t* text_item_0 = (decoder_ui_text_item_t*)&ui_element->m_items[item_index++];
        text_item_0->m_type                 = UIItemText;
        text_item_0->m_key                  = ctx->m_ui_heap->string_allocate("Packet Length", 13);
        text_item_0->m_key_len              = strlen(text_item_0->m_key);
        char* length_string                 = ctx->m_ui_heap->string_allocate("", 12);
        int   value_len                     = snprintf(length_string, 12, "%d bytes", length);
        text_item_0->m_value                = length_string;
        text_item_0->m_value_len            = value_len;

        // Second UI item is the packet version
        decoder_ui_text_item_t* text_item_1 = (decoder_ui_text_item_t*)&ui_element->m_items[item_index++];
        text_item_1->m_type                 = UIItemText;
        text_item_1->m_key                  = ctx->m_ui_heap->string_allocate("Packet Version", 14);
        text_item_1->m_key_len              = strlen(text_item_1->m_key);
        char* version_string                = (ctx->m_ui_heap->string_allocate("", 5));
        int   version_value_len             = snprintf(version_string, 5, "%d", packet->m_version);
        text_item_1->m_value                = version_string;
        text_item_1->m_value_len            = version_value_len;

        // Third UI item is the MAC address
        decoder_ui_text_item_t* text_item_2 = (decoder_ui_text_item_t*)&ui_element->m_items[item_index++];
        text_item_2->m_type                 = UIItemText;
        text_item_2->m_key                  = ctx->m_ui_heap->string_allocate("Device MAC Address", 18);
        text_item_2->m_key_len              = strlen(text_item_2->m_key);
        char*                mac_string     = (ctx->m_ui_heap->string_allocate("", 18));  // XX:XX:XX:XX:XX:XX = 17 chars + null
        const unsigned char* mac            = (const unsigned char*)packet->m_mac;
        int                  mac_value_len  = snprintf(mac_string, 18, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        text_item_2->m_value                = mac_string;
        text_item_2->m_value_len            = mac_value_len;

        sensor_value_t const* value = (sensor_value_t const*)(stream_data + sizeof(sensor_packet_t));
        for (int i = 0; i < num_values; ++i)
        {
            short sensor_scalar = 1;
            // short sensor_unit   = 0;  // EUnit
            int sensor_value = ((int)value->m_h << 8) | (int)value->m_l;

            // Process sensor value based on its type to determine scalar
            switch (value->m_type)
            {
                // Add more sensor types as needed
                default:
                    // Unknown sensor type
                    break;
            }

            nunit::enum_t sensor_unit = get_value_unit((nusertype::enum_t)value->m_type);
            const char*   unit_string = nunit::to_string(sensor_unit);

            // Initialization of a UI item for this sensor value
            char temporary_string[64];
            int  value_len = 0;
            if (sensor_scalar != 1)
            {
                float value_scaled = (float)sensor_value / (float)sensor_scalar;
                value_len          = snprintf(temporary_string, sizeof(temporary_string), "%.3f %s", value_scaled, unit_string);
            }
            else
            {
                value_len = snprintf(temporary_string, sizeof(temporary_string), "%d %s", sensor_value, unit_string);
            }
            char* value_string = ctx->m_ui_heap->string_allocate("", value_len);
            strncpy(value_string, temporary_string, value_len + 1);

            decoder_ui_text_item_t* text_item = (decoder_ui_text_item_t*)&ui_element->m_items[item_index++];
            text_item->m_type                 = UIItemText;
            text_item->m_value                = value_string;
            text_item->m_value_len            = value_len;
            text_item->m_key                  = to_key_string((nusertype::enum_t)value->m_type);
            text_item->m_key_len              = strlen(text_item->m_key);

            ++value;
        }
        return ui_element;
    }

    inline uint64_t make_id(uint8_t const* mac, uint8_t stream_type, uint8_t user_type)
    {
        return ((uint64_t)mac[0] << 40) | ((uint64_t)mac[1] << 32) | ((uint64_t)mac[2] << 24) | ((uint64_t)mac[3] << 16) | ((uint64_t)mac[4] << 8) | ((uint64_t)mac[5]) | ((uint64_t)stream_type << 48) | ((uint64_t)user_type << 56);
    }

    void decoder_write_to_stream(decoder_context_t* ctx, const unsigned char* packet_data, unsigned int packet_size)
    {
        const uint64_t current_time = static_cast<uint64_t>(std::time(nullptr));

        sensor_packet_t const* packet = (sensor_packet_t const*)packet_data;
        const unsigned char*   end    = packet_data + packet_size;

        // Write to the full packet stream
        const uint32_t sensor_hid = 0x00000000;  // Use a fixed HID for the full sensor packet
        const uint16_t sensor_lid = 0xFFFF;      // Use a fixed LID for the full sensor packet
        ncore::stream_id_t stream_id_var = ctx->m_stream->register_stream(sensor_hid, sensor_lid, nstreamtype::TypeVariable, nusertype::ID_SENSOR);
        ctx->m_stream->write_var_data(stream_id_var, current_time, packet_data, packet_size);

        const uint8_t* mac = packet->m_mac;
        const uint32_t hid = ((uint32_t)mac[0]) | ((uint32_t)mac[1] << 8) | ((uint32_t)mac[2] << 16) | ((uint16_t)mac[3] << 24);
        const uint16_t lid = ((uint16_t)mac[4] << 8) | ((uint16_t)mac[5]);

        // Write individual sensor values
        // Sensor values can only be U8, U16, S8 or S16, since all incoming values are 2 bytes
        sensor_value_t const* value = (sensor_value_t const*)(packet_data + sizeof(sensor_packet_t));
        while (((const unsigned char*)value + sizeof(sensor_value_t)) <= end)
        {
            const uint8_t      user_type   = (uint8_t)value->m_type;
            const uint8_t      stream_type = get_stream_type(nusertype::enum_t(user_type));
            ncore::stream_id_t stream_id   = ctx->m_stream->register_stream(hid, lid, stream_type, user_type);
            switch (stream_type)
            {
                case nstreamtype::TypeU8: ctx->m_stream->write_u8(stream_id, current_time, value->m_l); break;
                case nstreamtype::TypeU16:
                    const uint16_t sensor_value = ((uint16_t)value->m_h << 8) | (uint16_t)value->m_l;
                    ctx->m_stream->write_u16(stream_id, current_time, sensor_value);
                    break;
                case nstreamtype::TypeS8:
                    const int8_t sensor_value = (int8_t)value->m_l;
                    ctx->m_stream->write_s8(stream_id, current_time, (int8_t)sensor_value);
                    break;
                case nstreamtype::TypeS16:
                    const int16_t sensor_value = ((int16_t)value->m_h << 8) | (int16_t)value->m_l;
                    ctx->m_stream->write_s16(stream_id, current_time, (int16_t)sensor_value);
                    break;
                default:
                    // Unsupported stream type for this sensor value
                    break;
            }
            ++value;
        }
    }

    void decoder_initialize(decoder_context_t* ctx) {}

#ifdef __cplusplus
}
#endif
