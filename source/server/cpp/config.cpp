#include "ccore/c_target.h"
#include "ccore/c_allocator.h"

#include "cjson/c_json.h"
#include "cjson/c_json_allocator.h"
#include "cjson/c_json_decode.h"
#include "cjson/c_json_encode.h"

#include "cconartist/streamman.h"

#include "cconartist/config.h"

#include <stdio.h>
#include <stdlib.h>

namespace ncore
{
    static const char*            server_type_enum_strs[]   = {"tcp", "udp"};
    static const u64              server_type_enum_values[] = {1, 2};
    static njson::JsonEnumTypeDef json_server_type_enum("server_type_enum", sizeof(u16), alignof(u16), server_type_enum_strs, server_type_enum_values);

    static const char*            stream_type_enum_strs[]   = {"invalid", "u8", "u16", "u32", "f32", "fixed", "variable"};
    static const u64              stream_type_enum_values[] = {TypeInvalid, TypeU8, TypeU16, TypeU32, TypeF32, TypeFixed, TypeVariable};
    static njson::JsonEnumTypeDef json_stream_type_enum("stream_type_enum", sizeof(u16), alignof(u16), stream_type_enum_strs, stream_type_enum_values);

    template <>
    void njson::JsonObjectTypeRegisterFields<config_server_t>(config_server_t& base, njson::JsonFieldDescr*& members, s32& member_count)
    {
        // clang-format off
        static njson::JsonFieldDescr s_members[] = {
            njson::JsonFieldDescr("name", base.m_name),
            njson::JsonFieldDescr("type", base.m_type, json_server_type_enum),
            njson::JsonFieldDescr("port", base.m_port),
            njson::JsonFieldDescr("encoders", base.m_encoders, base.m_nb_encoders),
        };
        // clang-format on
        members      = s_members;
        member_count = sizeof(s_members) / sizeof(njson::JsonFieldDescr);
    }
    static njson::JsonObjectTypeDeclr<config_server_t> json_config_server("config_server");

    template <>
    void njson::JsonObjectTypeRegisterFields<config_stream_t>(config_stream_t& base, njson::JsonFieldDescr*& members, s32& member_count)
    {
        // clang-format off
        static njson::JsonFieldDescr s_members[] = {
            njson::JsonFieldDescr("name", base.m_name),
            njson::JsonFieldDescr("user_id", base.m_user_id),
            njson::JsonFieldDescr("user_type", base.m_user_type, json_server_type_enum),
            njson::JsonFieldDescr("stream_type", base.m_stream_type, json_stream_type_enum),
            njson::JsonFieldDescr("data_size", base.m_data_size),
        };
        // clang-format on
        members      = s_members;
        member_count = sizeof(s_members) / sizeof(njson::JsonFieldDescr);
    }
    static njson::JsonObjectTypeDeclr<config_stream_t> json_config_stream("config_stream");

    template <>
    void njson::JsonObjectTypeRegisterFields<config_main_t>(config_main_t& base, njson::JsonFieldDescr*& members, s32& member_count)
    {
        // clang-format off
        static njson::JsonFieldDescr s_members[] = {
            njson::JsonFieldDescr("servers", base.m_servers, base.m_num_servers, json_config_server),
            njson::JsonFieldDescr("streams", base.m_streams, base.m_num_streams, json_config_stream),
        };
        // clang-format on
        members      = s_members;
        member_count = sizeof(s_members) / sizeof(njson::JsonFieldDescr);
    }
    static njson::JsonObjectTypeDeclr<config_main_t> json_config_main("config_main");

    config_main_t* g_load_config(alloc_t* allocator)
    {
        config_main_t* config = g_construct<config_main_t>(allocator);

        // Load configuration from 'config.json' file

        // File IO
        size_t filesize = 0;
        FILE*  f        = fopen("config.json", "rb");
        if (!f)
        {
            g_destruct(allocator, config);
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

        njson::JsonObject json_root;
        json_root.m_descr    = &json_config_main;
        json_root.m_instance = config;

        njson::JsonAllocator alloc;
        alloc.Init(allocator, json_content_len * 2, "json allocator");
        njson::JsonAllocator scratch;
        scratch.Init(allocator, 64 * 1024, "json scratch allocator");

        char const* error_message = nullptr;
        if (!njson::JsonDecode((const char*)json_content, (const char*)json_content + json_content_len, json_root, &alloc, &scratch, error_message))
        {
            g_destruct(allocator, config);
            config = nullptr;
        }

        alloc.Destroy();
        scratch.Destroy();

        return config;
    }

}  // namespace ncore
