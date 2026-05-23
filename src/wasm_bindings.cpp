// Approach taken:
//   Primary bindings for ContentBlock, ParseError, SessionEntry, Session (existing).
//   Schema types (FieldStats, ColumnNode, RawSchema) added per Task 12.
//
//   std::optional<double> fields (numeric_min, numeric_max, array_avg_length,
//   array_max_length) on FieldStats cannot be bound directly via .field() because
//   Emscripten value_object does not know how to convert std::optional<T>.
//   Fallback applied: helper functions in schema.hpp expose has_*/get_* pairs,
//   bound as computed fields via .field("name", &fn).
//
//   std::optional<RawSchema> on Session likewise cannot be bound directly.
//   Fallback applied: hasRawSchema / getRawSchema exported as free functions
//   that take a `const Session&`. For this to actually work, Session must be
//   bound as class_<> (not value_object<>): value_object serializes the JS
//   object into a fresh C++ Session via the bound .field()s, and since
//   raw_schema is unbound, the round-trip drops it — making hasRawSchema
//   always return false. class_<> exposes the object as a handle, so the
//   real C++ Session (with its populated optional) is what hasRawSchema sees.
//
//   ColumnNode also uses class_<> because it is self-referential
//   (contains std::vector<ColumnNode>); value_object semantics require a complete
//   copyable value type without reference cycles, which recursive vectors violate
//   in Emscripten's Embind.

#include <emscripten/bind.h>
#include "parser.hpp"
#include "schema.hpp"

using namespace emscripten;

EMSCRIPTEN_BINDINGS(session_parser) {
    value_object<ContentBlock>("ContentBlock")
        .field("type", &ContentBlock::type)
        .field("text", &ContentBlock::text)
        .field("toolName", &ContentBlock::tool_name)
        .field("toolInput", &ContentBlock::tool_input)
        .field("toolUseId", &ContentBlock::tool_use_id)
        .field("isError", &ContentBlock::is_error);

    value_object<ParseError>("ParseError")
        .field("lineNumber", &ParseError::line_number)
        .field("rawLine", &ParseError::raw_line);

    value_object<SessionEntry>("SessionEntry")
        .field("type", &SessionEntry::type)
        .field("timestamp", &SessionEntry::timestamp)
        .field("content", &SessionEntry::content)
        .field("lineNumber", &SessionEntry::line_number);

    register_vector<ContentBlock>("VectorContentBlock");
    register_vector<SessionEntry>("VectorSessionEntry");
    register_vector<ParseError>("VectorParseError");

    // Schema types
    value_object<std::pair<std::string, int>>("StringCount")
        .field("value", &std::pair<std::string, int>::first)
        .field("count", &std::pair<std::string, int>::second);
    register_vector<std::pair<std::string, int>>("VectorStringCount");

    register_map<std::string, int>("MapStringInt");
    // MapStringInt.keys() returns std::vector<std::string> — must be registered
    // or any JS-side iteration via .keys() throws "unbound types".
    register_vector<std::string>("VectorString");

    // FieldStats: optional<double> fields exposed via getter/setter pairs
    // (defined in schema.hpp) because Emscripten value_object .field() requires
    // both a getter and setter; setters are no-ops since these are read-only from JS.
    value_object<FieldStats>("FieldStats")
        .field("presentCount", &FieldStats::present_count)
        .field("nullCount", &FieldStats::null_count)
        .field("typeCounts", &FieldStats::type_counts)
        .field("topValues", &FieldStats::top_values)
        .field("hasNumericMin",    &has_numeric_min,      &set_has_numeric_min)
        .field("numericMin",       &get_numeric_min,      &set_numeric_min)
        .field("hasNumericMax",    &has_numeric_max,      &set_has_numeric_max)
        .field("numericMax",       &get_numeric_max,      &set_numeric_max)
        .field("hasArrayAvgLength",&has_array_avg_length, &set_has_array_avg_length)
        .field("arrayAvgLength",   &get_array_avg_length, &set_array_avg_length)
        .field("hasArrayMaxLength",&has_array_max_length, &set_has_array_max_length)
        .field("arrayMaxLength",   &get_array_max_length, &set_array_max_length);

    // ColumnNode uses class_<> because it is recursive (contains vector<ColumnNode>).
    class_<ColumnNode>("ColumnNode")
        .property("name", &ColumnNode::name)
        .property("path", &ColumnNode::path)
        .property("kind", &ColumnNode::kind)
        .property("children", &ColumnNode::children)
        .property("stats", &ColumnNode::stats);
    register_vector<ColumnNode>("VectorColumnNode");

    value_object<RawSchema>("RawSchema")
        .field("columns", &RawSchema::columns)
        .field("recordCount", &RawSchema::record_count);

    // Session uses class_<> so JS holds a handle (not a copy). This preserves
    // the raw_schema optional across calls to hasRawSchema / getRawSchema.
    class_<Session>("Session")
        .property("title", &Session::title)
        .property("entries", &Session::entries)
        .property("errors", &Session::errors);

    function("hasRawSchema", &has_raw_schema);
    function("getRawSchema", &get_raw_schema);

    function("parseSession", &parse_session);
}
