#pragma once
#include "definitions.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <memory>

namespace Yxis
{
   class YX_API Logger
   {
   public:
      using logger_t = std::shared_ptr<spdlog::logger>;

      static void initialize() noexcept; // this function can't throw errors because it gets called before try block
      static logger_t getClientLogger() noexcept;
      static logger_t getCoreLogger() noexcept;
   private:
      static bool m_initialized;
      static logger_t m_coreLogger;
      static logger_t m_clientLogger;
   };
}