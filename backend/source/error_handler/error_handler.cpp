#include "error_handler.h"
#include <iostream>

namespace ErrorHandler {

crow::response ErrorHandler::handleError(const ApiException& e) {
    const auto& details = e.getDetails();
    
    std::cerr << "API Error [" << errorTypeToString(details.type) << "]: " 
              << details.message << " | Details: " << details.details << std::endl;
    
    auto response = createErrorResponse(
        details.status_code,
        errorTypeToString(details.type),
        details.message,
        details.details
    );
    
    crow::response resp(details.status_code, response.dump());
    resp.set_header("Content-Type", "application/json");
    return resp;
}

crow::response ErrorHandler::handleStdException(const std::exception& e) {
    std::cerr << "Standard exception: " << e.what() << std::endl;
    
    auto response = createErrorResponse(
        500,
        "internal_server_error",
        "Internal server error occurred",
        e.what()
    );
    
    crow::response resp(500, response.dump());
    resp.set_header("Content-Type", "application/json");
    return resp;
}

crow::response ErrorHandler::handleUnknownException() {
    std::cerr << "Unknown exception occurred" << std::endl;
    
    auto response = createErrorResponse(
        500,
        "unknown_error",
        "Unknown internal server error occurred"
    );
    
    crow::response resp(500, response.dump());
    resp.set_header("Content-Type", "application/json");
    return resp;
}

crow::response ErrorHandler::badRequest(const std::string& message, const std::string& details) {
    BadRequestException ex(message, details);
    return handleError(ex);
}

crow::response ErrorHandler::notFound(const std::string& message, const std::string& details) {
    NotFoundException ex(message, details);
    return handleError(ex);
}

crow::response ErrorHandler::internalError(const std::string& message, const std::string& details) {
    ApiException ex(ErrorDetails{ErrorType::INTERNAL_SERVER_ERROR, message, details, 500});
    return handleError(ex);
}

crow::response ErrorHandler::validationError(const std::string& message, const std::string& details) {
    ValidationException ex(message, details);
    return handleError(ex);
}

crow::response ErrorHandler::databaseError(const std::string& message, const std::string& details) {
    DatabaseException ex(message, details);
    return handleError(ex);
}

std::string ErrorHandler::errorTypeToString(ErrorType type) {
    switch (type) {
        case ErrorType::DATABASE_ERROR: return "database_error";
        case ErrorType::VALIDATION_ERROR: return "validation_error";
        case ErrorType::NOT_FOUND_ERROR: return "not_found_error";
        case ErrorType::BAD_REQUEST_ERROR: return "bad_request_error";
        case ErrorType::INTERNAL_SERVER_ERROR: return "internal_server_error";
        default: return "unknown_error";
    }
}

json ErrorHandler::createErrorResponse(int status_code, const std::string& error, 
                                     const std::string& message, const std::string& details) {
    json response;
    response["status"] = "error";
    response["error"] = error;
    response["message"] = message;
    
    if (!details.empty()) {
        response["details"] = details;
    }
    
    response["status_code"] = status_code;
    response["timestamp"] = std::to_string(std::time(nullptr));
    
    return response;
}

} // namespace error_handler