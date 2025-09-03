#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

#include "application_builder.h"
#include "controller/book_controller.h"    
#include "service/book_service.h"          

using json = nlohmann::json;
// using json = nlohmann::json_abi_v3_11_2::json;

ApplicationBuilder::ApplicationBuilder() {
     // Получаем абсолютный путь к config.json относительно исполняемого файла
    std::filesystem::path exe_path = std::filesystem::current_path();
    std::filesystem::path config_path = exe_path.parent_path() / "config.json";
    config_path_ = config_path.string();
};

ApplicationBuilder& ApplicationBuilder::withConfigPath(const std::string& config_path) {
    config_path_ = config_path;
    return *this;
}

ApplicationBuilder& ApplicationBuilder::withConfig(const AppConfig& config) {
    custom_config_ = config;
    return *this;
}

AppComponents ApplicationBuilder::buildApplication(bool init_database) {
    // 1. Загрузка конфигурации
    if (custom_config_.has_value()) {
        config_ = custom_config_.value();
        std::cout << "Prepare: Using custom configuration." << std::endl;
    } else {
        std::cout << "Prepare: Loading configuration from: " << config_path_ << std::endl;
        config_ = loadConfigFromFile(config_path_);
    }

    // 2. Инициализация БД (если требуется)
    if (init_database) {
        std::cout << "Database initialization requested..." << std::endl;
        if (initializeDatabase(config_)) {
            std::cout << "Database initialized successfully!" << std::endl;
        } else {
            throw std::runtime_error("Database initialization failed.");
        }
    }

    // 3. Создание экземпляра приложения Crow
    auto app = std::make_unique<crow::SimpleApp>();

    // 4. Установка соединения с БД (уже к инициализированной базе)
    auto connection = establishDbConnection(config_);
    if (!connection) {
        throw std::runtime_error("Failed to establish database connection during application build.");
    }

    auto book_service = std::make_shared<BookService>(config_);
    auto controller = std::make_shared<BookController>(book_service);

    controller->setupRoutes(*app);
    // 5. Регистрация всех маршрутов
    registerRoutes(*app);

    std::cout << "Application built successfully. Server will run on port " << config_.server_port << "." << std::endl;
    return {std::move(app), controller};
}

bool ApplicationBuilder::initializeDatabase(const AppConfig& config) const {
    try {
        std::cout << "Step 1: Connecting to PostgreSQL server..." << std::endl;
        
        // Подключаемся к стандартной БД postgres для создания новой БД
        auto connection = establishDbConnection(config, "postgres");
        if (!connection || !connection->is_open()) {
            std::cerr << "Can't connect to PostgreSQL server" << std::endl;
            return false;
        }

        std::cout << "Connected to PostgreSQL server successfully!" << std::endl;
        
        // Проверяем, существует ли уже база данных bookshelf
        bool db_exists = false;
        try {
            pqxx::work check_txn(*connection);
            auto result = check_txn.exec_params(
                "SELECT 1 FROM pg_database WHERE datname = $1",
                config.db_name
            );
            db_exists = !result.empty();
            check_txn.commit();
        } catch (...) {
            // Игнорируем ошибки при проверке - возможно, базы нет
        }

        if (!db_exists) {
            std::cout << "Step 2: Creating database '" << config.db_name << "'..." << std::endl;
            connection->prepare("create_db", "CREATE DATABASE bookshelf");
            pqxx::nontransaction ntx(*connection);
            ntx.exec("CREATE DATABASE bookshelf");
            // pqxx::work create_txn(*connection);
            // create_txn.exec_params("CREATE DATABASE " + config.db_name);
            // create_txn.commit();
            std::cout << "Database '" << config.db_name << "' created successfully!" << std::endl;
        } else {
            std::cout << "Database '" << config.db_name << "' already exists." << std::endl;
        }

        connection->disconnect();

        // Теперь подключаемся к созданной/существующей базе данных для создания таблиц
        std::cout << "Step 3: Connecting to database '" << config.db_name << "'..." << std::endl;
        auto connection_to_bookshelf = establishDbConnection(config);
        if (!connection_to_bookshelf || !connection_to_bookshelf->is_open()) {
            std::cerr << "Can't connect to database '" << config.db_name << "'" << std::endl;
            return false;
        }

        std::cout << "Step 4: Creating tables..." << std::endl;
        pqxx::work txn_bookshelf(*connection_to_bookshelf);
        
         // SQL для создания таблицы books
        const char* create_table_sql = R"(
            CREATE TABLE books (
                id SERIAL PRIMARY KEY,
                title VARCHAR(255) NOT NULL,
                author VARCHAR(255) NOT NULL,
                year INTEGER,
                status VARCHAR(50) DEFAULT 'planned',
                rating INTEGER CHECK (rating >= 1 AND rating <= 5),
                review TEXT,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
        )";

        txn_bookshelf.exec(create_table_sql);
        
        // Создаем индексы (IF NOT EXISTS для идемпотентности)
        txn_bookshelf.exec("CREATE INDEX IF NOT EXISTS idx_books_author ON books(author)");
        txn_bookshelf.exec("CREATE INDEX IF NOT EXISTS idx_books_title ON books(title)");
        
        txn_bookshelf.commit();
        std::cout << "Table 'books' created/verified successfully!" << std::endl;
        
        return true;
        
    } catch (const std::exception &e) {
        std::cerr << "Database initialization error: " << e.what() << std::endl;
        return false;
    }
}

AppConfig ApplicationBuilder::loadConfigFromFile(const std::string& config_path) const {
    std::ifstream config_file(config_path);
    if (!config_file.is_open()) {
        throw std::runtime_error("Failed to open config file: " + config_path);
    }

    json config_json;
    config_file >> config_json;
    config_file.close();

    // Для отладки
    // std::cout << "DEBUG: Config JSON: " << config_json.dump(2) << std::endl;

    const auto& db_cfg = config_json["db_config"];
    AppConfig config;
    config.db_host = db_cfg.value("host", "localhost");
    config.db_port = db_cfg.value("port", "5432");
    config.db_name = db_cfg.value("dbname", "bookshelf");
    config.db_user = db_cfg.value("user", "postgres");
    config.db_password = db_cfg.value("password", "");
    config.server_port = config_json.value("server_port", 8080);

    // Для отладки
    // std::cout << "DEBUG: Connection string: " << config.get_connection_string() << std::endl;

    std::cerr << "Loading configuration successfully!" << std::endl;

    return config;
}

std::shared_ptr<pqxx::connection> ApplicationBuilder::establishDbConnection(const AppConfig& config, const std::string& dbname) const {
    try {
        std::string conn_string = config.get_connection_string(dbname);

        // Для отладки
        // std::cout << "DEBUG: Trying to connect with: " << conn_string << std::endl;

        auto connection = std::make_shared<pqxx::connection>(conn_string);
        if (connection->is_open()) {
            // Для отладки
            std::cout << "DEBUG: Successfully connected to database: " << dbname << std::endl;

            return connection;
        } else {
            std::cerr << "Database connection is closed." << std::endl;
            return nullptr;
        }
    } catch (const std::exception& e) {
        std::cerr << "Database connection failed: " << e.what() << std::endl;
        return nullptr;
    }
}

void ApplicationBuilder::registerRoutes(crow::SimpleApp& app) const {
    // Создаем фабрику соединений
    // auto connection_factory = [config = config_]() {
    //     return std::make_unique<pqxx::connection>(config.get_connection_string());
    // };
    // auto book_service = std::make_shared<BookService>(std::move(connection_factory));
    
    // Настраиваем маршруты
    // book_controller->setupRoutes(app);
    
    // Можно добавить health-check endpoint
    CROW_ROUTE(app, "/health")([](){
        return crow::response(200, "OK");
    });
}

AppConfig ApplicationBuilder::getConfig() const {
    return config_;
}

