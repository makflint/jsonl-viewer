#pragma once
#include <algorithm>
#include <map>
#include <optional>
#include <string>
#include <vector>
#include "../third_party/json.hpp"

struct FieldStats {
    int present_count = 0;
    int null_count = 0;
    std::map<std::string, int> type_counts;
    std::vector<std::pair<std::string, int>> top_values;
    std::optional<double> numeric_min, numeric_max;
    std::optional<double> array_avg_length, array_max_length;
};

struct ColumnNode {
    std::string name;
    std::string path;
    std::string kind;
    std::vector<ColumnNode> children;
    FieldStats stats;
};

struct RawSchema {
    std::vector<ColumnNode> columns;
    int record_count = 0;
};

[[nodiscard]] inline std::string json_type_name(const nlohmann::json& value) {
    if (value.is_string()) return "string";
    if (value.is_number_integer()) return "number";
    if (value.is_number_float()) return "number";
    if (value.is_boolean()) return "boolean";
    if (value.is_null()) return "null";
    if (value.is_object()) return "object";
    if (value.is_array()) return "array";
    return "unknown";
}

[[nodiscard]] inline std::vector<std::pair<std::string, int>> top_n_by_frequency(
    const std::map<std::string, int>& counts, size_t n) {
    std::vector<std::pair<std::string, int>> sorted(counts.begin(), counts.end());
    std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) {
        if (a.second != b.second) return a.second > b.second;
        return a.first < b.first;
    });
    if (sorted.size() > n) sorted.resize(n);
    return sorted;
}

[[nodiscard]] inline RawSchema analyze_raw_schema(const std::vector<nlohmann::json>& entries) {
    RawSchema schema;
    schema.record_count = static_cast<int>(entries.size());

    std::map<std::string, size_t> column_index;
    std::vector<std::map<std::string, int>> string_counts;  // parallel to schema.columns
    std::vector<double> array_length_sum;
    std::vector<int> array_length_count;

    for (const auto& entry : entries) {
        if (!entry.is_object()) continue;
        for (auto it = entry.begin(); it != entry.end(); ++it) {
            const auto& key = it.key();
            const auto& value = it.value();
            if (column_index.find(key) == column_index.end()) {
                column_index[key] = schema.columns.size();
                std::string kind = value.is_array() ? "array_summary"
                                  : value.is_object() ? "object"
                                  : "leaf";
                schema.columns.push_back(ColumnNode{.name = key, .path = key, .kind = kind});
                string_counts.emplace_back();
                array_length_sum.push_back(0.0);
                array_length_count.push_back(0);
            }
            auto idx = column_index[key];
            auto& col = schema.columns[idx];
            col.stats.present_count++;
            if (value.is_null()) col.stats.null_count++;
            col.stats.type_counts[json_type_name(value)]++;
            if (value.is_number()) {
                double n = value.get<double>();
                if (!col.stats.numeric_min || n < *col.stats.numeric_min) col.stats.numeric_min = n;
                if (!col.stats.numeric_max || n > *col.stats.numeric_max) col.stats.numeric_max = n;
            }
            if (value.is_string()) {
                string_counts[idx][value.get<std::string>()]++;
            }
            if (value.is_array()) {
                double len = static_cast<double>(value.size());
                if (!col.stats.array_max_length || len > *col.stats.array_max_length) col.stats.array_max_length = len;
                array_length_sum[idx] += len;
                array_length_count[idx]++;
            }
        }
    }

    // Materialize top-5 per column
    for (size_t i = 0; i < schema.columns.size(); ++i) {
        schema.columns[i].stats.top_values = top_n_by_frequency(string_counts[i], 5);
        if (array_length_count[i] > 0) {
            schema.columns[i].stats.array_avg_length = array_length_sum[i] / array_length_count[i];
        }
    }

    return schema;
}
