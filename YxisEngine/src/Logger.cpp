#include <Yxis/Logger.h>

namespace Yxis
{
   bool Logger::m_initialized = false;
   Logger::logger_t Logger::m_coreLogger;
   Logger::logger_t Logger::m_clientLogger;

   void Logger::initialize() noexcept
   {
      if (m_initialized) return;

      auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
      consoleSink->set_level(spdlog::level::debug);
      consoleSink->set_pattern("%^[%n-%l %T] [TID:%t] %v%$");

      auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("log.txt", true);
      fileSink->set_level(spdlog::level::info);
      fileSink->set_pattern("[%n-%l %T] [TID:%t] %v%");

      m_coreLogger.reset(new spdlog::logger("YxisCore", { consoleSink, fileSink }));
      m_clientLogger.reset(new spdlog::logger("YxisClient", { consoleSink, fileSink }));
   }

   Logger::logger_t Logger::getCoreLogger() noexcept
   {
      return m_coreLogger;
   }

   Logger::logger_t Logger::getClientLogger() noexcept
   {
      return m_clientLogger;
   }
}