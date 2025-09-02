#pragma once
#include <string>
#include <stdexcept>
#include <crow.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace error_handler {

// Типы ошибок
enum class ErrorType {
    DATABASE_ERROR,
    VALIDATION_ERROR,
    NOT_FOUND_ERROR,
    BAD_REQUEST_ERROR,
    INTERNAL_SERVER_ERROR
};

// Структура для деталей ошибки
struct ErrorDetails {
    ErrorType type;
    std::string message;
    std::string details;
    int status_code;
};

// Базовый класс для пользовательских исключений
class ApiException : public std::runtime_error {
public:
    explicit ApiException(const ErrorDetails& details)
        : std::runtime_error(details.message), details_(details) {}
    
    const ErrorDetails& getDetails() const { return details_; }
    int getStatusCode() const { return details_.status_code; }
    
private:
    ErrorDetails details_;
};

// Конкретные типы исключений
class DatabaseException : public ApiException {
public:
    explicit DatabaseException(const std::string& message, const std::string& details = "")
        : ApiException({ErrorType::DATABASE_ERROR, message, details, 500}) {}
};

class ValidationException : public ApiException {
public:
    explicit ValidationException(const std::string& message, const std::string& details = "")
        : ApiException({ErrorType::VALIDATION_ERROR, message, details, 400}) {}
};

class NotFoundException : public ApiException {
public:
    explicit NotFoundException(const std::string& message, const std::string& details = "")
        : ApiException({ErrorType::NOT_FOUND_ERROR, message, details, 404}) {}
};

class BadRequestException : public ApiException {
public:
    explicit BadRequestException(const std::string& message, const std::string& details = "")
        : ApiException({ErrorType::BAD_REQUEST_ERROR, message, details, 400}) {}
};

// Класс ErrorHandler
class ErrorHandler {
public:
    static crow::response handleError(const ApiException& e);
    static crow::response handleStdException(const std::exception& e);
    static crow::response handleUnknownException();
    
    // Статические методы для создания конкретных ошибок
    static crow::response badRequest(const std::string& message, const std::string& details = "");
    static crow::response notFound(const std::string& message, const std::string& details = "");
    static crow::response internalError(const std::string& message, const std::string& details = "");
    static crow::response validationError(const std::string& message, const std::string& details = "");
    static crow::response databaseError(const std::string& message, const std::string& details = "");

private:
    static std::string errorTypeToString(ErrorType type);
    static json createErrorResponse(int status_code, const std::string& error, 
                                  const std::string& message, const std::string& details = "");
};

} // namespace error_handler