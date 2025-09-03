#pragma once

#include <crow.h>
#include <pqxx/pqxx>
#include <memory>
#include <string>
#include <optional>

class BookController;
class BookService;
class AppConfig;

// namespace nlohmann { class json; }

struct AppComponents {
    std::unique_ptr<crow::SimpleApp> app;
    std::shared_ptr<BookController> controller;
};

struct AppConfig {
    std::string db_host;
    std::string db_port;
    std::string db_name;
    std::string db_user;
    std::string db_password;
    int server_port;
    
    // Добавляем метод для получения строки подключения
    std::string get_connection_string(const std::string& dbname = "") const {
        std::string target_db = dbname.empty() ? db_name : dbname;
        return "host=" + db_host +
               " port=" + db_port +
               " dbname=" + target_db +
               " user=" + db_user +
               " password=" + db_password;
    }
};

class ApplicationBuilder {
public:
    ApplicationBuilder();

    // Основной метод построения приложения
    AppComponents buildApplication(bool init_database = false);

    // Методы для кастомизации
    ApplicationBuilder& withConfigPath(const std::string& config_path);
    ApplicationBuilder& withConfig(const AppConfig& config);

    // Добавляем метод для получения конфигурации
    AppConfig getConfig() const;

private:
    // Вспомогательные методы
    AppConfig loadConfigFromFile(const std::string& config_path) const;
    std::shared_ptr<pqxx::connection> establishDbConnection(const AppConfig& config, const std::string& dbname = "") const;
    void registerRoutes(crow::SimpleApp& app) const;
    
    // Новая функция инициализации БД
    bool initializeDatabase(const AppConfig& config) const;

    // std::string config_path_ = "config.json";
    std::string config_path_;
    std::optional<AppConfig> custom_config_;
    AppConfig config_; // Добавляем поле для хранения конфига
};