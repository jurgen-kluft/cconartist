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

    enum EServerType
    {
        cServerTypeTcp = 'T',
        cServerTypeUdp = 'U',
    };

    struct config_server_t
    {
        const char*  m_name;
        u8           m_type;
        u16          m_port;
        ncore::s32   m_nb_decoders;  // number of decoders
        const char** m_decoders;     // array of decoder names

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
        i32              m_num_servers;
        i32              m_num_streams;
        config_server_t* m_servers;
        config_stream_t* m_streams;

        DCORE_CLASS_PLACEMENT_NEW_DELETE
    };

    config_main_t* g_load_config(alloc_t* allocator);

}  // namespace ncore

#endif  // __CCONARTIST_CONFIG_H__
