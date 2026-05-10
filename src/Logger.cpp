#include "Logger.h"
#include <chrono>
#include <iomanip>

// تهيئة المتغير الثابت - المستوى الافتراضي للتسجيل
LogLevel Logger::currentLevel = LogLevel::INFO;

void Logger::log(LogLevel level, const std::string& message) {
	if (level >= currentLevel) {
		auto now = std::chrono::system_clock::now();
		auto time = std::chrono::system_clock::to_time_t(now);
		std::cerr << "[" << std::put_time(std::localtime(&time), "%H:%M:%S") << "] "
				  << "[Alif-LSP] " << levelToString(level) << ": " << message << std::endl;
	}
}
