#include <book_controller.h>
#include "error_handler.h"
// #include "error_handler.h"
// #include "book_validator.h"
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;

BookController::BookController(std::shared_ptr<BookService> book_service)
    : book_service_(book_service) {}

void BookController::setupRoutes(crow::SimpleApp& app) {
    // GET /api/books - получить все книги
    CROW_ROUTE(app, "/api/books")
    .methods("GET"_method)
    ([this]() {
        return handleGetAllBooks();
    });

    // GET /api/books/<int> - получить книгу по ID
    CROW_ROUTE(app, "/api/books/<int>")
    .methods("GET"_method)
    ([this](const crow::request& req, int id) {
        return handleGetBookById(req, id);
    });

    // POST /api/books - создать новую книгу
    CROW_ROUTE(app, "/api/books")
    .methods("POST"_method)
    ([this](const crow::request& req) {
        return handleCreateBook(req);
    });

    // PUT /api/books/<int> - обновить книгу
    CROW_ROUTE(app, "/api/books/<int>")
    .methods("PUT"_method)
    ([this](const crow::request& req, int id) {
        return handleUpdateBook(req, id);
    });

    // DELETE /api/books/<int> - удалить книгу
    CROW_ROUTE(app, "/api/books/<int>")
    .methods("DELETE"_method)
    ([this](int id) {
        return handleDeleteBook(id);
    });

    // GET /api/stats - получить статистику
    CROW_ROUTE(app, "/api/stats")
    .methods("GET"_method)
    ([this]() {
        return handleGetStats();
    });
}

crow::response BookController::handleGetBookById(const crow::request& req, int id) {
    try {
        json book = book_service_->getBookById(id);
        crow::response resp(book.dump());
        resp.set_header("Content-Type", "application/json");
        return resp;
        
    } catch (const error_handler::ApiException& e) {
        return error_handler::handle_error(e);
    } catch (const std::exception& e) {
        return error_handler::handle_std_exception(e);
    } catch (...) {
        return error_handler::handle_unknown_exception();
    }
}

crow::response BookController::handleDeleteBook(int id) {
    try {
        if (id <= 0) {
            return ErrorHandler::badRequest("Invalid book ID");
        }

        bool deleted = book_service_->deleteBook(id);
        if (!deleted) {
            return ErrorHandler::notFound("Book not found with ID: " + std::to_string(id));
        }

        crow::response resp(204); // No Content
        return resp;
        
    } catch (const std::exception& e) {
        std::cerr << "Error in handleDeleteBook: " << e.what() << std::endl;
        return ErrorHandler::internalError("Failed to delete book");
    }
}

crow::response BookController::handleGetAllBooks() {
    try {
        json books = book_service_->getAllBooks();
        crow::response resp(books.dump());
        resp.set_header("Content-Type", "application/json");
        return resp;
    } catch (const std::exception& e) {
        return crow::response(500, json{{"error", e.what()}}.dump());
    }
}

crow::response BookController::handleCreateBook(const crow::request& req) {
    try {
        auto book_data = json::parse(req.body);
        
        // Валидация
        if (!book_data.contains("title") || book_data["title"].is_null()) {
            throw error_handler::ValidationException(
                "Title is required",
                "Book title must be provided and cannot be null"
            );
        }
        
        if (!book_data.contains("author") || book_data["author"].is_null()) {
            throw error_handler::ValidationException(
                "Author is required", 
                "Book author must be provided and cannot be null"
            );
        }
        
        int book_id = book_service_->createBook(book_data);
        
        crow::response resp(201);
        resp.set_header("Location", "/api/books/" + std::to_string(book_id));
        return resp;
        
    } catch (const json::parse_error& e) {
        throw error_handler::BadRequestException(
            "Invalid JSON format",
            e.what()
        );
    } catch (const error_handler::ApiException& e) {
        return error_handler::handle_error(e);
    } catch (const std::exception& e) {
        return error_handler::handle_std_exception(e);
    } catch (...) {
        return error_handler::handle_unknown_exception();
    }
}

crow::response BookController::handleUpdateBook(const crow::request& req, int id) {
    try {
        auto book_data = json::parse(req.body);
        json updated_book = book_service_->updateBook(id, book_data);
        
        crow::response resp(updated_book.dump());
        resp.set_header("Content-Type", "application/json");
        return resp;
        
    } catch (const json::parse_error& e) {
        return crow::response(400, json{{"error", "Invalid JSON"}}.dump());
    } catch (const std::exception& e) {
        return crow::response(500, json{{"error", e.what()}}.dump());
    }
}

crow::response BookController::handleGetStats() {
    try {
        json stats = book_service_->getStats();
        
        crow::response resp(stats.dump());
        resp.set_header("Content-Type", "application/json");
        return resp;
        
    } catch (const std::exception& e) {
        std::cerr << "Error in handleGetStats: " << e.what() << std::endl;
        return ErrorHandler::internalError("Failed to retrieve statistics");
    }
}