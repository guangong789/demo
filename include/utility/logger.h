#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>

namespace gaozu
{
    namespace logger
    {
// 定义宏
#define log_debug(format, ...) \
    Logger::instance()->log(Logger::DEBUG, __FILE__, __LINE__, format, ##__VA_ARGS__);
#define log_info(format, ...) \
    Logger::instance()->log(Logger::INFO, __FILE__, __LINE__, format, ##__VA_ARGS__);
#define log_warn(format, ...) \
    Logger::instance()->log(Logger::WARN, __FILE__, __LINE__, format, ##__VA_ARGS__);
#define log_error(format, ...) \
    Logger::instance()->log(Logger::ERROR, __FILE__, __LINE__, format, ##__VA_ARGS__);
#define log_fatal(format, ...) \
    Logger::instance()->log(Logger::FATAL, __FILE__, __LINE__, format, ##__VA_ARGS__);

        class Logger
        {
        public:
            enum Level
            {
                DEBUG = 0,
                INFO,
                WARN,
                ERROR,
                FATAL,
                LEVEL_COUNT
            };
            static Logger *instance();
            void open(const std::string &filename);
            void close();
            void log(Level level, const char *file, int line, const char *format, ...);

            void level(Level level);
            void max(int bytes);
            void set_console(bool enable);    // 控制台输出开关
            void set_backup_count(int count); // 新增：设置备份数量

        private:
            Logger();
            ~Logger();
            void rotate();             // 日志翻滚
            void remove_old_backups(); // 删除旧的备份文件

        private:
            std::string m_filename;
            std::ofstream m_fout;
            Level m_level;
            int m_len;             // 当前长度
            int m_max;             // 最大容量
            bool m_console_output; // 是否控制台输出
            int m_backup_count;    // 备份文件数量
            static const char *s_level[LEVEL_COUNT];
            static Logger *m_instance;
        };
    }
}