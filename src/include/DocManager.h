#pragma once
#include <string>
#include <unordered_map>
#include <vector>

// أنواع الأخطاء المحتملة في إدارة المستندات
enum class DocumentError {
	SUCCESS = 0,
	INVALID_URI,
	DOCUMENT_NOT_FOUND,
	DOCUMENT_ALREADY_EXISTS,
	INVALID_CONTENT,
	OPERATION_FAILED
};

struct Document {
	std::string uri;
	std::string text;
	int version = 0;
	std::vector<std::string> lines;
};

class DocumentManager {
public:
	// إدارة المستندات مع معالجة الأخطاء
	DocumentError openDocument(const std::string& uri, const std::string& text, int version = 0);
	DocumentError updateDocument(const std::string& uri, const std::string& text, int version = 0);
	DocumentError closeDocument(const std::string& uri);
	
	// الحصول على معلومات المستند
	std::string getDocumentText(const std::string& uri) const;
	std::string getLineText(const std::string& uri, int line) const;
	int getDocumentVersion(const std::string& uri) const;
	bool hasDocument(const std::string& uri) const;
	size_t getDocumentCount() const;
	
	// تحويل رمز الخطأ إلى رسالة نصية
	static std::string errorToString(DocumentError error);

private:
	std::unordered_map<std::string, Document> documents;
	
	// مساعدة لتحديث الأسطر
	void updateDocumentLines(Document& doc);

	// التحقق من صحة URI
	bool isValidURI(const std::string& uri) const;
};