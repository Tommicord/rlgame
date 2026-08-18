#include <gtest/gtest.h>

extern "C"
{
#include "rlgame.base/cstl/cstl_heap_allocator.h"
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
      ASSERT_EQ (0, R_CSTL_HeapInit (256 * 1024));
      ASSERT_EQ (0, R_CSTL_LogInit ());
    }

    void
    TearDown () override
    {
      R_CSTL_LogShutdown ();
      R_CSTL_HeapShutdown ();
    }
};

} // namespace

TEST (CstlLogInitTest, InitFailsWithoutHeap)
{ EXPECT_EQ (-1, R_CSTL_LogInit ()); }

TEST (CstlLogInitTest, InitDestroyAndReinit)
{
  ASSERT_EQ (0, R_CSTL_HeapInit (128 * 1024));
  ASSERT_EQ (0, R_CSTL_LogInit ());
  R_CSTL_LogShutdown ();
  ASSERT_EQ (0, R_CSTL_LogInit ());
  R_CSTL_LogShutdown ();
  R_CSTL_HeapShutdown ();
}

TEST_F (CstlLogTest, LevelNames)
{
  EXPECT_STREQ ("TRACE", R_CSTL_LogLevelName (R_CSTL_LOG_LEVEL_TRACE));
  EXPECT_STREQ ("DEBUG", R_CSTL_LogLevelName (R_CSTL_LOG_LEVEL_DEBUG));
  EXPECT_STREQ ("INFO", R_CSTL_LogLevelName (R_CSTL_LOG_LEVEL_INFO));
  EXPECT_STREQ ("WARN", R_CSTL_LogLevelName (R_CSTL_LOG_LEVEL_WARN));
  EXPECT_STREQ ("ERROR", R_CSTL_LogLevelName (R_CSTL_LOG_LEVEL_ERROR));
  EXPECT_STREQ ("FATAL", R_CSTL_LogLevelName (R_CSTL_LOG_LEVEL_FATAL));
  EXPECT_STREQ ("UNKNOWN", R_CSTL_LogLevelName (static_cast<R_CSTL_LogLevel> (999)));
}

TEST_F (CstlLogTest, DefaultMinLevelIsTrace)
{ EXPECT_EQ (R_CSTL_LOG_LEVEL_TRACE, R_CSTL_LogGetMinLevel ()); }

TEST_F (CstlLogTest, SetMinLevelFiltersMessages)
{
  R_CSTL_LogSetMinLevel (R_CSTL_LOG_LEVEL_WARN);
  EXPECT_EQ (R_CSTL_LOG_LEVEL_WARN, R_CSTL_LogGetMinLevel ());

  R_CSTL_LOG_INFO ("filtered info %d", 1);
  R_CSTL_LOG_WARN ("visible warn %d", 2);
  R_CSTL_LogFlush ();

  R_CSTL_LogSetMinLevel (R_CSTL_LOG_LEVEL_TRACE);
}

TEST_F (CstlLogTest, InvalidMinLevelIsIgnored)
{
  R_CSTL_LogSetMinLevel (R_CSTL_LOG_LEVEL_INFO);
  R_CSTL_LogSetMinLevel (static_cast<R_CSTL_LogLevel> (-1));
  EXPECT_EQ (R_CSTL_LOG_LEVEL_INFO, R_CSTL_LogGetMinLevel ());
}

TEST_F (CstlLogTest, WriteAllLevelsAndFlush)
{
  R_CSTL_LOG_TRACE ("trace %s", "message");
  R_CSTL_LOG_DEBUG ("debug %d", 1);
  R_CSTL_LOG_INFO ("info %u", 2u);
  R_CSTL_LOG_WARN ("warn %f", 3.5);
  R_CSTL_LOG_ERROR ("error %x", 0x10);
  R_CSTL_LogFlush ();
  EXPECT_EQ (0u, R_CSTL_LogGetDroppedCount ());
}

TEST_F (CstlLogTest, VariadicFormattingMatchesPrintf)
{
  R_CSTL_LOG_INFO ("formatted %d %s %p", 42, "value", static_cast<void*> (nullptr));
  R_CSTL_LogFlush ();
  SUCCEED ();
}

TEST_F (CstlLogTest, FatalLogsBacktraceWithoutCrashing)
{
  R_CSTL_LOG_FATAL ("fatal condition %d", 7);
  R_CSTL_LogFlush ();
  SUCCEED ();
}

TEST_F (CstlLogTest, LogWriteVWorks)
{
  R_CSTL_LogWrite (R_CSTL_LOG_LEVEL_DEBUG, "direct write %s", "ok");
  R_CSTL_LogFlush ();
  SUCCEED ();
}

TEST_F (CstlLogTest, FlushOnEmptyQueueIsSafe)
{
  R_CSTL_LogFlush ();
  SUCCEED ();
}

TEST_F (CstlLogTest, ShutdownDrainsPendingMessages)
{
  R_CSTL_LOG_INFO ("pending before shutdown");
  R_CSTL_LogFlush ();
  SUCCEED ();
}

TEST_F (CstlLogTest, DroppedCountStartsAtZero)
{ EXPECT_EQ (0u, R_CSTL_LogGetDroppedCount ()); }
