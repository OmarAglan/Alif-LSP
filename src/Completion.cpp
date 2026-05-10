#include "Completion.h"
#include <vector>
#include <string>


json Completion::getSuggestions(const CompletionContext& ctx) {
	// Define enhanced completion item structure
	struct CompletionItem {
		std::string label{};
		int kind{};  // LSP CompletionItemKind
		std::string detail{};
		std::string documentation{};
	};

	std::vector<CompletionItem> items{};

	// Determine context from line text up to the cursor character
	std::string prefix = "";
	if (ctx.character > 0 && ctx.character <= ctx.lineText.length()) {
		prefix = ctx.lineText.substr(0, ctx.character);
	}

	bool isMemberAccess = !prefix.empty() && prefix.back() == '.';

	if (isMemberAccess) {
		// Example member properties/methods for objects
		const std::vector<std::string> members = {
			"طول", "اضف", "احذف", "امسح", "رتب", "ابحث"
		};
		for (const auto& m : members) {
			items.push_back({ m, 2, "خاصية/دالة كائن", "طريقة متاحة للكائن" }); // Kind 2: Method
		}
	} else {
		// General completions (Keywords, built-ins, etc.)
		const std::vector<std::string> keywords = {
			"و", "ك", "توقف", "صنف", "استمر",
			"دالة", "احذف", "اواذا", "والا", "خلل", "نهاية", "لاجل", "من",
			"عام", "اذا", "استورد", "في", "هل", "نطاق", "ليس",
			"او", "مرر", "ارجع", "حاول", "بينما", "عند", "ولد"
		};
		for (const auto& kw : keywords) {
			items.push_back({ kw, 14, "محجوزة", "كلمة مفتاحية محجوزة" }); // Kind 14: Keyword
		}

		const std::vector<std::string> constants = { "خطا", "عدم", "صح" };
		for (const auto& c : constants) {
			items.push_back({ c, 21, "ثابت", "قيمة ثابتة ضمنية" }); // Kind 21: Constant
		}

		const std::vector<std::string> functions = {
			"مطلق", "اطبع", "اي", "منطق", "فهرس", "عشري",
			"ادخل", "صحيح", "طول", "مصفوفة", "اقصى", "ادنى", "افتح", "مدى",
			"نص", "اصل", "مترابطة", "نوع"
		};
		for (const auto& fn : functions) {
			items.push_back({ fn, 3, "دالة ضمنية", "دالة من مكتبة ألف الضمنية" }); // Kind 3: Function
		}

		const std::vector<std::string> types = {
			"منطق", "فهرس", "عشري",
			"صحيح", "مصفوفة", "كائن", "نص", "مترابطة", "نوع"
		};
		for (const auto& t : types) {
			items.push_back({ t, 7, "نوع", "نوع بيانات اساسي" }); // Kind 7: Class
		}
	}

	// Sort alphabetically
	std::sort(items.begin(), items.end(),
		[](const CompletionItem& a, const CompletionItem& b) {
			return a.label < b.label;
		});

	// Convert to JSON with proper snippet handling
	json jsonItems = json::array();
	for (const auto& item : items) {
		json j = {
			{"label", item.label},
			{"kind", item.kind},
			{"detail", item.detail},
			{"documentation", item.documentation}
		};
		jsonItems.push_back(j);
	}

	return { {"isIncomplete", false}, {"items", jsonItems} };
}
