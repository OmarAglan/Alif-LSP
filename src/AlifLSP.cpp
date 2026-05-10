#include "Server.h"
#include "Transport.h"
#include "Logger.h"

#include <iostream>
#include <memory>
#include <string>

int main(int argc, char* argv[]) {
	// Parse command line arguments
	for (int i = 1; i < argc; ++i) {
		std::string arg = argv[i];
		if (arg == "--debug" || arg == "-v") {
			Logger::setLevel(LogLevel::DEBUG);
			Logger::debug("Debug logging enabled via command line");
		}
	}

	// Disable stdio buffering
	setvbuf(stdin, nullptr, _IONBF, 0);
	setvbuf(stdout, nullptr, _IONBF, 0);

	auto transport = std::make_shared<StdioTransport>();
	LSPServer server(transport);
	return server.run();
}
