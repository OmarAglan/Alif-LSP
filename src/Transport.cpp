#include "Transport.h"
#include "Logger.h"

#include <iostream>
#include <vector>

#if defined(_WIN32)
#define NOMINMAX
#include <io.h>
#include <windows.h>
#include <fcntl.h>
#endif

StdioTransport::StdioTransport() {
#if defined(_WIN32)
	// Set binary mode for stdin/stdout on Windows
	int stdinInit = _setmode(_fileno(stdin), _O_BINARY);
	int stdoutInit = _setmode(_fileno(stdout), _O_BINARY);
	if (stdinInit == -1 || stdoutInit == -1) {
		Logger::error("Fatal Error: Cannot set binary mode for stdin/stdout");
		perror("Cannot set mode");
	}
#endif
}

std::optional<std::string> StdioTransport::readMessage() {
	std::string line;
	size_t length = 0;
	bool contentLengthFound = false;
	bool headerRead = false;

	while (std::getline(std::cin, line)) {
		headerRead = true;
		if (!line.empty() && line.back() == '\r') {
			line.pop_back();
		}

		if (line.find("Content-Length: ") == 0) {
			std::string lengthStr = line.substr(16);
			try {
				length = std::stoul(lengthStr);
				contentLengthFound = true;

				if (length > 1024 * 1024) {
					Logger::warn("Message too large: " + std::to_string(length) + " bytes");
					return std::nullopt;
				}
				if (length == 0) {
					Logger::warn("Empty message received (Content-Length: 0)");
					return std::nullopt;
				}
				Logger::debug("Content-Length: " + std::to_string(length));
			}
			catch (const std::exception& e) {
				Logger::warn("Invalid Content-Length header: " + lengthStr);
				return std::nullopt;
			}
		}

		if (line.empty()) {
			break;
		}
	}

	if (!headerRead && std::cin.eof()) {
		Logger::info("End of input stream reached. Exiting.");
		return std::nullopt;
	}

	if (!contentLengthFound) {
		Logger::warn("Message received without Content-Length header");
		return std::nullopt;
	}

	std::vector<char> buffer(length);
	std::cin.read(buffer.data(), length);

	if (!std::cin) {
		Logger::error("Failed to read message body (" + std::to_string(length) + " bytes)");
		std::cin.clear();
		return std::nullopt;
	}

	size_t bytesRead = std::cin.gcount();
	if (bytesRead != length) {
		Logger::warn("Incomplete message read: expected " + std::to_string(length) +
			" bytes, got " + std::to_string(bytesRead));
		return std::nullopt;
	}

	if (buffer.empty()) {
		Logger::warn("Empty message buffer received");
		return std::nullopt;
	}

	return std::string(buffer.begin(), buffer.end());
}

void StdioTransport::writeMessage(const std::string& msg) {
	std::cout << "Content-Length: " << msg.size() << "\r\n\r\n" << msg;
	std::cout.flush();
}

bool StdioTransport::isClosed() const {
	return std::cin.eof();
}
