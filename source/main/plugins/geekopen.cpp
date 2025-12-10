#include "../cpp/units.cpp"
#include "../cpp/sensor_types.cpp"

#include <string>

#include "decoder_interface.h"

#ifdef __cplusplus
extern "C"
{
#endif

    const char* sGeekOpenPropertyNames[] = {
        "current",
        "energy",
        "ip",
        "key",
        "key1",
        "key2",
        "key3",
        "keyLock",
        "mac",
        "messageId",
        "online",
        "power",
        "ssid",
        "type",
        "version",
        "voltage",
        "wifiLock",
    };

    ui_element_t* build_ui_element(const unsigned char* stream_data, ui_builder_t* ui_builder)
    {
        // GeekOpen packets are JSON, but we don't want to have a full blown JSON parser here, so we do it manually.
        // So in all observed cases they are flat JSON objects with string keys and either string or numeric values.

        // Examples:
        // {"online":1}
        // {"messageId":"","key":0,"mac":"8CCE4E50AF57","voltage":225.415,"current":0,"power":0.102,"energy":0.028}
        // {"messageId":"","mac":"D48AFC3A53DE","type":"Zero-2","version":"2.1.2","wifiLock":0,"keyLock":0,"ip":"192.168.8.93","ssid":"OBNOSIS8","key1":0,"key3":0}

        // To reduce stream data size, we can do the following:
        // - Prebuild property name dictionary and we write out an 8 bit index instead of the full string
        //   - We do need a full dictionary
        // - Certain property values are converted to integer representation instead of string
        //   - e.g. boolean values (0/1), integer values (e.g. voltage in 0.1V steps), IP address (4 bytes), MAC address (6 bytes)
        // - Version strings can be converted to a 32 bit integer representation (e.g. major.minor.patch.build -> 8 bits each)
        // - SSID strings can be stored as is, but limited to 32 bytes!

        stream_json_t* json_data = (stream_json_t*)stream_data;

        const unsigned char* ptr = stream_data + sizeof(stream_json_t);
        const unsigned char* end = stream_data + sizeof(stream_json_t) + json_data->m_json_size;

        while (ptr < end && (*ptr == ' ' || *ptr == '{'))
            ptr++;

        const int   max_properties = 32;
        const char* key_begin[max_properties];
        const char* key_end[max_properties];
        const char* value_begin[max_properties];
        const char* value_end[max_properties];
        int         value_type[max_properties];  // 0 = string, 1 = integer, 2 = double, 3 = boolean, 4 = mac, 5 = ip, 6 = ssid, 7 = version, 8 = type

        short count = 0;
        while (ptr < end && *ptr != '}' && count < max_properties)
        {
            // Skip whitespace and quotes
            while (ptr < end && (*ptr == ' ' || *ptr == '\"'))
                ptr++;

            // Parse key
            const unsigned char* key_start = ptr;
            while (ptr < end && *ptr != '\"')
                ptr++;
            uint32_t key_len = (uint32_t)(ptr - key_start);

            key_begin[count] = (const char*)key_start;
            key_end[count]   = (const char*)(key_start + key_len);

            while (ptr < end && *ptr != ':')
                ptr++;
            ptr++;  // skip ':'

            while (ptr < end && (*ptr == ' '))
                ptr++;

            // Parse value
            int         value_type_id = 0;  // default to string
            const char* val_str_begin = nullptr;
            const char* val_str_end   = nullptr;

            if (*ptr == '\"')
            {
                // String value
                ptr++;
                const unsigned char* value_start = ptr;
                while (ptr < end && *ptr != '\"')
                    ptr++;
                uint32_t value_len = (uint32_t)(ptr - value_start);

                val_str_begin = (const char*)value_start;
                val_str_end   = (const char*)(value_start + value_len);

                ptr++;  // skip closing quote
            }
            else
            {
                // Numeric or boolean
                const unsigned char* value_start = ptr;
                while (ptr < end && *ptr != ',' && *ptr != '}')
                    ptr++;
                uint32_t value_len = (uint32_t)(ptr - value_start);

                val_str_begin = (const char*)value_start;
                val_str_end   = (const char*)(value_start + value_len);

                // Determine if boolean, integer or double
                if (value_len == 1 && (val_str_begin[0] == '0' || val_str_begin[0] == '1'))
                {
                    value_type_id = 3;  // boolean
                }
                else
                {
                    bool is_double = false;
                    for (uint32_t i = 0; i < value_len; i++)
                    {
                        if (val_str_begin[i] == '.')
                        {
                            is_double = true;
                            break;
                        }
                    }
                    value_type_id = is_double ? 2 : 1;  // double or integer
                }
            }

            value_begin[count] = val_str_begin;
            value_end[count]   = val_str_end;
            value_type[count]  = value_type_id;

            count++;

            // Skip comma
            while (ptr < end && (*ptr == ',' || *ptr == ' '))
                ptr++;
        }

        // Setup UI elements
        ui_element_t* ui_element = ui_builder->heap_allocate<ui_element_t>();
        ui_element->m_count      = count;
        ui_element->m_items      = ui_builder->heap_allocate_array<ui_item_t>(count);
        for (int i = 0; i < ui_element->m_count; i++)
        {
            // Copy key
            const uint32_t key_len = (uint32_t)(key_end[i] - key_begin[i]);
            const char*    key_str = (char*)ui_builder->string_allocate(key_begin[i], key_len);

            // Copy value
            const uint32_t value_len = (uint32_t)(value_end[i] - value_begin[i]);
            const char*    value_str = (char*)ui_builder->string_allocate(value_begin[i], value_len);

            ui_text_item_t* text_item = (ui_text_item_t*)&ui_element->m_items[i];
            text_item->m_type         = UIItemText;
            text_item->m_key_len      = key_len;
            text_item->m_value_len    = value_len;
            text_item->m_key          = key_str;
            text_item->m_value        = value_str;
        }

        return ui_element;
    }

    void* write_to_stream(connection_context_t* ctx, const unsigned char* stream_data, unsigned int packet_size)
    {
        // For GeekOpen we write the raw JSON data as is to the stream that is coupled with the GeekOpen TCP server.
        unsigned char* stream_data_ptr = ctx->m_connection_interface->stream_allocate(sizeof(stream_json_t) + packet_size);
        if (stream_data_ptr == nullptr)
            return nullptr;
        stream_json_t* json_data = (stream_json_t*)stream_data_ptr;
        json_data->m_json_size   = packet_size;
        memcpy(stream_data_ptr + sizeof(stream_json_t), stream_data, packet_size);
        return stream_data_ptr;
    }

#ifdef __cplusplus
}
#endif
