#include "bytecode.hpp"

#include "canopy/bit.hpp"

namespace silva::lox::bytecode {

  using enum opcode_t;

  chunk_t::chunk_t(syntax_ward_ptr_t swp) : swp(std::move(swp)) {}

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

  expected_t<string_t> chunk_t::to_string(const index_t level) const
  {
    string_t space_buf(level * 2 + 2, ' ');
    string_view_t space{space_buf};
    string_t retval;
    index_t ip = 0;
    const tokenization_t::location_t* prev_tloc{nullptr};
    auto it        = origin_info.begin();
    const auto end = origin_info.end();
    while (ip < bytecode.size()) {
      retval += space.substr(0, level * 2);
      retval += fmt::format("{:4} ", ip);
      const parse_tree_span_t pts = origin_info_at_instr(ip);
      SILVA_EXPECT(pts != parse_tree_span_t{}, ASSERT);
      const tokenization_t::location_t* tloc = &pts.tp->token_locations[pts[0].token_begin];
      if (prev_tloc == nullptr || *prev_tloc != *tloc) {
        retval +=
            fmt::format("{:20}", fmt::format("[{}:{}]", tloc->line_num + 1, tloc->column + 1));
      }
      else {
        retval += fmt::format("{:20}", " -- ");
      }
      prev_tloc     = tloc;
      const auto ll = SILVA_EXPECT_FWD(to_string_at(retval, ip));
      retval += '\n';
      ip += ll;
    }
    if (!constant_table.empty()) {
      retval += "\n";
      retval += space.substr(0, level * 2);
      retval += "CONSTANT-TABLE\n";
      for (index_t i = 0; i < constant_table.size(); ++i) {
        retval += space.substr(0, level * 2);
        retval += fmt::format("CONSTANT {} ", i);
        const object_ref_t& cc = constant_table[i];
        if (cc->holds_function()) {
          const function_t& fun = std::get<function_t>(cc->data);
          retval += silva::pretty_string(fun.pts);
          retval += '\n';
          SILVA_EXPECT(fun.chunk, MAJOR);
          retval += SILVA_EXPECT_FWD(fun.chunk->to_string(level + 1));
        }
        else {
          retval += silva::pretty_string(cc);
          retval += '\n';
        }
      }
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
        SILVA_EXPECT(bc.size() >= 5, MINOR, "no operand for constant-instruction");
        const auto const_idx = bit_cast_ptr<index_t>(&bc[1]);
        SILVA_EXPECT(0 <= const_idx && const_idx < chunk->constant_table.size(), MINOR);
        const auto& cc = chunk->constant_table[const_idx];
        retval += fmt::format("{} {}", name, const_idx);
        return 5;
      }
      expected_t<index_t> token_instr(const string_view_t name)
      {
        SILVA_EXPECT(bc.size() >= 5, MINOR, "no operand for token-instruction");
        const auto ti = bit_cast_ptr<index_t>(&bc[1]);
        SILVA_EXPECT(0 <= ti && ti < chunk->swp->token_infos.size(), MINOR);
        const auto tinfo = chunk->swp->token_infos[ti];
        retval += fmt::format("{} {} {}", name, ti, tinfo.str);
        return 5;
      }
      expected_t<index_t> index_instr(const string_view_t name)
      {
        SILVA_EXPECT(bc.size() >= 5, MINOR, "no operand for index-instruction");
        const auto index = bit_cast_ptr<index_t>(&bc[1]);
        retval += fmt::format("{} {}", name, index);
        return 5;
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
        return SILVA_EXPECT_FWD(tsai.index_instr("GET_LOCAL"));
      case SET_LOCAL:
        return SILVA_EXPECT_FWD(tsai.index_instr("SET_LOCAL"));
      case GET_GLOBAL:
        return SILVA_EXPECT_FWD(tsai.token_instr("GET_GLOBAL"));
      case DEFINE_GLOBAL:
        return SILVA_EXPECT_FWD(tsai.token_instr("DEFINE_GLOBAL"));
      case SET_GLOBAL:
        return SILVA_EXPECT_FWD(tsai.token_instr("SET_GLOBAL"));
      case GET_UPVALUE:
        return SILVA_EXPECT_FWD(tsai.token_instr("GET_UPVALUE"));
      case SET_UPVALUE:
        return SILVA_EXPECT_FWD(tsai.token_instr("SET_UPVALUE"));
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
        return SILVA_EXPECT_FWD(tsai.index_instr("JUMP"));
      case JUMP_IF_FALSE:
        return SILVA_EXPECT_FWD(tsai.index_instr("JUMP_IF_FALSE"));
      case CALL:
        return SILVA_EXPECT_FWD(tsai.index_instr("CALL"));
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

  namespace detail {
    void set_pts(chunk_t& self, const parse_tree_span_t& pts)
    {
      // TODO: insert with hint at the end of the flatmap.
      self.origin_info[self.bytecode.size()] = pts;
    }

    template<typename T>
    void append_bit(chunk_t& self, const T x)
    {
      const index_t pos = self.bytecode.size();
      self.bytecode.resize(self.bytecode.size() + sizeof(T));
      bit_write_at<index_t>(&self.bytecode[pos], x);
    }
  }

  expected_t<void> chunk_nursery_t::append_constant_instr(const parse_tree_span_t& pts,
                                                          object_ref_t obj_ref)
  {
    detail::set_pts(chunk, pts);
    const index_t idx  = chunk.constant_table.size();
    const auto max_idx = index_t(std::numeric_limits<std::underlying_type_t<std::byte>>::max());
    SILVA_EXPECT(idx < max_idx, MAJOR, "Too many constants in chunk {} < {}", idx, max_idx);
    chunk.constant_table.push_back(std::move(obj_ref));
    chunk.bytecode.push_back(byte_t(CONSTANT));
    detail::append_bit(chunk, idx);
    return {};
  }

  expected_t<void> chunk_nursery_t::append_simple_instr(const parse_tree_span_t& pts,
                                                        const opcode_t opcode)
  {
    SILVA_EXPECT(opcode == NIL || opcode == TRUE || opcode == FALSE || opcode == POP ||
                     opcode == EQUAL || opcode == GREATER || opcode == LESS || opcode == ADD ||
                     opcode == SUBTRACT || opcode == MULTIPLY || opcode == DIVIDE ||
                     opcode == NOT || opcode == NEGATE || opcode == PRINT || opcode == RETURN,
                 ASSERT);
    detail::set_pts(chunk, pts);
    chunk.bytecode.push_back(byte_t(opcode));
    return {};
  }

  expected_t<index_t> chunk_nursery_t::append_index_instr(const parse_tree_span_t& pts,
                                                          const opcode_t opcode,
                                                          const index_t idx)
  {
    SILVA_EXPECT(opcode == DEFINE_GLOBAL || opcode == SET_LOCAL || opcode == GET_LOCAL ||
                     opcode == SET_GLOBAL || opcode == GET_GLOBAL || opcode == JUMP ||
                     opcode == JUMP_IF_FALSE || opcode == CALL,
                 ASSERT);
    detail::set_pts(chunk, pts);
    const index_t rv = chunk.bytecode.size();
    chunk.bytecode.push_back(byte_t(opcode));
    detail::append_bit(chunk, idx);
    return rv;
  }

  expected_t<void> chunk_nursery_t::backpatch_index_instr(const index_t position, const index_t idx)
  {
    bit_write_at<index_t>(&chunk.bytecode[position + 1], idx);
    return {};
  }
}
