#include "cconartist/conman.h"

#include <string.h>

namespace ncore
{
    // Connections are identified by:
    // - type
    // - local_port
    // - remote_port
    // - remote_ip
    static int compare_key(connection_info_t *a, const connection_info_t *b)
    {
        if (a->m_flags[0] < b->m_flags[0])
            return -1;
        else if (a->m_flags[0] > b->m_flags[0])
            return 1;

        if (a->m_local_port < b->m_local_port)
            return -1;
        else if (a->m_local_port > b->m_local_port)
            return 1;

        if (a->m_remote_port < b->m_remote_port)
            return -1;
        else if (a->m_remote_port > b->m_remote_port)
            return 1;

        for (int i = 0; i < 4; i++)
        {
            if (a->m_remote_ip[i] != b->m_remote_ip[i])
            {
                if (a->m_remote_ip[i] < b->m_remote_ip[i])
                    return -1;
                else
                    return 1;
            }
        }

        return 0;
    }

    static ssize_t search_connection(connection_manager_t *mgr, connection_info_t *info, int *found)
    {
        size_t left = 0, right = mgr->m_count;
        *found = 0;
        while (left < right)
        {
            size_t mid = (left + right) / 2;
            int    cmp = compare_key(info, mgr->m_connections[mid]);
            if (cmp == 0)
            {
                *found = 1;
                return mid;
            }
            else if (cmp < 0)
                right = mid;
            else
                left = mid + 1;
        }
        return left;
    }

    int connection_manager_init(connection_manager_t *mgr, size_t initial_capacity)
    {
        mgr->m_connections     = (connection_info_t **)calloc(initial_capacity, sizeof(connection_info_t *));
        mgr->m_free_connection = NULL;
        mgr->m_count           = 0;
        mgr->m_capacity        = initial_capacity;
        uv_mutex_init(&mgr->m_mutex);
        return 0;
    }

    void connection_manager_destroy(connection_manager_t *mgr)
    {
        uv_mutex_lock(&mgr->m_mutex);
        for (int i = 0; i < mgr->m_count; i++)
            free(mgr->m_connections[i]);
        free(mgr->m_connections);
        uv_mutex_unlock(&mgr->m_mutex);
        uv_mutex_destroy(&mgr->m_mutex);
    }

    connection_info_t *connection_manager_alloc(connection_manager_t *mgr)
    {
        uv_mutex_lock(&mgr->m_mutex);
        if (mgr->m_free_connection)
        {
            connection_info_t *info = mgr->m_free_connection;
            mgr->m_free_connection  = NULL;
            uv_mutex_unlock(&mgr->m_mutex);
            return info;
        }
        uv_mutex_unlock(&mgr->m_mutex);
        connection_info_t *info = (connection_info_t *)malloc(sizeof(connection_info_t));
        memset(info, 0, sizeof(connection_info_t));
        return info;
    }

    void connection_manager_release(connection_manager_t *mgr, connection_info_t *info)
    {
        uv_mutex_lock(&mgr->m_mutex);
        if (mgr->m_free_connection)
        {
            free(mgr->m_free_connection);
        }
        mgr->m_free_connection = info;
        uv_mutex_unlock(&mgr->m_mutex);
    }

    connection_info_t *connection_manager_commit(connection_manager_t *mgr, connection_info_t *info)
    {
        uv_mutex_lock(&mgr->m_mutex);
        int     found;
        ssize_t idx = search_connection(mgr, info, &found);
        if (found)
        {
            connection_info_t *existing = mgr->m_connections[idx];
            connection_manager_release(mgr, info);
            existing->set_connected();
            existing->m_last_active = uv_hrtime();
            uv_mutex_unlock(&mgr->m_mutex);
            return existing;
        }
        if (mgr->m_count >= mgr->m_capacity)
        {
            mgr->m_capacity *= 2;
            mgr->m_connections = (connection_info_t **)realloc(mgr->m_connections, mgr->m_capacity * sizeof(connection_info_t *));
        }
        info->m_last_active = uv_hrtime();
        info->set_connected();
        memmove(&mgr->m_connections[idx + 1], &mgr->m_connections[idx], (mgr->m_count - idx) * sizeof(connection_info_t *));
        mgr->m_connections[idx] = info;
        mgr->m_count++;
        uv_mutex_unlock(&mgr->m_mutex);
        return info;
    }

    void connection_manager_mark_disconnected(connection_manager_t *mgr, connection_info_t *info)
    {
        uv_mutex_lock(&mgr->m_mutex);
        int     found;
        ssize_t idx = search_connection(mgr, info, &found);
        if (found)
        {
            mgr->m_connections[idx]->set_disconnected();
            mgr->m_connections[idx]->m_last_active = uv_hrtime();
        }
        uv_mutex_unlock(&mgr->m_mutex);
    }

}  // namespace ncore
