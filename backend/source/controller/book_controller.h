#pragma once

#include <crow.h>
#include <memory>

class BookService;

class BookController {
public:
    explicit BookController(std::shared_ptr<BookService> book_service);
    
    void setupRoutes(crow::SimpleApp& app);
    
private:
    std::shared_ptr<BookService> book_service_;
    
    // Обработчики запросов
    crow::response handleGetAllBooks();
    crow::response handleGetBookById(int id);
    crow::response handleCreateBook(const crow::request& req);
    crow::response handleUpdateBook(const crow::request& req, int id);
    crow::response handleDeleteBook(int id);
    crow::response handleGetStats();
};