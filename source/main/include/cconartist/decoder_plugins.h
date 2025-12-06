#ifndef __CCONARTIST_DECODER_PLUGINS_H__
#define __CCONARTIST_DECODER_PLUGINS_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
#    pragma once
#endif

#include "clibuv/uv.h"

#include "cconartist/decoder_interface.h"

namespace ncore
{
    namespace nplugins
    {
        struct plugin_t
        {
            char                m_name[256];
            write_to_stream_fn  m_write_to_stream_fn;
            build_ui_element_fn m_build_ui_element_fn;
            void               *m_dlhandle;
        };

        struct registry_t;

        registry_t *create_registry(const char *path_to_folder, u32 max_plugins, uv_loop_t *loop);
        void        destroy_registry(registry_t *registry);
        bool        load_plugin(registry_t *registry, const char *filename);
        bool        unload_plugin(registry_t *registry, const char *filename);
        plugin_t   *find_plugin(registry_t *registry, const char *name);
    }  // namespace nplugins
}  // namespace ncore

#endif  // __CCONARTIST_DECODER_PLUGINS_H__
