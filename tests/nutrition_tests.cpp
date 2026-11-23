#include "nutrition.hpp"

#include <cmath>
#include <iostream>
#include <sstream>
#include <string_view>

int main() {
    std::istringstream catalog(
        "Гречка|гречки|110|4.2|150\n"
        "Яйцо|яйца;яиц|155|12.6|55|55\n"
        "Салат|три вкуса|100|1|150\n"
    );
    const protery::KnowledgeGraph graph(catalog);

    const struct {
        std::string_view phrase;
        std::string_view name;
        double grams;
    } cases[]{
        {"гречки", "Гречка", 150},
        {"200г гречки", "Гречка", 200},
        {"0,2 кг гречки", "Гречка", 200},
        {"2 яйца", "Яйцо", 110},
        {"1,5 шт. яйца", "Яйцо", 82.5},
        {"ДВА яйца", "Яйцо", 110},
        {"одну штуку яйца", "Яйцо", 55},
        {"полтора яйца", "Яйцо", 82.5},
        {"полторы штуки яйца", "Яйцо", 82.5},
        {"полтора килограмма гречки", "Гречка", 1500},
        {"двести граммов гречки", "Гречка", 200},
        {"сто пятнадцать граммов гречки", "Гречка", 115},
        {"девятьсот девяносто девять граммов гречки", "Гречка", 999},
        {"три вкуса", "Салат", 150},
        {"два гречки", "", 0},
        {"двадцать пятнадцать граммов гречки", "", 0},
        {"двести граммов", "", 0},
        {"два", "", 0},
        {"\xFF яйца", "", 0},
    };

    bool passed = true;
    for (const auto& test : cases) {
        const auto items = protery::analyzeMeal(graph, test.phrase);
        if (items.size() != 1 || items.front().foodName != test.name ||
            std::abs(items.front().grams - test.grams) > 1e-9) {
            std::cerr << "Неверный результат для фразы «" << test.phrase << "»\n";
            passed = false;
        }
    }
    return passed ? 0 : 1;
}
