#pragma once
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

[[nodiscard]] inline RawSchema analyze_raw_schema(const std::vector<nlohmann::json>& entries) {
    RawSchema schema;
    schema.record_count = static_cast<int>(entries.size());

    std::map<std::string, size_t> column_index;

    for (const auto& entry : entries) {
        if (!entry.is_object()) continue;
        for (auto it = entry.begin(); it != entry.end(); ++it) {
            const auto& key = it.key();
            const auto& value = it.value();
            if (column_index.find(key) == column_index.end()) {
                column_index[key] = schema.columns.size();
                schema.columns.push_back(ColumnNode{.name = key, .path = key, .kind = "leaf"});
            }
            auto& col = schema.columns[column_index[key]];
            col.stats.present_count++;
            col.stats.type_counts[json_type_name(value)]++;
        }
    }

    return schema;
}
