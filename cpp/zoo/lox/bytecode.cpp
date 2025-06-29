#include "bytecode.hpp"

namespace silva::lox::bytecode {

  using enum opcode_t;

  parse_tree_span_t chunk_t::origin_info_at_instr(const index_t ip) const
  {
    if (origin_info.empty()) {
      return {};
    }
    auto it = origin_info.find(ip);
    if (it == origin_info.end() || (it != origin_info.begin() && (*it).first != ip)) {
      --it;
    }
    return (*it).second;
  }

  expected_t<string_t> chunk_t::to_string() const
  {
    string_t retval;
    index_t ip = 0;
    parse_tree_span_t prev_pts;
    auto it        = origin_info.begin();
    const auto end = origin_info.end();
    while (ip < bytecode.size()) {
      retval += fmt::format("{:4} ", ip);
      const parse_tree_span_t pts = origin_info_at_instr(ip);
      if (pts != prev_pts) {
        SILVA_EXPECT(pts != parse_tree_span_t{}, ASSERT);
        const auto& tloc = pts.tp->token_locations[pts[0].token_begin];
        retval += fmt::format("{:20}", fmt::format("[{}:{}]", tloc.line_num + 1, tloc.column + 1));
      }
      else {
        retval += fmt::format("{:20}", "");
      }
      prev_pts      = pts;
      const auto ll = SILVA_EXPECT_FWD(to_string_at(retval, ip));
      retval += '\n';
      ip += ll;
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
        retval += fmt::format("{} {} {}", name, const_idx, silva::pretty_string(cc));
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
    SILVA_EXPECT(idx < bytecode.size(),
                 MAJOR,
                 "index {} out of bytecode range ( < {} )",
                 idx,
                 bytecode.size());
    const span_t<const byte_t> bc = span_t<const byte_t>(bytecode).subspan(idx);
    impl::to_string_at_impl tsai{
        .chunk  = this,
        .retval = retval,
        .bc     = bc,
    };

    index_t instr_len = 0;

    switch (opcode_t(bc.front())) {
      case CONSTANT:
        return SILVA_EXPECT_FWD(tsai.const_instr("CONSTANT"));
      case NIL:
        return SILVA_EXPECT_FWD(tsai.simple_instr("NIL"));
      case TRUE:
        return SILVA_EXPECT_FWD(tsai.simple_instr("TRUE"));
      case FALSE:
        return SILVA_EXPECT_FWD(tsai.simple_instr("FALSE"));
      case POP:
        return SILVA_EXPECT_FWD(tsai.simple_instr("POP"));
      case GET_LOCAL:
        return SILVA_EXPECT_FWD(tsai.byte_instr("GET_LOCAL"));
      case SET_LOCAL:
        return SILVA_EXPECT_FWD(tsai.byte_instr("SET_LOCAL"));
      case GET_GLOBAL:
        return SILVA_EXPECT_FWD(tsai.const_instr("GET_GLOBAL"));
      case DEFINE_GLOBAL:
        return SILVA_EXPECT_FWD(tsai.const_instr("DEFINE_GLOBAL"));
      case SET_GLOBAL:
        return SILVA_EXPECT_FWD(tsai.const_instr("SET_GLOBAL"));
      case GET_UPVALUE:
        return SILVA_EXPECT_FWD(tsai.byte_instr("GET_UPVALUE"));
      case SET_UPVALUE:
        return SILVA_EXPECT_FWD(tsai.byte_instr("SET_UPVALUE"));
      case GET_PROPERTY:
        return SILVA_EXPECT_FWD(tsai.const_instr("GET_PROPERTY"));
      case SET_PROPERTY:
        return SILVA_EXPECT_FWD(tsai.const_instr("SET_PROPERTY"));
      case GET_SUPER:
        return SILVA_EXPECT_FWD(tsai.const_instr("GET_SUPER"));
      case EQUAL:
        return SILVA_EXPECT_FWD(tsai.simple_instr("EQUAL"));
      case GREATER:
        return SILVA_EXPECT_FWD(tsai.simple_instr("GREATER"));
      case LESS:
        return SILVA_EXPECT_FWD(tsai.simple_instr("LESS"));
      case ADD:
        return SILVA_EXPECT_FWD(tsai.simple_instr("ADD"));
      case SUBTRACT:
        return SILVA_EXPECT_FWD(tsai.simple_instr("SUBTRACT"));
      case MULTIPLY:
        return SILVA_EXPECT_FWD(tsai.simple_instr("MULTIPLY"));
      case DIVIDE:
        return SILVA_EXPECT_FWD(tsai.simple_instr("DIVIDE"));
      case NOT:
        return SILVA_EXPECT_FWD(tsai.simple_instr("NOT"));
      case NEGATE:
        return SILVA_EXPECT_FWD(tsai.simple_instr("NEGATE"));
      case PRINT:
        return SILVA_EXPECT_FWD(tsai.simple_instr("PRINT"));
      case JUMP:
        return SILVA_EXPECT_FWD(tsai.jump_instr("JUMP", 1));
      case JUMP_IF_FALSE:
        return SILVA_EXPECT_FWD(tsai.jump_instr("JUMP_IF_FALSE", 1));
      case LOOP:
        return SILVA_EXPECT_FWD(tsai.jump_instr("LOOP", -1));
      case CALL:
        return SILVA_EXPECT_FWD(tsai.byte_instr("CALL"));
      case INVOKE:
        return SILVA_EXPECT_FWD(tsai.invoke_instr("INVOKE"));
      case SUPER_INVOKE:
        return SILVA_EXPECT_FWD(tsai.invoke_instr("SUPER_INVOKE"));
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
        // return offset;
      }
      case CLOSE_UPVALUE:
        return SILVA_EXPECT_FWD(tsai.simple_instr("CLOSE_UPVALUE"));
      case RETURN:
        return SILVA_EXPECT_FWD(tsai.simple_instr("RETURN"));
      case CLASS:
        return SILVA_EXPECT_FWD(tsai.const_instr("CLASS"));
      case INHERIT:
        return SILVA_EXPECT_FWD(tsai.simple_instr("INHERIT"));
      case METHOD:
        return SILVA_EXPECT_FWD(tsai.const_instr("METHOD"));
      default:
        SILVA_EXPECT(false,
                     MAJOR,
                     "unknown instruction '{}' at index {}",
                     uint8_t(bc.front()),
                     idx);
    }
  }

  void chunk_nursery_t::register_origin_info(const parse_tree_span_t pts)
  {
    // TODO: insert with hint at the end of the flatmap.
    retval.origin_info[retval.bytecode.size()] = pts;
  }

  expected_t<void> chunk_nursery_t::append_constant_instr(object_ref_t obj_ref)
  {
    const auto idx     = retval.constant_table.size();
    const auto max_idx = index_t(std::numeric_limits<std::underlying_type_t<std::byte>>::max());
    SILVA_EXPECT(idx < max_idx, MAJOR, "Too many constants in chunk {} < {}", idx, max_idx);
    retval.constant_table.push_back(std::move(obj_ref));
    retval.bytecode.push_back(byte_t(CONSTANT));
    retval.bytecode.push_back(byte_t(idx));
    return {};
  }

  expected_t<void> chunk_nursery_t::append_simple_instr(const opcode_t opcode)
  {
    SILVA_EXPECT(opcode == NIL || opcode == TRUE || opcode == FALSE || opcode == POP ||
                     opcode == EQUAL || opcode == GREATER || opcode == LESS || opcode == ADD ||
                     opcode == SUBTRACT || opcode == MULTIPLY || opcode == DIVIDE ||
                     opcode == NOT || opcode == NEGATE || opcode == PRINT || opcode == RETURN,
                 ASSERT);
    retval.bytecode.push_back(byte_t(opcode));
    return {};
  }

  chunk_t chunk_nursery_t::finish() &&
  {
    return std::move(*this).retval;
  }
}
