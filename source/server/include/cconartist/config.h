#ifndef __CCONARTIST_CONFIG_H__
#define __CCONARTIST_CONFIG_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
#    pragma once
#endif

#include "ccore/c_allocator.h"

namespace ncore
{
    class alloc_t;

    struct config_tcp_server_t
    {
        const char* m_server_name;
        char const* m_stream_name;
        u16         m_port;

        DCORE_CLASS_PLACEMENT_NEW_DELETE
    };

    struct config_udp_server_t
    {
        const char* m_server_name;
        char const* m_stream_name;
        u16         m_port;

        DCORE_CLASS_PLACEMENT_NEW_DELETE
    };

    struct config_stream_t
    {
        const char* m_name;
        u64         m_user_id;
        u8          m_user_type;    // enum EUserType
        u8          m_stream_type;  // type stream_type_t
        u32         m_data_size;

        DCORE_CLASS_PLACEMENT_NEW_DELETE
    };

    struct config_main_t
    {
        i32                  m_num_tcp_servers;
        i32                  m_num_udp_servers;
        config_tcp_server_t* m_tcp_servers;
        config_udp_server_t* m_udp_servers;
        u16                  m_discovery_port;

        DCORE_CLASS_PLACEMENT_NEW_DELETE
    };

    config_main_t* g_load_config(alloc_t* allocator);

}  // namespace ncore

#endif  // __CCONARTIST_CONFIG_H__
