#include "cconartist/decoder_plugins.h"

#include <dlfcn.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

namespace ncore
{
    namespace nplugins
    {
        struct registry_t
        {
            plugin_t     *m_plugins;
            u32           m_plugin_count;
            u32           m_plugin_capacity;
            char          m_folder[256];
            uv_loop_t    *m_loop;
            uv_fs_event_t m_fs_event;
        };

        static bool has_so_extension(const char *filename)
        {
            const char *ext = strrchr(filename, '.');
            return (ext && strcmp(ext, ".dylib") == 0);
        }

        static void on_fs_event(uv_fs_event_t *handle, const char *filename, int events, int status)
        {
            registry_t *reg = (registry_t *)handle->data;
            if (status < 0 || !filename)
                return;

            if (has_so_extension(filename) && (events & UV_CHANGE))
            {
                fprintf(stdout, "[Registry] Detected change in %s\n", filename);
                load_plugin(reg, filename);
            }
        }

        registry_t *create_registry(const char *path_to_folder, u32 max_plugins, uv_loop_t *loop)
        {
            registry_t *reg = (registry_t *)malloc(sizeof(registry_t));
            memset(reg, 0, sizeof(registry_t));

            reg->m_plugins         = (plugin_t *)malloc(sizeof(plugin_t) * max_plugins);
            reg->m_plugin_capacity = max_plugins;
            reg->m_plugin_count    = 0;
            strncpy(reg->m_folder, path_to_folder, sizeof(reg->m_folder) - 1);
            reg->m_loop = loop;

            uv_fs_event_init(loop, &reg->m_fs_event);
            reg->m_fs_event.data = reg;
            uv_fs_event_start(&reg->m_fs_event, on_fs_event, reg->m_folder, 0);

            fprintf(stdout, "[Registry] Watching folder: %s\n", reg->m_folder);

            // Initial manual load of plugins
            DIR *dir = opendir(path_to_folder);
            if (dir)
            {
                struct dirent *entry;
                while ((entry = readdir(dir)) != nullptr)
                {
                    if (has_so_extension(entry->d_name))
                    {
                        load_plugin(reg, entry->d_name);
                    }
                }
                closedir(dir);
            }

            return reg;
        }

        void destroy_registry(registry_t *registry)
        {
            uv_fs_event_stop(&registry->m_fs_event);
            for (u32 i = 0; i < registry->m_plugin_count; ++i)
            {
                dlclose(registry->m_plugins[i].m_dlhandle);
            }
            free(registry->m_plugins);
            free(registry);
        }

        bool load_plugin(registry_t *registry, const char *filename)
        {
            if (registry->m_plugin_count >= registry->m_plugin_capacity)
                return false;

            plugin_t *p = nullptr;

            // Check if already loaded before
            for (u32 i = 0; i < registry->m_plugin_count; ++i)
            {
                if (strcmp(registry->m_plugins[i].m_name, filename) == 0)
                {
                    fprintf(stdout, "[Registry] Plugin will be reloaded: %s\n", filename);
                    dlclose(registry->m_plugins[i].m_dlhandle);
                    p = &registry->m_plugins[i];
                    break;
                }
            }

            // TODO, this needs to be more robust, use temporary allocator ?
            char fullpath[512];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", registry->m_folder, filename);

            void *handle = dlopen(fullpath, RTLD_NOW);
            if (!handle)
            {
                fprintf(stderr, "[Registry] Failed to %sload %s\n", (p == nullptr) ? "" : "re", fullpath);
                return false;
            }

            decoder_initialize_fn       initialize       = (decoder_initialize_fn)dlsym(handle, "decoder_initialize");
            decoder_write_to_stream_fn  write_to_stream  = (decoder_write_to_stream_fn)dlsym(handle, "decoder_write_to_stream");
            decoder_build_ui_element_fn build_ui_element = (decoder_build_ui_element_fn)dlsym(handle, "decoder_build_ui_element");
            if (!write_to_stream || !build_ui_element)
            {
                fprintf(stderr, "[Registry] Invalid plugin: %s\n", filename);
                dlclose(handle);
                return false;
            }

            fprintf(stdout, "[Registry] %soaded plugin %s\n", (p == nullptr) ? "L" : "Rel", filename);

            if (p == nullptr)
            {
                p = &registry->m_plugins[registry->m_plugin_count++];
                memset(p, 0, sizeof(plugin_t));
                strncpy(p->m_name, filename, sizeof(p->m_name) - 1);
            }

            p->m_initialize_fn       = initialize;
            p->m_write_to_stream_fn  = write_to_stream;
            p->m_build_ui_element_fn = build_ui_element;
            p->m_dlhandle            = handle;

            return true;
        }

        bool unload_plugin(registry_t *registry, const char *name)
        {
            for (u32 i = 0; i < registry->m_plugin_count; ++i)
            {
                if (strcmp(registry->m_plugins[i].m_name, name) == 0)
                {
                    dlclose(registry->m_plugins[i].m_dlhandle);
                    // Swap remove the plugin
                    registry->m_plugins[i] = registry->m_plugins[registry->m_plugin_count - 1];
                    registry->m_plugin_count--;
                    fprintf(stdout, "[Registry] Unloaded plugin %s\n", name);
                    return true;
                }
            }
            return false;
        }

        plugin_t *find_plugin(registry_t *registry, const char *name)
        {
            for (u32 i = 0; i < registry->m_plugin_count; ++i)
            {
                if (strcmp(registry->m_plugins[i].m_name, name) == 0)
                {
                    return &registry->m_plugins[i];
                }
            }
            return nullptr;
        }

        void        update_registry(registry_t *registry, f64 now)
        {
            // Currently nothing to do here, as we use fs_event to monitor changes
        }

    }  // namespace nplugins
}  // namespace ncore
