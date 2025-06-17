#pragma once

#include "canopy/flatmap.hpp"

#include "object.hpp"

namespace silva::lox::bytecode {
  enum class opcode_t : uint8_t {
    CONSTANT,
    NIL,
    TRUE,
    FALSE,
    POP,
    GET_LOCAL,
    SET_LOCAL,
    GET_GLOBAL,
    DEFINE_GLOBAL,
    SET_GLOBAL,
    GET_UPVALUE,
    SET_UPVALUE,
    GET_PROPERTY,
    SET_PROPERTY,
    GET_SUPER,
    EQUAL,
    GREATER,
    LESS,
    ADD,
    SUBTRACT,
    MULTIPLY,
    DIVIDE,
    NOT,
    NEGATE,
    PRINT,
    JUMP,
    JUMP_IF_FALSE,
    LOOP,
    CALL,
    INVOKE,
    SUPER_INVOKE,
    CLOSURE,
    CLOSE_UPVALUE,
    RETURN,
    CLASS,
    INHERIT,
    METHOD
  };

  struct chunk_t {
    parse_tree_span_t pts;
    vector_t<byte_t> bytecode;
    // (index in "bytecode") -> (index in "pts")
    flatmap_t<index_t, index_t> origin_info;
    vector_t<object_ref_t> constant_table;

    expected_t<string_t> to_string() const;
    expected_t<index_t> to_string_at(string_t&, index_t) const;
  };
}
