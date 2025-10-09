#include "utility/logger.h"

namespace fs = std::filesystem;
using namespace gaozu::logger;

const char *Logger::s_level[] = {
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL"};

Logger *Logger::m_instance = nullptr;

Logger::Logger() : m_level(DEBUG), m_len(0), m_max(0), m_console_output(true), m_backup_count(0) {}

Logger::~Logger()
{
    close();
}

Logger *Logger::instance()
{
    if (m_instance == nullptr)
    {
        m_instance = new Logger();
    }
    return m_instance;
}

void Logger::open(const std::string &filename)
{
    m_filename = filename;
    m_fout.open(filename, std::ios::app);
    if (m_fout.fail())
    {
        throw std::logic_error("open file faild" + filename);
    }
    m_fout.seekp(0, std::ios::end);
    m_len = m_fout.tellp(); // 获取当前长度
}

void Logger::close()
{
    m_fout.close();
}

void Logger::log(Level level, const char *file, int line, const char *format, ...)
{
    if (m_level > level)
        return;
    if (m_fout.fail())
    {
        throw std::logic_error("open file failed");
    }
    time_t ticks = time(nullptr);
    struct tm *ptn = localtime(&ticks);
    char timestamp[32];
    std::memset(timestamp, 0, sizeof(timestamp));
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", ptn);

    const char *fmt = "%s %s %s:%d "; // snprintf()函数依次解析：时间，级别，文件，行号
    int size = snprintf(nullptr, 0, fmt, timestamp, s_level[level], file, line);
    // 只计算，不写入，精确分配大小
    if (size > 0)
    {
        char *buffer = new char[size + 1]; // 加上字符结尾
        snprintf(buffer, size + 1, fmt, timestamp, s_level[level], file, line);
        buffer[size] = '\0';

        // 控制台输出
        if (m_console_output)
        {
            std::cout << buffer;
        }
        m_fout << buffer; // 写入文件
        m_len += size;    // 增加长度
        delete[] buffer;
    }

    va_list arg_ptr;
    va_start(arg_ptr, format);
    size = vsnprintf(nullptr, 0, format, arg_ptr); // 接收可变参数
    va_end(arg_ptr);
    if (size > 0)
    {
        char *content = new char[size + 1];
        va_start(arg_ptr, format);
        vsnprintf(content, size + 1, format, arg_ptr);
        va_end(arg_ptr);
        content[size] = '\0';

        // 控制台输出
        if (m_console_output)
        {
            std::cout << content;
        }
        m_fout << content; // 写入文件
        m_len += size;
        delete[] content;
    }

    // 控制台输出换行
    if (m_console_output)
    {
        std::cout << std::endl;
    }
    m_fout << "\n";
    m_fout.flush(); // 写入日志

    if (m_max > 0 && m_len >= m_max)
    {
        rotate();
    }
}

void Logger::rotate()
{
    close();
    
    // 生成带时间戳的备份文件名
    time_t ticks = time(nullptr);
    struct tm *ptm = localtime(&ticks);
    char timestamp[32];
    memset(timestamp, 0, sizeof(timestamp));
    strftime(timestamp, sizeof(timestamp), ".%Y-%m-%d_%H:%M:%S", ptm);
    std::string backup_filename = m_filename + timestamp;
    
    // 重命名当前日志文件为备份文件
    if (rename(m_filename.c_str(), backup_filename.c_str()) != 0)
    {
        throw std::logic_error("rename log file failed: " + std::string(strerror(errno)));
    }
    
    // 删除旧的备份文件（如果设置了备份数量限制）
    if (m_backup_count > 0) {
        remove_old_backups();
    }
    
    // 重新打开主日志文件
    open(m_filename);
}

void Logger::remove_old_backups()
{
    try {
        // 获取日志文件目录和基本名称
        size_t last_slash = m_filename.find_last_of('/');
        std::string dir_str = (last_slash != std::string::npos) ? m_filename.substr(0, last_slash + 1) : "./";
        std::string base_name = (last_slash != std::string::npos) ? m_filename.substr(last_slash + 1) : m_filename;
        
        fs::path dir(dir_str);
        
        // 确保目录存在
        if (!fs::exists(dir) || !fs::is_directory(dir)) {
            log_error("Directory does not exist: %s", dir_str.c_str());
            return;
        }
        
        // 存储找到的备份文件信息
        struct BackupFile {
            fs::path path;
            std::time_t mtime;
        };
        std::vector<BackupFile> backup_files;
        
        // 遍历目录，查找所有备份文件
        for (const auto& entry : fs::directory_iterator(dir)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                
                // 检查是否是备份文件：以 base_name 开头且包含时间戳格式
                if (filename.find(base_name) == 0 && filename != base_name) {
                    // 检查是否是时间戳格式的备份文件 (base_name.YYYY-MM-DD_HH:MM:SS)
                    if (filename.length() > base_name.length() + 1 && 
                        filename[base_name.length()] == '.') {
                        // 验证时间戳格式（简单检查）
                        std::string timestamp_part = filename.substr(base_name.length() + 1);
                        if (timestamp_part.length() == 19 && // YYYY-MM-DD_HH:MM:SS 长度
                            timestamp_part[4] == '-' && timestamp_part[7] == '-' &&
                            timestamp_part[10] == '_' && timestamp_part[13] == ':' &&
                            timestamp_part[16] == ':') {
                            
                            auto ftime = fs::last_write_time(entry.path());
                            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                                ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
                            std::time_t mtime = std::chrono::system_clock::to_time_t(sctp);
                            
                            backup_files.push_back({entry.path(), mtime});
                        }
                    }
                }
            }
        }
        
        // 如果备份文件数量超过限制，删除最旧的文件
        if (backup_files.size() > static_cast<size_t>(m_backup_count)) {
            // 按修改时间排序（最旧的在前）
            std::sort(backup_files.begin(), backup_files.end(),
                     [](const BackupFile& a, const BackupFile& b) {
                         return a.mtime < b.mtime;
                     });
            
            // 删除最旧的文件，直到数量符合要求
            for (size_t i = 0; i < backup_files.size() - m_backup_count; ++i) {
                try {
                    if (fs::remove(backup_files[i].path)) {
                        log_info("Removed old backup: %s", backup_files[i].path.string().c_str());
                    } else {
                        log_error("Failed to remove backup: %s", backup_files[i].path.string().c_str());
                    }
                } catch (const fs::filesystem_error& e) {
                    log_error("Error removing backup %s: %s", 
                             backup_files[i].path.string().c_str(), e.what());
                }
            }
        }
        
        log_debug("Backup cleanup completed. Current backups: %zu, limit: %d", 
                 backup_files.size(), m_backup_count);
    }
    catch (const std::exception& e) {
        log_error("Failed to remove old backups: %s", e.what());
    }
}

void Logger::level(Level level)
{ // 设置最低级别
    m_level = level;
}

void Logger::max(int bytes)
{ // 设置最大容量
    m_max = bytes;
}

// 设置控制台输出
void Logger::set_console(bool enable)
{
    m_console_output = enable;
}

// 新增：设置备份数量
void Logger::set_backup_count(int count)
{
    m_backup_count = count;
    if (m_backup_count < 0) m_backup_count = 0;
    log_debug("Backup count set to: %d", m_backup_count);
}