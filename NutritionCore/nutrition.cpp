#include "nutrition.hpp"

#include <algorithm>
#include <cmath>
#include <locale>
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

enum class Unit {
    Grams,
    Kilograms,
    Pieces,
};

struct FoodQuery {
    std::string name;
    double amount;
    Unit unit;
};

struct WrittenAmount {
    double value;
    std::size_t end;
};

WrittenAmount parseWrittenAmount(std::string_view text) {
    static const std::unordered_map<std::string_view, int> values{
        {"один", 1}, {"одна", 1}, {"одно", 1}, {"одну", 1}, {"два", 2}, {"две", 2},
        {"три", 3}, {"четыре", 4}, {"пять", 5}, {"шесть", 6}, {"семь", 7},
        {"восемь", 8}, {"девять", 9}, {"десять", 10}, {"одиннадцать", 11},
        {"двенадцать", 12}, {"тринадцать", 13}, {"четырнадцать", 14}, {"пятнадцать", 15},
        {"шестнадцать", 16}, {"семнадцать", 17}, {"восемнадцать", 18}, {"девятнадцать", 19},
        {"двадцать", 20}, {"тридцать", 30}, {"сорок", 40}, {"пятьдесят", 50},
        {"шестьдесят", 60}, {"семьдесят", 70}, {"восемьдесят", 80}, {"девяносто", 90},
        {"сто", 100}, {"двести", 200}, {"триста", 300}, {"четыреста", 400},
        {"пятьсот", 500}, {"шестьсот", 600}, {"семьсот", 700}, {"восемьсот", 800},
        {"девятьсот", 900},
    };

    auto end = text.find(' ');
    const auto firstWord = text.substr(0, end);
    if (firstWord == "полтора" || firstWord == "полторы") {
        return {1.5, end};
    }

    // После сотен идут десятки или 1–19, после десятков — только 1–9.
    WrittenAmount amount{0, 0};
    int limit = 1000;
    std::size_t start = 0;
    // Последнее слово оставляем для имени продукта или единицы.
    while (end != std::string_view::npos) {
        const auto word = values.find(text.substr(start, end - start));
        if (word == values.end() || word->second >= limit) {
            break;
        }
        const int part = word->second;
        amount.value += part;
        amount.end = end;
        if (part < 20) {
            break;
        }
        limit = part >= 100 ? 100 : 10;
        start = end + 1;
        end = text.find(' ', start);
    }
    return amount;
}

std::optional<FoodQuery> parseItem(const std::string& text) {
    auto amountEnd = text.find_first_not_of("0123456789.,");
    std::optional<double> amount;
    if (amountEnd == 0) {
        const auto written = parseWrittenAmount(text);
        amountEnd = written.end;
        amount = written.value;
    } else if (amountEnd != std::string::npos) {
        amount = number(text.substr(0, amountEnd));
    }
    if (!amount || *amount <= 0 || amountEnd == std::string::npos) {
        return std::nullopt;
    }

    const auto remainder = normalize(std::string_view(text).substr(amountEnd));
    const auto unitEnd = remainder.find(' ');
    const auto unit = remainder.substr(0, unitEnd);
    if (unitEnd != std::string::npos) {
        const auto name = remainder.substr(unitEnd + 1);
        if (unit == "г" || unit == "грамм" || unit == "грамма" || unit == "граммов") {
            return FoodQuery{name, *amount, Unit::Grams};
        }
        if (unit == "кг" || unit == "килограмм" || unit == "килограмма" || unit == "килограммов") {
            return FoodQuery{name, *amount, Unit::Kilograms};
        }
        if (unit == "шт" || unit == "шт." || unit == "штука" || unit == "штуку" ||
            unit == "штуки" || unit == "штук") {
            return FoodQuery{name, *amount, Unit::Pieces};
        }
    }

    // В «2 яйца» слово «штуки» подразумевается.
    if (text[amountEnd] == ' ') {
        return FoodQuery{remainder, *amount, Unit::Pieces};
    }
    return std::nullopt;
}

std::optional<double> weightInGrams(const FoodQuery& query, const Food& food) {
    switch (query.unit) {
        case Unit::Grams:
            return query.amount;
        case Unit::Kilograms:
            return query.amount * 1000;
        case Unit::Pieces:
            if (food.pieceGrams) {
                return query.amount * *food.pieceGrams;
            }
            return std::nullopt;
    }
    return std::nullopt;
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
        if ((fields.size() != 5 && fields.size() != 6) || normalize(fields[0]).empty()) {
            throw std::runtime_error(location + ": ожидаются 5 или 6 полей, первое — название продукта");
        }

        const auto kcal = number(normalize(fields[2]));
        const auto protein = number(normalize(fields[3]));
        const auto portion = number(normalize(fields[4]));
        if (!kcal || !protein || !portion || *portion <= 0) {
            throw std::runtime_error(location + ": неверные пищевые значения или размер порции");
        }

        std::optional<double> pieceGrams;
        if (fields.size() == 6 && !normalize(fields[5]).empty()) {
            pieceGrams = number(normalize(fields[5]));
            if (!pieceGrams || *pieceGrams <= 0) {
                throw std::runtime_error(location + ": неверный вес одной штуки");
            }
        }

        auto names = split(fields[1], ";");
        names.push_back(fields[0]);
        for (const auto& name : names) {
            const auto key = normalize(name);
            if (!key.empty() && !foodByName_.emplace(key, foods_.size()).second) {
                throw std::runtime_error(location + ": повторяющееся название «" + key + "»");
            }
        }
        foods_.push_back({fields[0], {*kcal, *protein}, *portion, pieceGrams});
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
        std::optional<double> grams;
        if (food) {
            grams = food->portionGrams;
        } else if (const auto query = parseItem(text)) {
            food = graph.find(query->name);
            if (food) {
                grams = weightInGrams(*query, *food);
            }
        }
        if (!grams || !std::isfinite(*grams) || *grams <= 0) {
            items.push_back({text, "", 0, {}});
            continue;
        }

        const double factor = *grams / 100;
        const Nutrients nutrients{food->per100.kcal * factor, food->per100.protein * factor};
        if (!std::isfinite(nutrients.kcal) || !std::isfinite(nutrients.protein)) {
            items.push_back({text, "", 0, {}});
            continue;
        }
        items.push_back({text, food->name, *grams, nutrients});
    }
    return items;
}

}
