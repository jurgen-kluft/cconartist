#include "ccore/c_target.h"
#include "ccore/c_allocator.h"

#include "cconartist/packet_pool.h"

#include "cunittest/cunittest.h"

using namespace ncore;

UNITTEST_SUITE_BEGIN(packet_pool)
{
    UNITTEST_FIXTURE(basic)
    {
        UNITTEST_ALLOCATOR;

        UNITTEST_FIXTURE_SETUP() {}
        UNITTEST_FIXTURE_TEARDOWN() {}

        UNITTEST_TEST(initialize)
        {
            packet_pool_t* pool = packet_pool_create(Allocator, 10, 256);

            packet_pool_destroy(pool);
        }

        UNITTEST_TEST(acquire_release)
        {
            packet_pool_t* pool = packet_pool_create(Allocator, 10, 256);

            packet_t* packet = packet_acquire(pool);
            CHECK_NOT_NULL(packet);
            CHECK_EQUAL((u32)256, packet->m_data_capacity);

            packet_release(pool, packet);

            packet_pool_destroy(pool);
        }

        UNITTEST_TEST(acquire_release_maximum)
        {
            packet_pool_t* pool = packet_pool_create(Allocator, 10, 256);

            packet_t* packets[10];
            for (int i = 0; i < 10; ++i)
            {
                packets[i] = packet_acquire(pool);
                CHECK_EQUAL((u32)256, packets[i]->m_data_capacity);
                CHECK_NOT_NULL(packets[i]);
            }

            packet_t* packet = packet_acquire(pool);
            CHECK_NULL(packet);

            for (int i = 0; i < 10; ++i)
            {
                packet_release(pool, packets[i]);
            }

            packet_pool_destroy(pool);
        }
    }
}
UNITTEST_SUITE_END
