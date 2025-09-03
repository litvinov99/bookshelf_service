#include "application_builder.h"
#include <iostream>
#include <cstring> // для strcmp

int main(int argc, char* argv[]) {
    bool init_database = false;
    
    // Проверяем аргументы командной строки
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--init-db") == 0 || strcmp(argv[i], "-i") == 0) {
            init_database = true;
        }
        // Можно добавить другие флаги, например для порта
        else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  -i, --init-db    Initialize database schema" << std::endl;
            std::cout << "  -h, --help       Show this help message" << std::endl;
            return 0;
        }
    }

    try {
        ApplicationBuilder builder;
        auto components = builder.buildApplication(init_database);
        
        // Получаем конфигурацию из билдера
        AppConfig config = builder.getConfig();
        
        std::cout << "Starting server on port " << config.server_port << "..." << std::endl;
        std::cout << "Database: " << config.db_name << " on " << config.db_host << ":" << config.db_port << std::endl;
        
        // Используем порт из конфигурации
        components.app->port(config.server_port).multithreaded().run();

    } catch (const std::exception& e) {
        std::cerr << "Fatal error during application startup: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}