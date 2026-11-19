#include "nutrition.hpp"

#include <algorithm>
#include <cmath>
#include <locale>
#include <optional>
#include <sstream>
#include <stdexcept>

namespace protery {
namespace {

std::string normalize(std::string_view text) {
    const std::string_view uppercase = "АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯЁ";
    const std::string_view lowercase = "абвгдежзийклмнопрстуфхцчшщъыьэюяе";
    std::string result;

    for (std::size_t i = 0; i < text.size(); ++i) {
        const char current = text[i];
        if (std::string_view(" \t\r\n").find(current) != std::string_view::npos) {
            if (!result.empty() && result.back() != ' ') {
                result += ' ';
            }
            continue;
        }

        // Русские буквы занимают два байта в UTF-8.
        if (i + 1 < text.size()) {
            const auto letter = text.substr(i, 2);
            const auto position = uppercase.find(letter);
            if (position != std::string_view::npos && position % 2 == 0) {
                result += lowercase.substr(position, 2);
                ++i;
                continue;
            }
            if (letter == "ё") {
                result += "е";
                ++i;
                continue;
            }
        }
        result += current;
    }
    if (!result.empty() && result.back() == ' ') {
        result.pop_back();
    }
    return result;
}

std::optional<double> number(std::string value) {
    std::replace(value.begin(), value.end(), ',', '.');
    std::istringstream input(value);
    input.imbue(std::locale::classic());

    double result;
    if (!(input >> result) || !input.eof() || !std::isfinite(result) || result < 0) {
        return std::nullopt;
    }
    return result;
}

std::vector<std::string> split(std::string_view text, std::string_view delimiter) {
    std::vector<std::string> parts;
    auto end = text.find(delimiter);
    while (end != std::string_view::npos) {
        parts.emplace_back(text.substr(0, end));
        text.remove_prefix(end + delimiter.size());
        end = text.find(delimiter);
    }
    parts.emplace_back(text);
    return parts;
}

struct FoodQuery {
    std::string name;
    std::optional<double> grams;
};

FoodQuery parseItem(const std::string& text) {
    const auto amountEnd = text.find_first_not_of("0123456789.,");
    if (amountEnd == 0 || amountEnd == std::string::npos) {
        return {text, std::nullopt};
    }
    const auto amount = number(text.substr(0, amountEnd));
    if (!amount || *amount <= 0) {
        return {text, std::nullopt};
    }

    std::string remainder = normalize(std::string_view(text).substr(amountEnd));
    const auto unitEnd = remainder.find(' ');
    if (unitEnd == std::string::npos) {
        return {text, std::nullopt};
    }

    const auto unit = remainder.substr(0, unitEnd);
    if (unit == "г") {
        return {remainder.substr(unitEnd + 1), amount};
    }
    if (unit == "кг" && std::isfinite(*amount * 1000)) {
        return {remainder.substr(unitEnd + 1), *amount * 1000};
    }
    return {text, std::nullopt};
}

}

KnowledgeGraph::KnowledgeGraph(std::istream& input) {
    std::string line;
    std::size_t lineNumber = 0;
    while (std::getline(input, line)) {
        ++lineNumber;
        if (normalize(line).empty() || line.front() == '#') {
            continue;
        }

        const auto fields = split(line, "|");
        const auto location = "Строка " + std::to_string(lineNumber);
        if (fields.size() != 5 || normalize(fields[0]).empty()) {
            throw std::runtime_error(location + ": ожидаются 5 полей и название продукта");
        }

        const auto kcal = number(normalize(fields[2]));
        const auto protein = number(normalize(fields[3]));
        const auto portion = number(normalize(fields[4]));
        if (!kcal || !protein || !portion || *portion <= 0) {
            throw std::runtime_error(location + ": неверные пищевые значения или размер порции");
        }

        auto names = split(fields[1], ";");
        names.push_back(fields[0]);
        for (const auto& name : names) {
            const auto key = normalize(name);
            if (!key.empty() && !foodByName_.emplace(key, foods_.size()).second) {
                throw std::runtime_error(location + ": повторяющееся название «" + key + "»");
            }
        }
        foods_.push_back({fields[0], {*kcal, *protein}, *portion});
    }
    if (input.bad() || foods_.empty()) {
        throw std::runtime_error("Не удалось прочитать каталог продуктов");
    }
}

const Food* KnowledgeGraph::find(std::string_view name) const {
    const auto match = foodByName_.find(normalize(name));
    if (match == foodByName_.end()) {
        return nullptr;
    }
    return &foods_[match->second];
}

std::vector<MealItem> analyzeMeal(const KnowledgeGraph& graph, std::string_view phrase) {
    std::vector<MealItem> items;
    for (const auto& text : split(normalize(phrase), " и ")) {
        if (text.empty()) {
            continue;
        }

        const auto* food = graph.find(text);
        std::optional<double> explicitGrams;
        if (!food) {
            const auto query = parseItem(text);
            food = graph.find(query.name);
            explicitGrams = query.grams;
        }
        if (!food) {
            items.push_back({text, "", 0, {}});
            continue;
        }

        const double grams = explicitGrams.value_or(food->portionGrams);
        const double factor = grams / 100;
        const Nutrients nutrients{food->per100.kcal * factor, food->per100.protein * factor};
        if (!std::isfinite(nutrients.kcal) || !std::isfinite(nutrients.protein)) {
            items.push_back({text, "", 0, {}});
            continue;
        }
        items.push_back({text, food->name, grams, nutrients});
    }
    return items;
}

}
