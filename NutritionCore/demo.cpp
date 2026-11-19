#include "nutrition.hpp"

#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Использование: protery_demo <каталог> \"<фраза>\"\n";
        return 1;
    }

    std::ifstream catalog(argv[1]);
    if (!catalog) {
        std::cerr << "Не удалось открыть каталог: " << argv[1] << '\n';
        return 1;
    }

    try {
        const protery::KnowledgeGraph graph(catalog);
        const auto items = protery::analyzeMeal(graph, argv[2]);
        if (items.empty()) {
            std::cerr << "Введите название продукта.\n";
            return 1;
        }

        protery::Nutrients total;
        std::size_t matched = 0;
        std::cout << std::fixed << std::setprecision(1);
        for (const auto& item : items) {
            if (item.foodName.empty()) {
                std::cout << "Не распознано: " << item.text << '\n';
                continue;
            }
            ++matched;
            total.kcal += item.nutrients.kcal;
            total.protein += item.nutrients.protein;
            std::cout << item.foodName << " — " << item.grams << " г, "
                      << item.nutrients.kcal << " ккал, белок " << item.nutrients.protein << " г\n";
        }

        if (matched > 0) {
            std::cout << "Итого по распознанным: " << total.kcal << " ккал, белок "
                      << total.protein << " г\n";
        }
        return matched == items.size() ? 0 : 2;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
