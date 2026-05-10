#pragma once
#include <string>
#include "json.hpp"
#include "Logger.h"
#include "DocManager.h"
#include "Completion.h"

using json = nlohmann::json;

// حالات دورة حياة خادم LSP
enum class ServerState {
	Uninitialized,  // قبل استلام طلب initialize
	Running,        // بعد initialize/initialized — التشغيل العادي
	ShuttingDown    // بعد استلام طلب shutdown
};

class LSPServer {
public:
	int run();
	void handleMessage(const json& msg);

private:
	// Server lifecycle state
	ServerState state_ = ServerState::Uninitialized;

	// Server-owned components (no globals)
	DocumentManager docManager_;
	Completion completionEngine_;

	void sendResponse(const json& response);
	void sendErrorResponse(const json& id, int code, const std::string& message);
	void initialize(const json& params, const json& id);
	void handleCompletion(const json& params, const json& id);
	bool isValidLSPMessage(const json& msg);
};
