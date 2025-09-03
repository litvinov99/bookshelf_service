#include "service/book_service.h"
#include "error_handler.h"

#include <pqxx/pqxx>
#include <nlohmann/json.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <optional>

using json = nlohmann::json;

BookService::BookService(const AppConfig& config)
    : config_(config) {}

BookService::BookService(ConnectionFactory connection_factory)
    : connection_factory_(std::move(connection_factory)) {
        // ДЛЯ ОТЛАДКИ
        // std::cout << "BookService created, factory is " 
        //         << (connection_factory_ ? "valid" : "invalid") << std::endl;
        
        // // Тестируем фабрику сразу
        // try {
        //     auto test_conn = connection_factory_();
        //     std::cout << "Test connection: " 
        //             << (test_conn && test_conn->is_open() ? "success" : "failed") 
        //             << std::endl;
        // } catch (const std::exception& e) {
        //     std::cerr << "Factory test failed: " << e.what() << std::endl;
        // }
    }

json BookService::getAllBooks() {
    try {
        pqxx::work txn(*connection_);
        pqxx::result result = txn.exec(
            "SELECT id, title, author, year, status, rating, review, created_at, updated_at "
            "FROM books ORDER BY created_at DESC"
        );
        txn.commit();

        json books = json::array();
        for (const auto& row : result) {
            books.push_back(rowToJson(row));
        }
        return books;

    } catch (const std::exception& e) {
        throw std::runtime_error("Database error in GetAllBooks: " + std::string(e.what()));
    }
}

json BookService::getBookById(int id) {
    try {
        pqxx::work txn(*connection_);
        pqxx::result result = txn.exec_params(
            "SELECT * FROM books WHERE id = $1", id
        );
        
        if (result.empty()) {
            throw error_handler::NotFoundException(
                "Book not found", 
                "Book with id " + std::to_string(id) + " does not exist"
            );
        }
        
        return rowToJson(result[0]);
        
    } catch (const pqxx::sql_error& e) {
        throw error_handler::DatabaseException(
            "Database query failed",
            e.what()
        );
    }
}

int BookService::createBook(const json& book_data) {
    try {
        pqxx::work txn(*connection_);
        
        // Обработка NULL для year
        std::optional<int> year_opt;
        if (!book_data["year"].is_null()) {
            year_opt = book_data["year"].get<int>();
        }

        pqxx::result result;
        if (year_opt.has_value()) {
            result = txn.exec_params(
                "INSERT INTO books (title, author, year, status) "
                "VALUES ($1, $2, $3, $4) RETURNING id",
                book_data["title"].get<std::string>(),
                book_data["author"].get<std::string>(),
                year_opt.value(),
                book_data.value("status", "planned")
            );
        } else {
            result = txn.exec_params(
                "INSERT INTO books (title, author, status) "
                "VALUES ($1, $2, $3) RETURNING id",
                book_data["title"].get<std::string>(),
                book_data["author"].get<std::string>(),
                book_data.value("status", "planned")
            );
        }
        txn.commit();

        return result[0]["id"].as<int>();

    } catch (const std::exception& e) {
        throw std::runtime_error("Database error in CreateBook: " + std::string(e.what()));
    }
}

// json BookService::updateBook(int id, const json& book_data) {
//     try {
//         pqxx::work txn(*connection_);
        
//         // Простая реализация - обновляем каждое поле отдельно, если оно присутствует
//         if (book_data.contains("title")) {
//             txn.exec_params(
//                 "UPDATE books SET title = $1, updated_at = CURRENT_TIMESTAMP WHERE id = $2",
//                 book_data["title"].get<std::string>(),
//                 id
//             );
//         }
//         if (book_data.contains("author")) {
//             txn.exec_params(
//                 "UPDATE books SET author = $1, updated_at = CURRENT_TIMESTAMP WHERE id = $2",
//                 book_data["author"].get<std::string>(),
//                 id
//             );
//         }
//         if (book_data.contains("year")) {
//             if (book_data["year"].is_null()) {
//                 txn.exec_params(
//                     "UPDATE books SET year = NULL, updated_at = CURRENT_TIMESTAMP WHERE id = $1",
//                     id
//                 );
//             } else {
//                 txn.exec_params(
//                     "UPDATE books SET year = $1, updated_at = CURRENT_TIMESTAMP WHERE id = $2",
//                     book_data["year"].get<int>(),
//                     id
//                 );
//             }
//         }
//         if (book_data.contains("status")) {
//             txn.exec_params(
//                 "UPDATE books SET status = $1, updated_at = CURRENT_TIMESTAMP WHERE id = $2",
//                 book_data["status"].get<std::string>(),
//                 id
//             );
//         }
//         if (book_data.contains("rating")) {
//             if (book_data["rating"].is_null()) {
//                 txn.exec_params(
//                     "UPDATE books SET rating = NULL, updated_at = CURRENT_TIMESTAMP WHERE id = $1",
//                     id
//                 );
//             } else {
//                 txn.exec_params(
//                     "UPDATE books SET rating = $1, updated_at = CURRENT_TIMESTAMP WHERE id = $2",
//                     book_data["rating"].get<int>(),
//                     id
//                 );
//             }
//         }
//         if (book_data.contains("review")) {
//             txn.exec_params(
//                 "UPDATE books SET review = $1, updated_at = CURRENT_TIMESTAMP WHERE id = $2",
//                 book_data["review"].get<std::string>(),
//                 id
//             );
//         }
        
//         txn.commit();
//         return 

//     } catch (const std::exception& e) {
//         throw std::runtime_error("Database error in UpdateBook: " + std::string(e.what()));
//     }
// }

bool BookService::deleteBook(int id) {
    try {
        pqxx::work txn(*connection_);
        txn.exec_params("DELETE FROM books WHERE id = $1", id);
        txn.commit();
        return true;

    } catch (const std::exception& e) {
        throw std::runtime_error("Database error in DeleteBook: " + std::string(e.what()));
    }
    return false;
}

json BookService::rowToJson(const pqxx::row& row) {
    json book;
    book["id"] = row["id"].as<int>();
    book["title"] = row["title"].as<std::string>();
    book["author"] = row["author"].as<std::string>();
    
    // Правильная обработка NULL значений
    if (row["year"].is_null()) {
        book["year"] = nullptr;
    } else {
        book["year"] = row["year"].as<int>();
    }
    
    book["status"] = row["status"].as<std::string>();
    
    if (row["rating"].is_null()) {
        book["rating"] = nullptr;
    } else {
        book["rating"] = row["rating"].as<int>();
    }
    
    if (row["review"].is_null()) {
        book["review"] = "";
    } else {
        book["review"] = row["review"].as<std::string>();
    }
    
    book["created_at"] = row["created_at"].as<std::string>();
    book["updated_at"] = row["updated_at"].as<std::string>();

    return book;
}

json BookService::updateBook(int id, const json& book_data) {
    try {
        pqxx::work txn(*connection_);
        
        // Простая и надежная реализация - отдельные запросы для каждого поля
        if (book_data.contains("title")) {
            txn.exec_params(
                "UPDATE books SET title = $1, updated_at = CURRENT_TIMESTAMP WHERE id = $2",
                book_data["title"].get<std::string>(),
                id
            );
        }
        if (book_data.contains("author")) {
            txn.exec_params(
                "UPDATE books SET author = $1, updated_at = CURRENT_TIMESTAMP WHERE id = $2",
                book_data["author"].get<std::string>(),
                id
            );
        }
        if (book_data.contains("year")) {
            if (book_data["year"].is_null()) {
                txn.exec_params(
                    "UPDATE books SET year = NULL, updated_at = CURRENT_TIMESTAMP WHERE id = $1",
                    id
                );
            } else {
                txn.exec_params(
                    "UPDATE books SET year = $1, updated_at = CURRENT_TIMESTAMP WHERE id = $2",
                    book_data["year"].get<int>(),
                    id
                );
            }
        }
        if (book_data.contains("status")) {
            txn.exec_params(
                "UPDATE books SET status = $1, updated_at = CURRENT_TIMESTAMP WHERE id = $2",
                book_data["status"].get<std::string>(),
                id
            );
        }
        if (book_data.contains("rating")) {
            if (book_data["rating"].is_null()) {
                txn.exec_params(
                    "UPDATE books SET rating = NULL, updated_at = CURRENT_TIMESTAMP WHERE id = $1",
                    id
                );
            } else {
                txn.exec_params(
                    "UPDATE books SET rating = $1, updated_at = CURRENT_TIMESTAMP WHERE id = $2",
                    book_data["rating"].get<int>(),
                    id
                );
            }
        }
        if (book_data.contains("review")) {
            txn.exec_params(
                "UPDATE books SET review = $1, updated_at = CURRENT_TIMESTAMP WHERE id = $2",
                book_data["review"].get<std::string>(),
                id
            );
        }
        
        txn.commit();

        // Возвращаем обновленную книгу
        return getBookById(id);

    } catch (const std::exception& e) {
        throw std::runtime_error("Database error in UpdateBook: " + std::string(e.what()));
    }
}

json BookService::getStats() {
    try {
        // auto connection = connection_factory_();

        pqxx::connection connection(config_.get_connection_string());
        pqxx::work txn(connection);

        // pqxx::work txn(*connection);
        // pqxx::work txn(*connection_);
        
        pqxx::result status_result = txn.exec(
            "SELECT status, COUNT(*) as count FROM books GROUP BY status"
        );
        
        pqxx::result rating_result = txn.exec(
            "SELECT AVG(rating) as avg_rating FROM books WHERE rating IS NOT NULL"
        );
        
        pqxx::result total_result = txn.exec("SELECT COUNT(*) as total FROM books");
        
        txn.commit();

        json stats;
        stats["by_status"] = json::object();
        
        for (const auto& row : status_result) {
            stats["by_status"][row["status"].as<std::string>()] = row["count"].as<int>();
        }
        
        if (rating_result[0]["avg_rating"].is_null()) {
            stats["average_rating"] = nullptr;
        } else {
            stats["average_rating"] = rating_result[0]["avg_rating"].as<double>();
        }
            
        stats["total_books"] = total_result[0]["total"].as<int>();

        return stats;

    } catch (const std::exception& e) {
        throw std::runtime_error("Database error in GetStats: " + std::string(e.what()));
    }
}