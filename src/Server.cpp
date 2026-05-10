#include "Server.h"
#include "Logger.h"

#include <string>
#include <vector>

void LSPServer::sendResponse(const json& response) {
	if (transport_) {
		transport_->writeMessage(response.dump());
	}
}

// إرسال رد خطأ وفقاً لمعايير LSP
void LSPServer::sendErrorResponse(const json& id, int code, const std::string& message) {
	json errorResponse = {
		{"jsonrpc", "2.0"},
		{"id", id},
		{"error", {
			{"code", code},
			{"message", message}
		}}
	};
	sendResponse(errorResponse);
	Logger::warn("Error response sent: " + message);
}

json LSPServer::getCapabilities() const {
	return {
		{"completionProvider", {
			{"triggerCharacters", json::array({".", " "})} 
		}},
		{"textDocumentSync", 1} // Full sync
	};
}

void LSPServer::sendNotification(const std::string& method, const json& params) {
	json notification = {
		{"jsonrpc", "2.0"},
		{"method", method},
		{"params", params}
	};
	sendResponse(notification);
}

void LSPServer::publishDiagnostics(const std::string& uri, const json& diagnostics) {
	sendNotification("textDocument/publishDiagnostics", {
		{"uri", uri},
		{"diagnostics", diagnostics}
	});
}

// --- Constructor & dispatch table registration ---

LSPServer::LSPServer(std::shared_ptr<Transport> transport)
	: transport_(std::move(transport)) {
	registerHandlers();
}

void LSPServer::registerHandlers() {
	// Request handlers (client expects a response with matching id)
	requestHandlers_["initialize"]              = [this](const json& p, const json& id) { handleInitialize(p, id); };
	requestHandlers_["shutdown"]                 = [this](const json& p, const json& id) { handleShutdown(p, id); };
	requestHandlers_["textDocument/completion"]  = [this](const json& p, const json& id) { handleCompletion(p, id); };

	// Notification handlers (no response expected)
	notificationHandlers_["initialized"]             = [this](const json& p) { handleInitialized(p); };
	notificationHandlers_["exit"]                     = [this](const json& p) { handleExit(p); };
	notificationHandlers_["textDocument/didOpen"]     = [this](const json& p) { handleDidOpen(p); };
	notificationHandlers_["textDocument/didChange"]   = [this](const json& p) { handleDidChange(p); };
	notificationHandlers_["textDocument/didClose"]    = [this](const json& p) { handleDidClose(p); };
}

// --- Request handlers ---

void LSPServer::handleInitialize(const json& params, const json& id) {
	if (state_ != ServerState::Uninitialized) {
		Logger::warn("Duplicate initialize request rejected");
		sendErrorResponse(id, -32600, "Server already initialized");
		return;
	}

	sendResponse({
		{"jsonrpc", "2.0"},
		{"id", id},
		{"result", {
			{"capabilities", getCapabilities()},
			{"serverInfo", {
				{"name", "Alif-LSP"},
				{"version", "0.0.1"}
			}}
		}}
	});

	Logger::info("Server state: Uninitialized -> Waiting for 'initialized' notification");
}

void LSPServer::handleShutdown(const json& params, const json& id) {
	sendResponse({ {"jsonrpc", "2.0"}, {"id", id}, {"result", nullptr} });
	state_ = ServerState::ShuttingDown;
	Logger::info("Server state: Running -> ShuttingDown");
}

void LSPServer::handleCompletion(const json& params, const json& id) {
	// التحقق من وجود المعاملات المطلوبة
	if (!params.contains("textDocument") || !params.contains("position")) {
		sendErrorResponse(id, -32602, "Missing required parameters: textDocument or position");
		return;
	}

	auto textDoc = params["textDocument"];
	auto position = params["position"];

	// التحقق من وجود URI في مستند النص
	if (!textDoc.contains("uri") || !textDoc["uri"].is_string()) {
		sendErrorResponse(id, -32602, "Invalid textDocument: missing or invalid uri");
		return;
	}

	// التحقق من صحة معاملات الموضع
	if (!position.contains("line") || !position.contains("character") ||
		!position["line"].is_number_integer() || !position["character"].is_number_integer()) {
		sendErrorResponse(id, -32602, "Invalid position: line and character must be integers");
		return;
	}

	int line = position["line"].get<int>();
	int character = position["character"].get<int>();

	// التحقق من حدود القيم
	if (line < 0 || character < 0) {
		sendErrorResponse(id, -32602, "Invalid position: line and character must be non-negative");
		return;
	}

	std::string uri = textDoc["uri"].get<std::string>();

	// التحقق من وجود المستند في DocumentManager
	if (!docManager_.hasDocument(uri)) {
		Logger::warn("Completion requested for unopened document: " + uri);
		sendErrorResponse(id, -32603, "Document not found: " + uri);
		return;
	}

	std::string documentText = docManager_.getDocumentText(uri);
	std::string lineText = docManager_.getLineText(uri, line);

	CompletionContext ctx{ uri, line, character, lineText, documentText };

	try {
		json result = completionEngine_.getSuggestions(ctx);
		sendResponse({ {"jsonrpc", "2.0"}, {"id", id}, {"result", result} });
		Logger::debug("Completion request processed successfully for: " + uri);
	}
	catch (const std::exception& e) {
		sendErrorResponse(id, -32603, "Internal error: " + std::string(e.what()));
		Logger::error("Completion processing failed for " + uri + ": " + std::string(e.what()));
	}
}

// --- Notification handlers ---

void LSPServer::handleInitialized(const json& params) {
	state_ = ServerState::Running;
	Logger::info("Client confirmed initialization — server fully operational (State -> Running)");
}

void LSPServer::handleExit(const json& params) {
	Logger::info("Received exit notification. Shutting down cleanly.");
	running_ = false;
}

void LSPServer::handleDidOpen(const json& params) {
	if (!params.contains("textDocument")) {
		Logger::warn("didOpen notification missing textDocument");
		return;
	}
	auto doc = params["textDocument"];
	if (!doc.contains("uri") || !doc.contains("text") ||
		!doc["uri"].is_string() || !doc["text"].is_string()) {
		Logger::warn("didOpen notification has invalid textDocument structure");
		return;
	}
	int version = doc.contains("version") ? doc["version"].get<int>() : 0;
	DocumentError result = docManager_.openDocument(doc["uri"], doc["text"], version);
	if (result != DocumentError::SUCCESS) {
		Logger::warn("Failed to open document " + doc["uri"].get<std::string>() +
			": " + DocumentManager::errorToString(result));
	}
}

void LSPServer::handleDidChange(const json& params) {
	if (!params.contains("textDocument") || !params.contains("contentChanges")) {
		Logger::warn("didChange notification missing required parameters");
		return;
	}
	auto doc = params["textDocument"];
	auto changes = params["contentChanges"];

	if (!doc.contains("uri") || !doc["uri"].is_string() ||
		!changes.is_array() || changes.empty()) {
		Logger::warn("didChange notification has invalid structure");
		return;
	}

	if (!changes[0].contains("text") || !changes[0]["text"].is_string()) {
		Logger::warn("didChange notification has invalid content changes");
		return;
	}

	int version = doc.contains("version") ? doc["version"].get<int>() : docManager_.getDocumentVersion(doc["uri"]) + 1;
	DocumentError result = docManager_.updateDocument(doc["uri"], changes[0]["text"], version);
	if (result != DocumentError::SUCCESS) {
		Logger::warn("Failed to update document " + doc["uri"].get<std::string>() +
			": " + DocumentManager::errorToString(result));
	}
}

void LSPServer::handleDidClose(const json& params) {
	if (!params.contains("textDocument")) {
		Logger::warn("didClose notification missing textDocument");
		return;
	}
	auto doc = params["textDocument"];
	if (!doc.contains("uri") || !doc["uri"].is_string()) {
		Logger::warn("didClose notification has invalid textDocument structure");
		return;
	}
	DocumentError result = docManager_.closeDocument(doc["uri"]);
	if (result != DocumentError::SUCCESS) {
		Logger::warn("Failed to close document " + doc["uri"].get<std::string>() +
			": " + DocumentManager::errorToString(result));
	}
}

// --- Message dispatch ---

void LSPServer::handleMessage(const json& msg) {
	// التحقق من وجود حقل method
	if (!msg.contains("method") || !msg["method"].is_string()) {
		Logger::warn("Received message without valid method field");
		return;
	}

	std::string method = msg["method"].get<std::string>();
	Logger::debug("Processing method: " + method);

	// --- Lifecycle gating ---
	// في حالة عدم التهيئة: فقط initialize و exit مسموح بهما
	if (state_ == ServerState::Uninitialized && method != "initialize" && method != "exit") {
		Logger::warn("Rejected method before initialization: " + method);
		if (msg.contains("id")) {
			sendErrorResponse(msg["id"], -32002, "Server not yet initialized");
		}
		return;
	}

	// في حالة إيقاف التشغيل: فقط exit مسموح به
	if (state_ == ServerState::ShuttingDown && method != "exit") {
		Logger::warn("Rejected method after shutdown: " + method);
		if (msg.contains("id")) {
			sendErrorResponse(msg["id"], -32600, "Server is shutting down");
		}
		return;
	}

	// --- Dispatch via tables ---
	// Check if it's a request (has "id" field)
	if (msg.contains("id")) {
		auto it = requestHandlers_.find(method);
		if (it != requestHandlers_.end()) {
			json params = msg.contains("params") ? msg["params"] : json::object();
			it->second(params, msg["id"]);
			return;
		}
		// Unknown request method — respond with method not found
		sendErrorResponse(msg["id"], -32601, "Method not found: " + method);
		return;
	}

	// It's a notification (no "id")
	auto it = notificationHandlers_.find(method);
	if (it != notificationHandlers_.end()) {
		json params = msg.contains("params") ? msg["params"] : json::object();
		it->second(params);
		return;
	}

	// Unknown notification — just log it
	Logger::debug("Unsupported notification: " + method);
}

// التحقق من صحة بنية رسالة LSP الأساسية
bool LSPServer::isValidLSPMessage(const json& msg) {
	// التحقق من أن الرسالة كائن JSON
	if (!msg.is_object()) {
		Logger::debug("Message validation failed: not a JSON object");
		return false;
	}

	// التحقق من وجود حقل jsonrpc (اختياري لكن مفيد)
	if (msg.contains("jsonrpc")) {
		if (!msg["jsonrpc"].is_string() || msg["jsonrpc"] != "2.0") {
			Logger::debug("Message validation failed: invalid jsonrpc version");
			return false;
		}
	}

	// التحقق من وجود حقل method
	if (!msg.contains("method")) {
		Logger::debug("Message validation failed: missing method field");
		return false;
	}

	if (!msg["method"].is_string()) {
		Logger::debug("Message validation failed: method is not a string");
		return false;
	}

	// التحقق من أن method ليس فارغاً
	std::string method = msg["method"].get<std::string>();
	if (method.empty()) {
		Logger::debug("Message validation failed: empty method");
		return false;
	}

	// التحقق من أن id موجود وصالح للطلبات التي تحتاج رد
	if (msg.contains("id")) {
		if (!msg["id"].is_number() && !msg["id"].is_string() && !msg["id"].is_null()) {
			Logger::debug("Message validation failed: invalid id type");
			return false;
		}
	}

	Logger::debug("Message validation passed for method: " + method);
	return true;
}

int LSPServer::run() {
	Logger::info("Alif Server Started");

	while (running_ && transport_ && !transport_->isClosed()) {
		auto msgStr = transport_->readMessage();
		if (!msgStr) continue;

		try {
			json msg = json::parse(*msgStr);

			// التحقق من أن الرسالة تحتوي على حقول أساسية
			if (msg.is_null() || (!msg.is_object())) {
				Logger::warn("Invalid JSON structure: not an object");
				continue;
			}

			// التحقق من صحة رسالة LSP
			if (!isValidLSPMessage(msg)) {
				Logger::warn("Invalid LSP message structure received");
				continue;
			}

			Logger::debug("Message parsed successfully");
			handleMessage(msg);
		}
		catch (const json::parse_error& e) {
			// خطأ في تحليل JSON مع تفاصيل أكثر
			Logger::error("JSON Parse Error at byte " + std::to_string(e.byte) +
				": " + std::string(e.what()));

			std::string preview = msgStr->substr(0, std::min(size_t(100), msgStr->size()));
			Logger::debug("Message preview: " + preview + (msgStr->size() > 100 ? "..." : ""));
		}
		catch (const std::exception& e) {
			Logger::error("Unexpected error during message processing: " + std::string(e.what()));
		}
	}

	// LSP spec: return 0 if shutdown was requested before exit, 1 otherwise
	int exitCode = (state_ == ServerState::ShuttingDown) ? 0 : 1;
	Logger::info("Alif Server exiting with code " + std::to_string(exitCode));
	return exitCode;
}
