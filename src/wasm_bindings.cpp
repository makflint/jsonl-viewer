#include <emscripten/bind.h>
#include "parser.hpp"

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
        .field("content", &SessionEntry::content);

    register_vector<ContentBlock>("VectorContentBlock");
    register_vector<SessionEntry>("VectorSessionEntry");
    register_vector<ParseError>("VectorParseError");

    value_object<Session>("Session")
        .field("title", &Session::title)
        .field("entries", &Session::entries)
        .field("errors", &Session::errors);

    function("parseSession", &parse_session);
}
