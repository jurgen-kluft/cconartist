#include "ccore/c_target.h"
#include "ccore/c_allocator.h"
#include "ccore/c_memory.h"

#include "cjson/c_json_allocator.h"
#include "cjson/c_json_decoder.h"

#include "cconartist/stream_manager.h"

#include "cconartist/user_types.h"
#include "cconartist/config.h"

#include <stdio.h>
#include <stdlib.h>

namespace ncore
{
    static void decode_config_tcp_server(njson::ndecoder::decoder_t* d, config_tcp_server_t* obj)
    {
        njson::ndecoder::result_t result = njson::ndecoder::read_object_begin(d);
        if (njson::ndecoder::NotOk(result))
            return;

        njson::ndecoder::register_member(d, "name", &obj->m_server_name);
        njson::ndecoder::register_member(d, "port", &obj->m_port);
        njson::ndecoder::register_member(d, "stream", &obj->m_stream_name);
        while (njson::ndecoder::OkAndNotEnded(result))
        {
            njson::ndecoder::field_t field = njson::ndecoder::decode_field(d);
            njson::ndecoder::decoder_decode_member(d, field);
            result = njson::ndecoder::read_object_end(d);
        }
    }

    static void decode_config_udp_server(njson::ndecoder::decoder_t* d, config_udp_server_t* obj)
    {
        njson::ndecoder::result_t result = njson::ndecoder::read_object_begin(d);
        if (njson::ndecoder::NotOk(result))
            return;

        njson::ndecoder::register_member(d, "name", &obj->m_server_name);
        njson::ndecoder::register_member(d, "port", &obj->m_port);
        njson::ndecoder::register_member(d, "stream", &obj->m_stream_name);
        while (njson::ndecoder::OkAndNotEnded(result))
        {
            njson::ndecoder::field_t field = njson::ndecoder::decode_field(d);
            njson::ndecoder::decoder_decode_member(d, field);
            result = njson::ndecoder::read_object_end(d);
        }
    }

    template <typename T>
    void decode_object_array(njson::ndecoder::decoder_t* d, T*& out_array, i32& out_array_size, void (*decode_object)(njson::ndecoder::decoder_t*, T*))
    {
        out_array_size                   = 0;
        out_array                        = nullptr;
        njson::ndecoder::result_t result = njson::ndecoder::read_array_begin(d, out_array_size);
        if (njson::ndecoder::OkAndNotEnded(result))
        {
            out_array = d->m_DecoderAllocator->AllocateArray<T>(out_array_size);
            nmem::memset(out_array, 0, sizeof(T) * out_array_size);
            i32 array_index = 0;
            while (njson::ndecoder::OkAndNotEnded(result))
            {
                if (array_index < out_array_size)
                    decode_object(d, &out_array[array_index]);
                array_index++;
                result = njson::ndecoder::read_array_end(d);
            }
        }
    }
    template <typename T>
    void decode_object_array(njson::ndecoder::decoder_t* d, T*& out_array, i16& out_array_size, void (*decode_object)(njson::ndecoder::decoder_t*, T*))
    {
        i32 array_size;
        decode_object_array<T>(d, out_array, array_size, decode_object);
        out_array_size = (i16)array_size;
    }

    static void decode_config_main(njson::ndecoder::decoder_t* d, config_main_t* obj)
    {
        njson::ndecoder::result_t result = njson::ndecoder::read_object_begin(d);
        if (njson::ndecoder::NotOk(result))
            return;

        njson::ndecoder::register_member(d, "discovery-port", &obj->m_discovery_port);

        while (njson::ndecoder::OkAndNotEnded(result))
        {
            njson::ndecoder::field_t field = njson::ndecoder::decode_field(d);
            if (njson::ndecoder::field_equal(field, "tcp-servers"))
            {
                decode_object_array<config_tcp_server_t>(d, obj->m_tcp_servers, obj->m_num_tcp_servers, decode_config_tcp_server);
            }
            else if (njson::ndecoder::field_equal(field, "udp-servers"))
            {
                decode_object_array<config_udp_server_t>(d, obj->m_udp_servers, obj->m_num_udp_servers, decode_config_udp_server);
            }
            else
            {
                njson::ndecoder::decoder_decode_member(d, field);
            }
            result = njson::ndecoder::read_object_end(d);
        }
    }

    config_main_t* g_load_config(alloc_t* allocator)
    {
        // Load configuration from 'config.json' file
        // File IO
        size_t filesize = 0;
        FILE*  f        = fopen("config.json", "rb");
        if (!f)
        {
            return nullptr;
        }
        fseek(f, 0, SEEK_END);
        filesize = ftell(f);
        fseek(f, 0, SEEK_SET);
        unsigned int   json_content_len = (unsigned int)filesize;
        unsigned char* json_content     = (unsigned char*)malloc(filesize + 1);
        fread(json_content, 1, filesize, f);
        fclose(f);
        json_content[filesize] = 0;

        ncore::njson::JsonAllocator stack_allocator;
        ncore::njson::JsonAllocator decoder_allocator;
        stack_allocator.Init(allocator, 1024 * 16, "JsonDecoderStack");
        decoder_allocator.Init(allocator, filesize * 2, "JsonDecoderMain");

        config_main_t*              config  = nullptr;
        njson::ndecoder::decoder_t* decoder = njson::ndecoder::create_decoder(&stack_allocator, &decoder_allocator, (const char*)json_content, (const char*)json_content + json_content_len);
        if (decoder != nullptr)
        {
            config = g_construct<config_main_t>(allocator);
            decode_config_main(decoder, config);
            njson::ndecoder::destroy_decoder(decoder);
        }
        stack_allocator.Destroy();
        return config;
    }

}  // namespace ncore
