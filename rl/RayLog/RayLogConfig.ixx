export module Rl.RayLog.Config;

namespace Rl::RayLog
{

/* Describes the max message queue size */
constinit size_t MaxQueueSize = 1024;

/* Describes the max log output file size */
constinit size_t MaxFileSize = 256 * 1024;

/* Count of worker threads */
constinit size_t WorkerThreads = 4;

/* Flush mode, immediate or not immediate */
constinit bool ImmediateFlush = true;

}
