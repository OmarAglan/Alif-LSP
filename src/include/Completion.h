#pragma once
#include <vector>
#include <string>
#include "json.hpp"

using json = nlohmann::json;

struct CompletionContext {
	std::string uri;
	int line;
	int character;
	std::string lineText;        // text of the current line
	std::string documentText;    // full document text
};

class Completion {
public:
	json getSuggestions(const CompletionContext& ctx);
};
