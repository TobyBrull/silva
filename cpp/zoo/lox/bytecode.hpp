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
    syntax_ward_ptr_t swp;

    vector_t<byte_t> bytecode;

    vector_t<object_ref_t> constant_table;

    // (index in "bytecode") -> "pts"
    flatmap_t<index_t, parse_tree_span_t> origin_info;
    parse_tree_span_t origin_info_at_instr(index_t) const;

    chunk_t(syntax_ward_ptr_t);

    expected_t<string_t> to_string() const;
    expected_t<index_t> to_string_at(string_t&, index_t) const;
  };

  struct chunk_nursery_t {
    chunk_t& chunk;

    expected_t<void> append_constant_instr(const parse_tree_span_t&, object_ref_t);
    expected_t<void> append_simple_instr(const parse_tree_span_t&, opcode_t);
    expected_t<index_t> append_index_instr(const parse_tree_span_t&, opcode_t, index_t);
    expected_t<void> backpatch_index_instr(index_t position, index_t);
  };
}
