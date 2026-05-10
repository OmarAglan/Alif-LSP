#pragma once

#include <string>
#include <optional>

class Transport {
public:
	virtual ~Transport() = default;
	virtual std::optional<std::string> readMessage() = 0;
	virtual void writeMessage(const std::string& msg) = 0;
	virtual bool isClosed() const = 0;
};

class StdioTransport : public Transport {
public:
	StdioTransport();
	std::optional<std::string> readMessage() override;
	void writeMessage(const std::string& msg) override;
	bool isClosed() const override;
};
