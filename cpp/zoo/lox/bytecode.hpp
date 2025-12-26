#pragma once

#include "canopy/flatmap.hpp"

#include "object.hpp"

namespace silva::lox {
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

  struct bytecode_chunk_t {
    syntax_ward_ptr_t swp;

    array_t<byte_t> bytecode;

    array_t<object_ref_t> constant_table;

    // (index in "bytecode") -> "pts"
    flatmap_t<index_t, parse_tree_span_t> origin_info;
    parse_tree_span_t origin_info_at_instr(index_t) const;

    bytecode_chunk_t(syntax_ward_ptr_t);

    expected_t<string_t> to_string(index_t level = 0) const;
    expected_t<index_t> to_string_at(string_t&, index_t) const;
  };

  struct bytecode_chunk_nursery_t {
    bytecode_chunk_t& chunk;

    template<typename T>
    void append(const parse_tree_span_t&, const T&);

    void append_constant_instr(const parse_tree_span_t&, object_ref_t);
    void append_simple_instr(const parse_tree_span_t&, opcode_t);
    index_t append_index_instr(const parse_tree_span_t&, opcode_t, index_t);
    void backpatch_index_instr(index_t position, index_t);
  };
}

// IMPLEMENTATION

namespace silva::lox {
  template<typename T>
  void bytecode_chunk_nursery_t::append(const parse_tree_span_t& pts, const T& x)
  {
    const index_t pos = chunk.bytecode.size();
    chunk.bytecode.resize(chunk.bytecode.size() + sizeof(T));
    bit_write_at<T>(&chunk.bytecode[pos], x);
  }
}
