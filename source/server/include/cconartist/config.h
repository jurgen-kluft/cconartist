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

    struct config_stream_t
    {
        const char* m_name;
        u64         m_index_size;     // unit is MB
        u64         m_data_size;      // unit is MB
        u32         m_max_consumers;  // maximum number of consumers

        const char* m_index_filename;
        const char* m_data_filename;
        const char* m_control_filename;
        const char* m_new_sem_name;
        const char* m_reg_sem_name;

        DCORE_CLASS_PLACEMENT_NEW_DELETE
    };

    struct config_tcp_server_t
    {
        const char*     m_server_name;
        char const*     m_stream_name;
        config_stream_t m_stream_config;
        u16             m_port;

        DCORE_CLASS_PLACEMENT_NEW_DELETE
    };

    struct config_udp_server_t
    {
        const char*     m_server_name;
        char const*     m_stream_name;
        config_stream_t m_stream_config;
        u16             m_port;

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
