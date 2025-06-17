#include "bytecode.hpp"

namespace silva::lox::bytecode {

  expected_t<string_t> chunk_t::to_string() const
  {
    string_t retval;
    index_t idx    = 0;
    auto it        = origin_info.begin();
    const auto end = origin_info.end();
    const auto pts = ptp->span();
    while (true) {
      retval += fmt::format("{:4}", idx);

      bool moved = false;
      while (std::next(it) != end && (*std::next(it)).first <= idx) {
        ++it;
      }
      if (moved) {
        const index_t node_idx = (*it).second;
        const auto curr_pts    = pts.sub_tree_span_at(node_idx);
        const auto tloc        = pts.tp->token_locations[curr_pts[0].token_begin];
        retval += fmt::format("{:3}:{:3} ", tloc.line_num, tloc.column);
      }
      else {
        retval += fmt::format("        ");
      }

      const auto ll = SILVA_EXPECT_FWD(to_string_at(retval, idx));
      retval += '\n';
      idx += ll;
    }
    return retval;
  }

  namespace impl {
    struct to_string_at_impl {
      const chunk_t* chunk = nullptr;
      string_t& retval;
      const span_t<const byte_t> bc;

      expected_t<index_t> simple_instr(const string_view_t name)
      {
        retval += fmt::format("{}", name);
        return 1;
      }
      expected_t<index_t> const_instr(const string_view_t name)
      {
        SILVA_EXPECT(bc.size() >= 2, MINOR, "no operand for constant-instruction");
        const index_t const_idx = index_t(bc[1]);
        SILVA_EXPECT(0 <= const_idx && const_idx < chunk->constant_table.size(), MINOR);
        const auto& cc = chunk->constant_table[const_idx];
        retval += fmt::format("{} {} {}", name, const_idx, silva::to_string(cc));
        return 2;
      }
      expected_t<index_t> invoke_instr(const string_view_t name)
      {
        // uint8_t constant = chunk->code[offset + 1];
        // uint8_t argCount = chunk->code[offset + 2];
        // printf("%-16s (%d args) %4d '", name, argCount, constant);
        // printValue(chunk->constants.values[constant]);
        // printf("'\n");
        return 3;
      }
      expected_t<index_t> byte_instr(const string_view_t name)
      {
        SILVA_EXPECT(bc.size() >= 2, MINOR, "no operand for byte-instruction");
        const index_t slot = index_t(bc[1]);
        retval += fmt::format("{} slot {}", name, slot);
        return 2;
      }
      expected_t<index_t> jump_instr(const string_view_t name, index_t sign)
      {
        SILVA_EXPECT(bc.size() >= 3, MINOR, "no two operands for jump-instruction");
        const index_t jump = index_t(size_t(bc[1] << 8) | size_t(bc[2])) * sign;
        retval += fmt::format("{} relative {}", name, jump);
        return 3;
      }
    };
  }

  expected_t<index_t> chunk_t::to_string_at(string_t& retval, const index_t idx) const
  {
    using enum opcode_t;

    const span_t<const byte_t> bc = span_t<const byte_t>(bytecode).subspan(idx);
    impl::to_string_at_impl tsai{
        .chunk  = this,
        .retval = retval,
        .bc     = bc,
    };

    index_t instr_len = 0;

    switch (opcode_t(bc.front())) {
      case CONSTANT:
        instr_len = SILVA_EXPECT_FWD(tsai.const_instr("CONSTANT"));
      case NIL:
        instr_len = SILVA_EXPECT_FWD(tsai.simple_instr("NIL"));
      case TRUE:
        instr_len = SILVA_EXPECT_FWD(tsai.simple_instr("TRUE"));
      case FALSE:
        instr_len = SILVA_EXPECT_FWD(tsai.simple_instr("FALSE"));
      case POP:
        instr_len = SILVA_EXPECT_FWD(tsai.simple_instr("POP"));
      case GET_LOCAL:
        instr_len = SILVA_EXPECT_FWD(tsai.byte_instr("GET_LOCAL"));
      case SET_LOCAL:
        instr_len = SILVA_EXPECT_FWD(tsai.byte_instr("SET_LOCAL"));
      case GET_GLOBAL:
        instr_len = SILVA_EXPECT_FWD(tsai.const_instr("GET_GLOBAL"));
      case DEFINE_GLOBAL:
        instr_len = SILVA_EXPECT_FWD(tsai.const_instr("DEFINE_GLOBAL"));
      case SET_GLOBAL:
        instr_len = SILVA_EXPECT_FWD(tsai.const_instr("SET_GLOBAL"));
      case GET_UPVALUE:
        instr_len = SILVA_EXPECT_FWD(tsai.byte_instr("GET_UPVALUE"));
      case SET_UPVALUE:
        instr_len = SILVA_EXPECT_FWD(tsai.byte_instr("SET_UPVALUE"));
      case GET_PROPERTY:
        instr_len = SILVA_EXPECT_FWD(tsai.const_instr("GET_PROPERTY"));
      case SET_PROPERTY:
        instr_len = SILVA_EXPECT_FWD(tsai.const_instr("SET_PROPERTY"));
      case GET_SUPER:
        instr_len = SILVA_EXPECT_FWD(tsai.const_instr("GET_SUPER"));
      case EQUAL:
        instr_len = SILVA_EXPECT_FWD(tsai.simple_instr("EQUAL"));
      case GREATER:
        instr_len = SILVA_EXPECT_FWD(tsai.simple_instr("GREATER"));
      case LESS:
        instr_len = SILVA_EXPECT_FWD(tsai.simple_instr("LESS"));
      case ADD:
        instr_len = SILVA_EXPECT_FWD(tsai.simple_instr("ADD"));
      case SUBTRACT:
        instr_len = SILVA_EXPECT_FWD(tsai.simple_instr("SUBTRACT"));
      case MULTIPLY:
        instr_len = SILVA_EXPECT_FWD(tsai.simple_instr("MULTIPLY"));
      case DIVIDE:
        instr_len = SILVA_EXPECT_FWD(tsai.simple_instr("DIVIDE"));
      case NOT:
        instr_len = SILVA_EXPECT_FWD(tsai.simple_instr("NOT"));
      case NEGATE:
        instr_len = SILVA_EXPECT_FWD(tsai.simple_instr("NEGATE"));
      case PRINT:
        instr_len = SILVA_EXPECT_FWD(tsai.simple_instr("PRINT"));
      case JUMP:
        instr_len = SILVA_EXPECT_FWD(tsai.jump_instr("JUMP", 1));
      case JUMP_IF_FALSE:
        instr_len = SILVA_EXPECT_FWD(tsai.jump_instr("JUMP_IF_FALSE", 1));
      case LOOP:
        instr_len = SILVA_EXPECT_FWD(tsai.jump_instr("LOOP", -1));
      case CALL:
        instr_len = SILVA_EXPECT_FWD(tsai.byte_instr("CALL"));
      case INVOKE:
        instr_len = SILVA_EXPECT_FWD(tsai.invoke_instr("INVOKE"));
      case SUPER_INVOKE:
        instr_len = SILVA_EXPECT_FWD(tsai.invoke_instr("SUPER_INVOKE"));
      case CLOSURE: {
        // offset++;
        // uint8_t constant = chunk->code[offset++];
        // printf("%-16s %4d ", "CLOSURE", constant);
        // printValue(chunk->constants.values[constant]);
        // printf("\n");
        // ObjFunction* function = AS_FUNCTION(chunk->constants.values[constant]);
        // for (index_t j = 0; j < function->upvalueCount; j++) {
        //   const index_t isLocal = chunk->code[offset++];
        //   const index_t index   = chunk->code[offset++];
        //   printf("%04d      |                     %s %d\n",
        //          offset - 2,
        //          isLocal ? "local" : "upvalue",
        //          index);
        // }
        // instr_len = offset;
      }
      case CLOSE_UPVALUE:
        instr_len = SILVA_EXPECT_FWD(tsai.simple_instr("CLOSE_UPVALUE"));
      case RETURN:
        instr_len = SILVA_EXPECT_FWD(tsai.simple_instr("RETURN"));
      case CLASS:
        instr_len = SILVA_EXPECT_FWD(tsai.const_instr("CLASS"));
      case INHERIT:
        instr_len = SILVA_EXPECT_FWD(tsai.simple_instr("INHERIT"));
      case METHOD:
        instr_len = SILVA_EXPECT_FWD(tsai.const_instr("METHOD"));
      default:
        SILVA_EXPECT(false,
                     MINOR,
                     "unknown instruction '{}' at index {}",
                     uint8_t(bc.front()),
                     idx);
        instr_len = 1;
    }

    return {instr_len};
  }
}
