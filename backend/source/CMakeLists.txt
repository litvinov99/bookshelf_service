# cmake_minimum_required(VERSION 3.10)
# project(bookshelf_backend)

# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_BUILD_TYPE Debug)

# if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
#     add_compile_options(-g -O0 -Wall -Wextra -pedantic)
# endif()

# # Правильные пути к submodules (они в third_party/)
# include_directories(
#     ${CMAKE_SOURCE_DIR}/third_party/crow/include
#     ${CMAKE_SOURCE_DIR}/third_party/nlohmann/include
#     ${CMAKE_SOURCE_DIR}                 
#     ${CMAKE_SOURCE_DIR}/builder         
#     ${CMAKE_SOURCE_DIR}/service         
#     ${CMAKE_SOURCE_DIR}/error_handler
#     ${CMAKE_SOURCE_DIR}/controller     
# )

# # Добавляем Crow библиотеку из third_party
# add_subdirectory(third_party/crow)

# # Поиск других библиотек
# find_package(PostgreSQL REQUIRED)
# find_package(PkgConfig REQUIRED)
# pkg_check_modules(PQXX REQUIRED libpqxx)

# # ПРАВИЛЬНЫЕ пути к исходным файлам (они в корне source/)
# set(SOURCES
#     main.cpp                
#     builder/application_builder.cpp
#     service/book_service.cpp
#     error_handler/error_handler.cpp
#     controller/book_controller.cpp
# )

# add_executable(bookshelf_api ${SOURCES})

# # Связываем библиотеки
# target_link_libraries(bookshelf_api 
#     Crow::Crow
#     PostgreSQL::PostgreSQL
#     ${PQXX_LIBRARIES}
#     pthread
# )

# set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

# # Обновленный путь к config.json (он в корне source/)
# if(EXISTS ${CMAKE_SOURCE_DIR}/config.json)
#     configure_file(${CMAKE_SOURCE_DIR}/config.json ${CMAKE_BINARY_DIR}/config.json COPYONLY)
# else()
#     message(WARNING "config.json not found in source directory")
# endif()

cmake_minimum_required(VERSION 3.10)
project(bookshelf_backend VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Установка типа сборки только для одноконфигурационных генераторов
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Флаги компиляции
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-g -O0 -Wall -Wextra -pedantic -Werror)
    add_compile_definitions(_GLIBCXX_ASSERTIONS)
elseif(MSVC)
    add_compile_options(/W4 /WX /EHsc)
endif()

# Добавляем Crow библиотеку
add_subdirectory(third_party/crow)

# Создаем интерфейсную цель для nlohmann/json
add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE 
    ${CMAKE_SOURCE_DIR}/third_party/nlohmann/include
)

# Поиск зависимостей
find_package(PostgreSQL REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
pkg_check_modules(PQXX REQUIRED libpqxx)

# Исходные файлы
set(SOURCES
    main.cpp
    builder/application_builder.cpp
    service/book_service.cpp
    error_handler/error_handler.cpp
    controller/book_controller.cpp
)

add_executable(bookshelf_api ${SOURCES})

# Подключение include директорий
target_include_directories(bookshelf_api PUBLIC
    ${CMAKE_SOURCE_DIR}/third_party/crow/include
    ${CMAKE_SOURCE_DIR}
)

target_include_directories(bookshelf_api PRIVATE
    ${CMAKE_SOURCE_DIR}/builder
    ${CMAKE_SOURCE_DIR}/service
    ${CMAKE_SOURCE_DIR}/error_handler
    ${CMAKE_SOURCE_DIR}/controller
)

# Линковка
target_link_libraries(bookshelf_api 
    Crow::Crow
    PostgreSQL::PostgreSQL
    ${PQXX_LIBRARIES}
    Threads::Threads
    nlohmann_json
)

# Выходная директория
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

# Копирование config.json
if(EXISTS ${CMAKE_SOURCE_DIR}/config.json)
    configure_file(${CMAKE_SOURCE_DIR}/config.json ${CMAKE_BINARY_DIR}/config.json COPYONLY)
else()
    message(WARNING "config.json not found in source directory")
endif()

# Добавьте установку
install(TARGETS bookshelf_api DESTINATION bin)
install(FILES ${CMAKE_BINARY_DIR}/config.json DESTINATION .)