cmake_minimum_required(VERSION 3.15)
project(chat_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)

add_executable(chat_demo
    test.cpp
    provider.cpp
    knowledge_points.cpp
    batch_meta.cpp
    review.cpp
    retry.cpp
    checkpoint.cpp
    publish.cpp 
)

target_link_libraries(chat_demo PRIVATE
    CURL::libcurl
    nlohmann_json::nlohmann_json
)