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
    // Transient accumulators used during analysis; finalize_schema clears these.
    std::map<std::string, int> _string_counts;
    double _array_length_sum = 0.0;
    int _array_length_count = 0;
    std::map<std::string, size_t> _child_index;  // name -> index in children
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

inline void merge_value(ColumnNode& col, const nlohmann::json& value);

inline void merge_object(std::vector<ColumnNode>& columns,
                          std::map<std::string, size_t>& index,
                          const std::string& path_prefix,
                          const nlohmann::json& obj) {
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        const auto& key = it.key();
        const auto& value = it.value();
        if (index.find(key) == index.end()) {
            index[key] = columns.size();
            std::string path = path_prefix.empty() ? key : path_prefix + "." + key;
            std::string kind = value.is_array() ? "array_summary"
                              : value.is_object() ? "object"
                              : "leaf";
            columns.push_back(ColumnNode{.name = key, .path = path, .kind = kind});
        }
        merge_value(columns[index[key]], value);
    }
}

inline void merge_value(ColumnNode& col, const nlohmann::json& value) {
    col.stats.present_count++;
    if (value.is_null()) col.stats.null_count++;
    col.stats.type_counts[json_type_name(value)]++;
    if (value.is_number()) {
        double n = value.get<double>();
        if (!col.stats.numeric_min || n < *col.stats.numeric_min) col.stats.numeric_min = n;
        if (!col.stats.numeric_max || n > *col.stats.numeric_max) col.stats.numeric_max = n;
    }
    if (value.is_string()) {
        col._string_counts[value.get<std::string>()]++;
    }
    if (value.is_array()) {
        double len = static_cast<double>(value.size());
        if (!col.stats.array_max_length || len > *col.stats.array_max_length) col.stats.array_max_length = len;
        col._array_length_sum += len;
        col._array_length_count++;
    }
    if (value.is_object()) {
        merge_object(col.children, col._child_index, col.path, value);
    }
}

inline void finalize_column(ColumnNode& col) {
    col.stats.top_values = top_n_by_frequency(col._string_counts, 5);
    if (col._array_length_count > 0) {
        col.stats.array_avg_length = col._array_length_sum / col._array_length_count;
    }
    col._string_counts.clear();
    col._child_index.clear();
    for (auto& child : col.children) finalize_column(child);
}

[[nodiscard]] inline RawSchema analyze_raw_schema(const std::vector<nlohmann::json>& entries) {
    RawSchema schema;
    schema.record_count = static_cast<int>(entries.size());
    std::map<std::string, size_t> top_index;

    for (const auto& entry : entries) {
        if (!entry.is_object()) continue;
        merge_object(schema.columns, top_index, "", entry);
    }

    for (auto& col : schema.columns) finalize_column(col);
    return schema;
}

// WASM binding helpers: Emscripten cannot bind std::optional<T> directly via
// value_object .field(), so these has_*/get_* getter/setter pairs expose optional
// fields safely. Setters are no-ops (fields are logically read-only from JS).
inline bool   has_numeric_min(const FieldStats& s)           { return s.numeric_min.has_value(); }
inline double get_numeric_min(const FieldStats& s)           { return s.numeric_min.value_or(0.0); }
inline void   set_has_numeric_min(FieldStats&, bool)         {}
inline void   set_numeric_min(FieldStats&, double)           {}
inline bool   has_numeric_max(const FieldStats& s)           { return s.numeric_max.has_value(); }
inline double get_numeric_max(const FieldStats& s)           { return s.numeric_max.value_or(0.0); }
inline void   set_has_numeric_max(FieldStats&, bool)         {}
inline void   set_numeric_max(FieldStats&, double)           {}
inline bool   has_array_avg_length(const FieldStats& s)      { return s.array_avg_length.has_value(); }
inline double get_array_avg_length(const FieldStats& s)      { return s.array_avg_length.value_or(0.0); }
inline void   set_has_array_avg_length(FieldStats&, bool)    {}
inline void   set_array_avg_length(FieldStats&, double)      {}
inline bool   has_array_max_length(const FieldStats& s)      { return s.array_max_length.has_value(); }
inline double get_array_max_length(const FieldStats& s)      { return s.array_max_length.value_or(0.0); }
inline void   set_has_array_max_length(FieldStats&, bool)    {}
inline void   set_array_max_length(FieldStats&, double)      {}
