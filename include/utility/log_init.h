#pragma once
#include "logger.h"
#include <string>

namespace gaozu {
    namespace init {
        class LoggerInitializer {
        public:
            // 静态工厂方法
            static LoggerInitializer create(const std::string& filename = "logs/app.log") {
                return LoggerInitializer(filename);
            }
            
            LoggerInitializer(const std::string& filename = "logs/app.log") {
                auto logger = gaozu::logger::Logger::instance();
                logger->open(filename);
            }
            
            LoggerInitializer& set_console(bool enable) {
                auto logger = gaozu::logger::Logger::instance();
                logger->set_console(enable);
                return *this;
            }
            
            LoggerInitializer& set_level(gaozu::logger::Logger::Level level) {
                auto logger = gaozu::logger::Logger::instance();
                logger->level(level);
                return *this;
            }
            
            LoggerInitializer& set_max_size(int bytes) {
                auto logger = gaozu::logger::Logger::instance();
                logger->max(bytes);
                return *this;
            }
            
            // 设置备份数量
            LoggerInitializer& set_backup_count(int count) {
                auto logger = gaozu::logger::Logger::instance();
                logger->set_backup_count(count);
                return *this;
            }
        };
    }
}