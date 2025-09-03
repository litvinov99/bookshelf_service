#pragma once

#include <pqxx/pqxx>
#include <memory>
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// class AppConfig;
#include "application_builder.h"

class BookService {
public:
    explicit BookService(const AppConfig& config);

    using ConnectionFactory = std::function<std::unique_ptr<pqxx::connection>()>;
    explicit BookService(ConnectionFactory connection_factory);
    
    json getAllBooks();
    json getBookById(int id);
    int createBook(const json& book_data);
    json updateBook(int id, const json& book_data);
    bool deleteBook(int id);
    json getStats();
    
private:
    json rowToJson(const pqxx::row& row);
    
    std::shared_ptr<pqxx::connection> connection_;
    ConnectionFactory connection_factory_;
    AppConfig config_;
};