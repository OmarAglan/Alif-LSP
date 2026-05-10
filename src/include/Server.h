#pragma once
#include <string>
#include <functional>
#include <unordered_map>
#include "Types.h"
#include "Logger.h"
#include "DocManager.h"
#include "Completion.h"
#include "Transport.h"
#include <memory>

// حالات دورة حياة خادم LSP
enum class ServerState {
	Uninitialized,  // قبل استلام طلب initialize
	Running,        // بعد initialize/initialized — التشغيل العادي
	ShuttingDown    // بعد استلام طلب shutdown
};

// Handler types for dispatch table
// Requests have an id and expect a response
using RequestHandler = std::function<void(const json& params, const json& id)>;
// Notifications are fire-and-forget (no id, no response)
using NotificationHandler = std::function<void(const json& params)>;

class LSPServer {
public:
	explicit LSPServer(std::shared_ptr<Transport> transport);
	int run();
	void handleMessage(const json& msg);

private:
	// Server lifecycle state
	ServerState state_ = ServerState::Uninitialized;
	bool running_ = true;

	// Server-owned components
	std::shared_ptr<Transport> transport_;
	DocumentManager docManager_;
	Completion completionEngine_;

	// --- Dispatch tables ---
	std::unordered_map<std::string, RequestHandler> requestHandlers_;
	std::unordered_map<std::string, NotificationHandler> notificationHandlers_;
	void registerHandlers();

	// --- Transport ---
	void sendResponse(const json& response);
	void sendErrorResponse(const json& id, int code, const std::string& message);
	void sendNotification(const std::string& method, const json& params);
	void publishDiagnostics(const std::string& uri, const json& diagnostics);

	// --- Request handlers (expect a response) ---
	void handleInitialize(const json& params, const json& id);
	void handleShutdown(const json& params, const json& id);
	void handleCompletion(const json& params, const json& id);

	// --- Notification handlers (no response) ---
	void handleInitialized(const json& params);
	void handleExit(const json& params);
	void handleDidOpen(const json& params);
	void handleDidChange(const json& params);
	void handleDidClose(const json& params);

	// --- Helpers ---
	json getCapabilities() const;
	bool isValidLSPMessage(const json& msg);
};
