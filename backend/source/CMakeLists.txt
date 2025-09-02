# cmake_minimum_required(VERSION 3.10)
# project(bookshelf_backend)

# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)

# if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
#     add_compile_options(-Wall -Wextra -pedantic)
# endif()

# set(SOURCE_DIR source)

# include_directories(
#     ${SOURCE_DIR}
#     ${SOURCE_DIR}/builder
#     ${SOURCE_DIR}/service
#     ${SOURCE_DIR}/controller
#     # ${SOURCE_DIR}/service/error_handler
#     # ${SOURCE_DIR}/service/book_validator

#     # Пути для crow, nlohmann/json, pqxx
#     ${SOURCE_DIR}/third_party/crow
#     ${SOURCE_DIR}/third_party/nlohmann

#     # ${SOURCE_DIR}/third_party/nlohmann/include

#     /usr/include/postgresql
    
#     # ${SOURCE_DIR}/third_party/pqxx
#     # ${SOURCE_DIR}/third_party/pqxx/internal
#     # ${SOURCE_DIR}/third_party/pqxx/internal/gates
# )
# message(STATUS "Include directories: ${CMAKE_INCLUDE_DIRECTORIES}")

# find_package(PostgreSQL REQUIRED)

# set(SOURCES
#     ${SOURCE_DIR}/main.cpp
#     ${SOURCE_DIR}/builder/application_builder.cpp
#     ${SOURCE_DIR}/service/book_service.cpp
#     ${SOURCE_DIR}/controller/book_controller.cpp
#     # ${SOURCE_DIR}/service/error_handler/error_handler.cpp
#     # ${SOURCE_DIR}/service/book_validator/book_validator.cpp
# )

# add_executable(bookshelf_api ${SOURCES})

# target_link_libraries(bookshelf_api 
#     PostgreSQL::PostgreSQL
#     pthread
# )

# if(WIN32)
#     target_link_libraries(bookshelf_api ws2_32 wsock32)
# endif()

# set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

# if(EXISTS ${CMAKE_SOURCE_DIR}/config.json)
#     configure_file(${CMAKE_SOURCE_DIR}/config.json ${CMAKE_BINARY_DIR}/config.json COPYONLY)
# else()
#     message(WARNING "config.json not found in source directory")
# endif()

cmake_minimum_required(VERSION 3.10)
project(bookshelf_backend)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-g -O0 -Wall -Wextra -pedantic)
endif()

# Правильные пути к submodules (они в third_party/)
include_directories(
    ${CMAKE_SOURCE_DIR}/third_party/crow/include
    ${CMAKE_SOURCE_DIR}/third_party/nlohmann/include
    ${CMAKE_SOURCE_DIR}                 
    ${CMAKE_SOURCE_DIR}/builder         
    ${CMAKE_SOURCE_DIR}/service         
    ${CMAKE_SOURCE_DIR}/error_handler
    ${CMAKE_SOURCE_DIR}/controller     
)

# Добавляем Crow библиотеку из third_party
add_subdirectory(third_party/crow)

# Поиск других библиотек
find_package(PostgreSQL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED libpqxx)

# ПРАВИЛЬНЫЕ пути к исходным файлам (они в корне source/)
set(SOURCES
    main.cpp                # В корне source/
    builder/application_builder.cpp
    service/book_service.cpp
    error_handler/error_handler.cpp
    controller/book_controller.cpp
)

add_executable(bookshelf_api ${SOURCES})

# Связываем библиотеки
target_link_libraries(bookshelf_api 
    Crow::Crow
    PostgreSQL::PostgreSQL
    ${PQXX_LIBRARIES}
    pthread
)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

# Обновленный путь к config.json (он в корне source/)
if(EXISTS ${CMAKE_SOURCE_DIR}/config.json)
    configure_file(${CMAKE_SOURCE_DIR}/config.json ${CMAKE_BINARY_DIR}/config.json COPYONLY)
else()
    message(WARNING "config.json not found in source directory")
endif()