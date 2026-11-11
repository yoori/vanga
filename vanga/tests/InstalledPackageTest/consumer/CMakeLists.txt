cmake_minimum_required(VERSION 3.20)
project(VangaConsumer LANGUAGES CXX)

find_package(Vanga CONFIG REQUIRED)
find_package(Vanga CONFIG REQUIRED)

add_executable(vanga_consumer main.cpp)
target_link_libraries(vanga_consumer PRIVATE Vanga::DTree)
set_target_properties(vanga_consumer PROPERTIES CXX_EXTENSIONS OFF)

enable_testing()
add_test(NAME Prediction COMMAND vanga_consumer)
