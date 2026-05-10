#pragma once
#include <vector>
#include <string>
#include "Types.h"

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
