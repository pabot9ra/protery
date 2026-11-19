cmake_minimum_required(VERSION 3.20)
project(Protery LANGUAGES CXX)

add_library(NutritionCore NutritionCore/nutrition.cpp)
target_compile_features(NutritionCore PUBLIC cxx_std_17)
target_include_directories(NutritionCore PUBLIC NutritionCore)

add_executable(protery_demo NutritionCore/demo.cpp)
target_link_libraries(protery_demo PRIVATE NutritionCore)
