#include "../cpp/value_unit.cpp"
#include "../cpp/user_types.cpp"

#include <string>
#include <time.h>

#include "decoder_interface.h"

#ifdef __cplusplus
extern "C"
{
#endif

    // Examples:
    // {"online":1}
    // {"messageId":"","key":0,"mac":"8CCE4E50AF57","voltage":225.415,"current":0,"power":0.102,"energy":0.028}
    // {"messageId":"","mac":"D48AFC3A53DE","type":"Zero-2","version":"2.1.2","wifiLock":0,"keyLock":0,"ip":"192.168.8.93","ssid":"OBNOSIS8","key1":0,"key3":0}

    static inline const char* trim_whitespace(const char* begin, const char* end)
    {
        while (begin < end && (*begin == ' ' || *begin == '\n' || *begin == '\r' || *begin == '\t'))
            begin++;
        return begin;
    }

    struct json_property_t
    {
        const char* key_begin;
        const char* key_end;
        const char* value_begin;
        const char* value_end;
    };

    // GeekOpen packets are JSON, but we don't want to have a full blown JSON parser here, so we do it manually.
    // So in all observed cases they are flat JSON objects with string keys and either string or numeric values.
    static int s_parse_json(const char* json, const char* json_end, json_property_t* properties, int max_properties)
    {
        enum EParseState
        {
            StateExpectObjectStart,
            StateExpectKeyOrObjectEnd,
            StateExpectColon,
            StateExpectValue,
            StateExpectCommaOrObjectEnd,
        };

        EParseState state          = StateExpectObjectStart;
        int         property_count = 0;
        const char* p              = json;

        while (p < json_end)
        {
            switch (state)
            {
                case StateExpectObjectStart:
                {
                    p = trim_whitespace(p, json_end);
                    if (*p == '{')
                    {
                        state = StateExpectKeyOrObjectEnd;
                    }
                    p++;
                    break;
                }

                case StateExpectKeyOrObjectEnd:
                {
                    p = trim_whitespace(p, json_end);
                    if (*p == '}')
                    {
                        return property_count;  // End of object
                    }
                    else if (*p == '"')
                    {
                        // Parse key
                        const char* key_begin = p + 1;
                        p++;
                        while (p < json_end && *p != '"')
                            p++;
                        const char* key_end = p;
                        p++;

                        // Store key
                        if (property_count < max_properties)
                        {
                            properties[property_count].key_begin = key_begin;
                            properties[property_count].key_end   = key_end;
                            state                                = StateExpectColon;
                        }
                        else
                        {
                            return property_count;  // Max properties reached
                        }
                    }
                    else
                    {
                        p++;
                    }
                    break;
                }

                case StateExpectColon:
                {
                    // Consume white space until ':'
                    p = trim_whitespace(p, json_end);
                    if (*p == ':')
                    {
                        state = StateExpectValue;
                    }
                    p++;
                    break;
                }

                case StateExpectValue:
                {
                    p                                      = trim_whitespace(p, json_end);
                    properties[property_count].value_begin = p;
                    if (*p == '"')
                    {
                        p++;  // Skip opening quote
                        while (p < json_end && *p != '"')
                            p++;
                        properties[property_count].value_end = p;
                        p++;  // Skip closing quote
                    }
                    else
                    {
                        while (p < json_end && *p != ',' && *p != '}')
                            p++;
                        properties[property_count].value_end = p;
                    }
                    property_count++;
                    state = StateExpectCommaOrObjectEnd;
                    break;
                }
                case StateExpectCommaOrObjectEnd:
                {
                    p = trim_whitespace(p, json_end);
                    if (*p == ',')
                    {
                        state = StateExpectKeyOrObjectEnd;
                    }
                    else if (*p == '}')
                    {
                        return property_count;  // End of object
                    }
                    p++;
                    break;
                }
            }
        }
        return property_count;
    }

    decoder_ui_element_t* decoder_build_ui_element(decoder_context_t* ctx, const unsigned char* stream_data, unsigned int stream_data_size)
    {
        const int       max_properties = 64;
        json_property_t properties[max_properties];
        int             property_count = s_parse_json((const char*)stream_data, (const char*)stream_data + stream_data_size, properties, max_properties);

        // Setup UI elements
        decoder_ui_element_t* ui_element = ctx->m_ui_heap->_new<decoder_ui_element_t>();
        ui_element->m_count              = property_count;
        ui_element->m_items              = ctx->m_ui_heap->allocate_array<decoder_ui_item_t>(property_count);
        for (int i = 0; i < ui_element->m_count; i++)
        {
            const char* key_begin   = properties[i].key_begin;
            const char* key_end     = properties[i].key_end;
            const char* value_begin = properties[i].value_begin;
            const char* value_end   = properties[i].value_end;

            // Copy key
            const uint32_t key_len = (uint32_t)(key_end - key_begin);
            const char*    key_str = (char*)ctx->m_ui_heap->string_allocate(key_begin, key_len);

            // Copy value
            const uint32_t          value_len = (uint32_t)(value_end - value_begin);
            const char*             value_str = (char*)ctx->m_ui_heap->string_allocate(value_begin, value_len);
            decoder_ui_text_item_t* text_item = (decoder_ui_text_item_t*)&ui_element->m_items[i];
            text_item->m_type                 = UIItemText;
            text_item->m_key_len              = key_len;
            text_item->m_value_len            = value_len;
            text_item->m_key                  = key_str;
            text_item->m_value                = value_str;
        }

        return ui_element;
    }

    void decoder_write_to_stream(decoder_context_t* ctx, const unsigned char* stream_data, unsigned int stream_data_size)
    {
        const int       max_properties = 64;
        json_property_t properties[max_properties];
        int             property_count = 0;

        if (ctx->m_user_context0 == nullptr)
        {
            property_count = s_parse_json((const char*)stream_data, (const char*)stream_data + stream_data_size, properties, max_properties);

            // Is there a MAC address property?
            for (int i = 0; i < property_count; i++)
            {
                const char* key_begin = properties[i].key_begin;
                const char* key_end   = properties[i].key_end;
                uint32_t    key_len   = (uint32_t)(key_end - key_begin);
                if (key_len == 3 && strncmp(key_begin, "mac", 3) == 0)
                {
                    const char* mac_value     = properties[i].value_begin + 1;  // Skip opening quote
                    uint32_t    mac_value_len = (uint32_t)(properties[i].value_end - properties[i].value_begin);
                    uint64_t    user_id       = 0;
                    // Parse MAC address into uint64_t user_id
                    for (uint32_t j = 0; j < mac_value_len; j++)
                    {
                        char c = mac_value[j];
                        user_id <<= 4;
                        if (c >= '0' && c <= '9')
                            user_id |= (uint64_t)(c - '0');
                        else if (c >= 'A' && c <= 'F')
                            user_id |= (uint64_t)(c - 'A' + 10);
                        else if (c >= 'a' && c <= 'f')
                            user_id |= (uint64_t)(c - 'a' + 10);
                    }
                    ctx->m_user_context0 = (void*)user_id;
                    break;
                }
            }
        }

        if (ctx->m_user_context0 != nullptr)
        {
            // For GeekOpen we write the raw JSON data as is to the stream that is coupled with the GeekOpen TCP server.
            uint64_t user_id   = (uint64_t)(ctx->m_user_context0);
            uint64_t data_time = (uint64_t)time(nullptr) * 1000;  // Current time in milliseconds
            ctx->m_stream->write_fix_data((user_id << 16) | ID_JSON, data_time, stream_data, stream_data_size);

            // Here we also parse properties that we are interested in as sensor data.
            // e.g.:
            //   - voltage, current, power, energy
            //   - key/key1/key2/key3 (on/off)
            if (property_count == 0)
            {
                property_count = s_parse_json((const char*)stream_data, (const char*)stream_data + stream_data_size, properties, max_properties);
            }

            for (int i = 0; i < property_count; i++)
            {
                const char* key_begin   = properties[i].key_begin;
                const char* key_end     = properties[i].key_end;
                const char* value_begin = properties[i].value_begin;
                const char* value_end   = properties[i].value_end;

                uint32_t key_len = (uint32_t)(key_end - key_begin);

                // Voltage
                if (key_len == 7 && strncmp(key_begin, "voltage", 7) == 0)
                {
                    float voltage = strtof(value_begin, nullptr);
                    ctx->m_stream->write_f32((user_id << 16) | ID_VOLTAGE, data_time, voltage);
                }
                // Current
                else if (key_len == 7 && strncmp(key_begin, "current", 7) == 0)
                {
                    float current = strtof(value_begin, nullptr);
                    ctx->m_stream->write_f32((user_id << 16) | ID_CURRENT, data_time, current);
                }
                // Power
                else if (key_len == 5 && strncmp(key_begin, "power", 5) == 0)
                {
                    float power = strtof(value_begin, nullptr);
                    ctx->m_stream->write_f32((user_id << 16) | ID_POWER, data_time, power);
                }
                // Energy
                else if (key_len == 6 && strncmp(key_begin, "energy", 6) == 0)
                {
                    double energy = strtod(value_begin, nullptr);
                    ctx->m_stream->write_f64((user_id << 16) | ID_ENERGY, data_time, energy);
                }
                // Key / Key1 / Key2 / Key3
                else
                {
                    // const uint32_t keyn = 0x0079656B; // "key"
                    // const uint32_t key0 = 0x3079656B; // "key0"
                    // const uint32_t key1 = 0x3179656B; // "key1"
                    // const uint32_t key2 = 0x3279656B; // "key2"
                    // const uint32_t key3 = 0x3379656B; // "key3"

                    // Be aware of little endian representation of uint32_t
                    const uint32_t key32 = *(const uint32_t*)key_begin;
                    if ((key32 & 0x00FFFFFF) == 0x0079656B)  // "key", "key0", "key1", "key2", "key3", ...
                    {
                        int switch_index = (key32 >> 24) & 0x0F;  // 0 for "key", 1 for "key1", etc.
                        if (switch_index == 0)
                            switch_index = 1;
                        const uint64_t user_type_switch = (uint64_t)(ID_SWITCH + (switch_index - 1));
                        // Parse value as integer
                        uint8_t key_value = 0;
                        if (value_end > value_begin)
                        {
                            if (*value_begin == '1' || *value_begin == 't' || *value_begin == 'T')
                                key_value = 1;
                        }
                        ctx->m_stream->write_u8((user_id << 16) | user_type_switch, data_time, key_value);
                    }
                }
            }
        }
    }

    void decoder_initialize(decoder_context_t *ctx)
    {

    }


#ifdef __cplusplus
}
#endif
