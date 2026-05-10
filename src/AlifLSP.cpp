#include "Server.h"
#include "Transport.h"

#include <iostream>
#include <memory>

int main() {
	// Disable stdio buffering
	setvbuf(stdin, nullptr, _IONBF, 0);
	setvbuf(stdout, nullptr, _IONBF, 0);

	auto transport = std::make_shared<StdioTransport>();
	LSPServer server(transport);
	return server.run();
}
