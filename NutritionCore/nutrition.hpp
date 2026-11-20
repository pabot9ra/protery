#pragma once

#include <istream>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace protery {

struct Nutrients {
    double kcal = 0;
    double protein = 0;
};

struct Food {
    std::string name;
    Nutrients per100;
    double portionGrams;
    std::optional<double> pieceGrams = std::nullopt;
};

struct MealItem {
    std::string text;
    std::string foodName;
    double grams = 0;
    Nutrients nutrients;
};

class KnowledgeGraph {
public:
    explicit KnowledgeGraph(std::istream& input);
    const Food* find(std::string_view name) const;

private:
    std::vector<Food> foods_;
    std::unordered_map<std::string, std::size_t> foodByName_;
};

std::vector<MealItem> analyzeMeal(const KnowledgeGraph& graph, std::string_view phrase);

}
