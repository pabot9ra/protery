cmake_minimum_required(VERSION 3.20)
project(Protery LANGUAGES CXX)

add_library(NutritionCore NutritionCore/nutrition.cpp)
target_compile_features(NutritionCore PUBLIC cxx_std_17)
target_include_directories(NutritionCore PUBLIC NutritionCore)

add_executable(protery_demo NutritionCore/demo.cpp)
target_link_libraries(protery_demo PRIVATE NutritionCore)

include(CTest)
if(BUILD_TESTING)
    add_executable(protery_tests tests/nutrition_tests.cpp)
    target_link_libraries(protery_tests PRIVATE NutritionCore)
    add_test(NAME nutrition_quantities COMMAND protery_tests)
endif()
