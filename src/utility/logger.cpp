#include "utility/logger.h"

namespace fs = std::filesystem;
using namespace gaozu::logger;

const char* Logger::s_level[] = {
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL"};

Logger* Logger::m_instance = nullptr;

Logger::Logger() : m_level(DEBUG), m_len(0), m_max(0), m_console_output(true) {}

Logger::~Logger() {
    close();
}

Logger* Logger::instance() {
    if (m_instance == nullptr) {
        m_instance = new Logger();
    }
    return m_instance;
}

void Logger::open(const std::string& filename) {
    m_filename = filename;
    m_fout.open(filename, std::ios::app);
    if (m_fout.fail()) {
        throw std::logic_error("open file faild" + filename);
    }
    m_fout.seekp(0, std::ios::end);
    m_len = m_fout.tellp();  // 获取当前长度
}

void Logger::close() {
    m_fout.close();
}

void Logger::log(Level level, const char* file, int line, const char* format, ...) {
    if (m_level > level)
        return;
    if (m_fout.fail()) {
        throw std::logic_error("open file failed");
    }
    time_t ticks = time(nullptr);
    struct tm* ptn = localtime(&ticks);
    char timestamp[32];
    std::memset(timestamp, 0, sizeof(timestamp));
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", ptn);

    const char* fmt = "%s %s %s:%d ";  // snprintf()函数依次解析：时间，级别，文件，行号
    int size = snprintf(nullptr, 0, fmt, timestamp, s_level[level], file, line);
    // 只计算，不写入，精确分配大小
    if (size > 0) {
        char* buffer = new char[size + 1];  // 加上字符结尾
        snprintf(buffer, size + 1, fmt, timestamp, s_level[level], file, line);
        buffer[size] = '\0';

        // 控制台输出
        if (m_console_output) {
            std::cout << buffer;
        }
        m_fout << buffer;  // 写入文件
        m_len += size;     // 增加长度
        delete[] buffer;
    }

    va_list arg_ptr;
    va_start(arg_ptr, format);
    size = vsnprintf(nullptr, 0, format, arg_ptr);  // 接收可变参数
    va_end(arg_ptr);
    if (size > 0) {
        char* content = new char[size + 1];
        va_start(arg_ptr, format);
        vsnprintf(content, size + 1, format, arg_ptr);
        va_end(arg_ptr);
        content[size] = '\0';
        // 控制台输出
        if (m_console_output) {
            std::cout << content;
        }
        m_fout << content;  // 写入文件
        m_len += size;
        delete[] content;
    }
    // 控制台输出换行
    if (m_console_output) {
        std::cout << std::endl;
    }
    m_fout << "\n";
    m_fout.flush();  // 写入日志
}

void Logger::level(Level level) {  // 设置最低级别
    m_level = level;
}

void Logger::max(int bytes) {  // 设置最大容量
    m_max = bytes;
}

void Logger::set_console(bool enable) {  // 设置控制台输出
    m_console_output = enable;
}