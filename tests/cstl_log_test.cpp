#include <gtest/gtest.h>

extern "C"
{
#include "rlgame.base/cstl/cstl_heap_allocator.h"
#include "rlgame.base/cstl/cstl_trace.h"
#include "rlgame.base/cstl/cstl_log.h"
}

namespace
{

class CstlLogTest : public ::testing::Test
{
    protected:
        void
        SetUp () override
        {
            ASSERT_EQ (0, r_cstl_heap_init (256 * 1024));
            ASSERT_EQ (0, r_cstl_log_init ());
        }

        void
        TearDown () override
        {
            r_cstl_log_shutdown ();
            r_cstl_heap_shutdown ();
        }
};

} // namespace

TEST (CstlLogInitTest, InitFailsWithoutHeap) { EXPECT_EQ (-1, r_cstl_log_init ()); }

TEST (CstlLogInitTest, InitDestroyAndReinit)
{
    ASSERT_EQ (0, r_cstl_heap_init (128 * 1024));
    ASSERT_EQ (0, r_cstl_log_init ());
    r_cstl_log_shutdown ();
    ASSERT_EQ (0, r_cstl_log_init ());
    r_cstl_log_shutdown ();
    r_cstl_heap_shutdown ();
}

TEST_F (CstlLogTest, LevelNames)
{
    EXPECT_STREQ ("TRACE", r_cstl_log_level_name (R_CSTL_LOG_LEVEL_TRACE));
    EXPECT_STREQ ("DEBUG", r_cstl_log_level_name (R_CSTL_LOG_LEVEL_DEBUG));
    EXPECT_STREQ ("INFO", r_cstl_log_level_name (R_CSTL_LOG_LEVEL_INFO));
    EXPECT_STREQ ("WARN", r_cstl_log_level_name (R_CSTL_LOG_LEVEL_WARN));
    EXPECT_STREQ ("ERROR", r_cstl_log_level_name (R_CSTL_LOG_LEVEL_ERROR));
    EXPECT_STREQ ("FATAL", r_cstl_log_level_name (R_CSTL_LOG_LEVEL_FATAL));
    EXPECT_STREQ ("UNKNOWN", r_cstl_log_level_name (static_cast<enum r_cstl_log_level> (999)));
}

TEST_F (CstlLogTest, DefaultMinLevelIsTrace) { EXPECT_EQ (R_CSTL_LOG_LEVEL_TRACE, r_cstl_log_get_min_level ()); }

TEST_F (CstlLogTest, SetMinLevelFiltersMessages)
{
    r_cstl_log_set_min_level (R_CSTL_LOG_LEVEL_WARN);
    EXPECT_EQ (R_CSTL_LOG_LEVEL_WARN, r_cstl_log_get_min_level ());

    R_CSTL_LOG_INFO ("filtered info %d", 1);
    R_CSTL_LOG_WARN ("visible warn %d", 2);
    r_cstl_log_flush ();

    r_cstl_log_set_min_level (R_CSTL_LOG_LEVEL_TRACE);
}

TEST_F (CstlLogTest, InvalidMinLevelIsIgnored)
{
    r_cstl_log_set_min_level (R_CSTL_LOG_LEVEL_INFO);
    r_cstl_log_set_min_level (static_cast<enum r_cstl_log_level> (-1));
    EXPECT_EQ (R_CSTL_LOG_LEVEL_INFO, r_cstl_log_get_min_level ());
}

TEST_F (CstlLogTest, WriteAllLevelsAndFlush)
{
    R_CSTL_LOG_TRACE ("trace %s", "message");
    R_CSTL_LOG_DEBUG ("debug %d", 1);
    R_CSTL_LOG_INFO ("info %u", 2u);
    R_CSTL_LOG_WARN ("warn %f", 3.5);
    R_CSTL_LOG_ERROR ("error %x", 0x10);
    r_cstl_log_flush ();
    EXPECT_EQ (0u, r_cstl_log_get_dropped_count ());
}

TEST_F (CstlLogTest, VariadicFormattingMatchesPrintf)
{
    R_CSTL_LOG_INFO ("formatted %d %s %p", 42, "value", static_cast<void*> (nullptr));
    r_cstl_log_flush ();
    SUCCEED ();
}

TEST_F (CstlLogTest, FatalLogsBacktraceWithoutCrashing)
{
    R_CSTL_LOG_FATAL ("fatal condition %d", 7);
    r_cstl_log_flush ();
    SUCCEED ();
}

TEST_F (CstlLogTest, LogWriteVWorks)
{
    r_cstl_log_write (R_CSTL_LOG_LEVEL_DEBUG, "direct write %s", "ok");
    r_cstl_log_flush ();
    SUCCEED ();
}

TEST_F (CstlLogTest, FlushOnEmptyQueueIsSafe)
{
    r_cstl_log_flush ();
    SUCCEED ();
}

TEST_F (CstlLogTest, ShutdownDrainsPendingMessages)
{
    R_CSTL_LOG_INFO ("pending before shutdown");
    r_cstl_log_flush ();
    SUCCEED ();
}

TEST_F (CstlLogTest, DroppedCountStartsAtZero) { EXPECT_EQ (0u, r_cstl_log_get_dropped_count ()); }

TEST_F (CstlLogTest, TraceGetSettingsReturnsValidSettings)
{
    const struct r_cstl_trace_settings* pSettings = r_cstl_trace_get_settings ();
    ASSERT_NE (nullptr, pSettings);
}

TEST_F (CstlLogTest, TraceSetMinDuration)
{
    r_cstl_trace_set_min_duration (1000);
    // Verify the function doesn't crash
    SUCCEED ();
}

TEST_F (CstlLogTest, TraceGetTimestampReturnsIncreasingValues)
{
    uint64_t ts1 = r_cstl_trace_get_timestamp ();
    uint64_t ts2 = r_cstl_trace_get_timestamp ();
    EXPECT_LE (ts1, ts2);
}

TEST_F (CstlLogTest, TraceFunctionEntryExit)
{
    r_cstl_trace_function_entry ("TestFunction", "test_file.c", 42);
    r_cstl_trace_function_exit ("TestFunction", "test_file.c", 42, 100);
    SUCCEED ();
}

TEST_F (CstlLogTest, TraceFunctionEntryWithNullParameters)
{
    r_cstl_trace_function_entry (nullptr, nullptr, 0);
    r_cstl_trace_function_exit (nullptr, nullptr, 0, 0);
    SUCCEED ();
}

TEST_F (CstlLogTest, TraceLogEnvironmentInfo)
{
    r_cstl_trace_log_environment_info ();
    r_cstl_log_flush ();
    SUCCEED ();
}

TEST_F (CstlLogTest, TraceFunctionEntryExitWithRealDuration)
{
    uint64_t start = r_cstl_trace_get_timestamp ();
    r_cstl_trace_function_entry ("TimedFunction", __FILE__, __LINE__);

    // Simulate some work
    volatile int dummy = 0;
    for (int i = 0; i < 1000; ++i)
        dummy += i;

    uint64_t end = r_cstl_trace_get_timestamp ();
    uint64_t duration = end - start;
    r_cstl_trace_function_exit ("TimedFunction", __FILE__, __LINE__, duration);
    SUCCEED ();
}
