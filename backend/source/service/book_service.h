#pragma once

#include <pqxx/pqxx>
#include <memory>
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class BookService {
public:
    explicit BookService(std::shared_ptr<pqxx::connection> connection);
    
    json getAllBooks();
    json getBookById(int id);
    int createBook(const json& book_data);
    json updateBook(int id, const json& book_data);
    bool deleteBook(int id);
    json getStats();
    
private:
    json rowToJson(const pqxx::row& row);
    std::shared_ptr<pqxx::connection> connection_;
};