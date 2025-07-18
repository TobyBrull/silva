#include "bytecode_vm.hpp"

namespace silva::lox::bytecode {

  using enum opcode_t;

  struct runner_t {
    const chunk_t& chunk;
    const vector_t<byte_t>& bytecode;
    vm_t& vm;

    index_t ip = 0;

    expected_t<void> _constant()
    {
      const auto const_idx = bit_cast_ptr<index_t>(&bytecode[ip + 1]);
      vm.stack.push_back(chunk.constant_table[const_idx]);
      ip += 5;
      return {};
    }
    expected_t<void> _nil()
    {
      vm.stack.push_back(vm.pool.const_nil);
      ip += 1;
      return {};
    }
    expected_t<void> _true()
    {
      vm.stack.push_back(vm.pool.const_true);
      ip += 1;
      return {};
    }
    expected_t<void> _false()
    {
      vm.stack.push_back(vm.pool.const_false);
      ip += 1;
      return {};
    }
    expected_t<void> _pop()
    {
      vm.stack.pop_back();
      ip += 1;
      return {};
    }
    expected_t<void> _get_local()
    {
      const auto stack_idx = bit_cast_ptr<index_t>(&bytecode[ip + 1]);
      SILVA_EXPECT(0 <= stack_idx && stack_idx < vm.stack.size(),
                   RUNTIME,
                   "{} stack index {} not inside stack",
                   chunk.origin_info_at_instr(ip),
                   stack_idx);
      vm.stack.push_back(vm.stack[stack_idx]);
      ip += 5;
      return {};
    }
    expected_t<void> _set_local()
    {
      const auto stack_idx = bit_cast_ptr<index_t>(&bytecode[ip + 1]);
      SILVA_EXPECT(0 <= stack_idx && stack_idx < vm.stack.size(),
                   RUNTIME,
                   "{} stack index {} not inside stack",
                   chunk.origin_info_at_instr(ip),
                   stack_idx);
      vm.stack[stack_idx] = vm.stack.back();
      ip += 5;
      return {};
    }
    expected_t<void> _get_global()
    {
      const auto ti = bit_cast_ptr<index_t>(&bytecode[ip + 1]);
      auto it       = vm.globals.find(ti);
      SILVA_EXPECT(it != vm.globals.end(),
                   RUNTIME,
                   "{} couldn't find global variable {}",
                   chunk.origin_info_at_instr(ip),
                   vm.swp->token_id_wrap(ti));
      vm.stack.push_back(it->second);
      ip += 5;
      return {};
    }
    expected_t<void> _define_global()
    {
      const auto ti = bit_cast_ptr<index_t>(&bytecode[ip + 1]);
      SILVA_EXPECT(vm.stack.size() >= 1,
                   RUNTIME,
                   "{} bytecode instruction DEFINE_GLOBAL needs non-empty stack",
                   chunk.origin_info_at_instr(ip));
      vm.globals[ti] = vm.stack.back();
      vm.stack.pop_back();
      ip += 5;
      return {};
    }
    expected_t<void> _set_global()
    {
      const auto ti = bit_cast_ptr<index_t>(&bytecode[ip + 1]);
      SILVA_EXPECT(vm.stack.size() >= 1,
                   RUNTIME,
                   "{} bytecode instruction SET_GLOBAL needs non-empty stack",
                   chunk.origin_info_at_instr(ip));
      auto it = vm.globals.find(ti);
      SILVA_EXPECT(it != vm.globals.end(),
                   RUNTIME,
                   "{} bytecode instruction SET_GLOBAL tried to assign to global variable {} which "
                   "was not previously defined",
                   chunk.origin_info_at_instr(ip),
                   vm.swp->token_id_wrap(ti));
      it->second = vm.stack.back();
      ip += 5;
      return {};
    }
    expected_t<void> _get_upvalue() { SILVA_EXPECT(false, ASSERT); }
    expected_t<void> _set_upvalue() { SILVA_EXPECT(false, ASSERT); }
    expected_t<void> _get_property() { SILVA_EXPECT(false, ASSERT); }
    expected_t<void> _set_property() { SILVA_EXPECT(false, ASSERT); }
    expected_t<void> _get_super() { SILVA_EXPECT(false, ASSERT); }

#define SIMPLE_BINARY_OP(here_name, obj_name)                                           \
  expected_t<void> here_name()                                                          \
  {                                                                                     \
    SILVA_EXPECT(vm.stack.size() >= 2,                                                  \
                 RUNTIME,                                                               \
                 "{} bytecode instruction 'obj_name' needs at least two stack entries", \
                 chunk.origin_info_at_instr(ip));                                       \
    auto rhs     = vm.stack.back();                                                     \
    auto lhs     = vm.stack[vm.stack.size() - 2];                                       \
    auto new_val = SILVA_EXPECT_FWD_PLAIN(obj_name(vm.pool, lhs, rhs));                 \
    vm.stack.pop_back();                                                                \
    vm.stack.pop_back();                                                                \
    vm.stack.push_back(std::move(new_val));                                             \
    ip += 1;                                                                            \
    return {};                                                                          \
  }
    SIMPLE_BINARY_OP(_add, add);
    SIMPLE_BINARY_OP(_subtract, sub);
    SIMPLE_BINARY_OP(_multiply, mul);
    SIMPLE_BINARY_OP(_divide, div);
    SIMPLE_BINARY_OP(_equal, eq);
    SIMPLE_BINARY_OP(_greater, gt);
    SIMPLE_BINARY_OP(_less, lt);
#undef SIMPLE_BINARY_OP

#define SIMPLE_UNARY_OP(here_name, obj_name)                                 \
  expected_t<void> here_name()                                               \
  {                                                                          \
    SILVA_EXPECT(vm.stack.size() >= 1,                                       \
                 RUNTIME,                                                    \
                 "{} bytecode instruction 'obj_name' needs non-empty stack", \
                 chunk.origin_info_at_instr(ip));                            \
    auto lhs     = vm.stack.back();                                          \
    auto new_val = SILVA_EXPECT_FWD(obj_name(vm.pool, lhs));                 \
    vm.stack.pop_back();                                                     \
    vm.stack.push_back(std::move(new_val));                                  \
    ip += 1;                                                                 \
    return {};                                                               \
  }
    SIMPLE_UNARY_OP(_not, neg);
    SIMPLE_UNARY_OP(_negate, inv);
#undef SIMPLE_UNARY_OP

    expected_t<void> _print()
    {
      SILVA_EXPECT(vm.stack.size() >= 1,
                   RUNTIME,
                   "{} bytecode instruction PRINT needs non-empty stack",
                   chunk.origin_info_at_instr(ip));
      auto x = vm.stack.back();
      vm.print_stream->format("{}\n", pretty_string(std::move(x)));
      vm.stack.pop_back();
      ip += 1;
      return {};
    }
    expected_t<void> _jump()
    {
      const auto offset = bit_cast_ptr<index_t>(&bytecode[ip + 1]);
      ip += offset;
      return {};
    }
    expected_t<void> _jump_if_false()
    {
      const auto offset = bit_cast_ptr<index_t>(&bytecode[ip + 1]);
      SILVA_EXPECT(vm.stack.size() >= 1,
                   RUNTIME,
                   "{} bytecode instruction JUMP_IF_FALSE needs non-empty stack",
                   chunk.origin_info_at_instr(ip));
      if (!vm.stack.back()->is_truthy()) {
        ip += offset;
      }
      else {
        ip += 5;
      }
      return {};
    }
    expected_t<void> _loop() { SILVA_EXPECT(false, ASSERT); }
    expected_t<void> _call() { SILVA_EXPECT(false, ASSERT); }
    expected_t<void> _invoke() { SILVA_EXPECT(false, ASSERT); }
    expected_t<void> _super_invoke() { SILVA_EXPECT(false, ASSERT); }
    expected_t<void> _closure() { SILVA_EXPECT(false, ASSERT); }
    expected_t<void> _close_upvalue() { SILVA_EXPECT(false, ASSERT); }
    expected_t<void> _return() { SILVA_EXPECT(false, ASSERT); }
    expected_t<void> _class() { SILVA_EXPECT(false, ASSERT); }
    expected_t<void> _inherit() { SILVA_EXPECT(false, ASSERT); }
    expected_t<void> _method() { SILVA_EXPECT(false, ASSERT); }

    expected_t<void> any()
    {
      switch (opcode_t(chunk.bytecode[ip])) {
        case CONSTANT:
          return _constant();
        case NIL:
          return _nil();
        case TRUE:
          return _true();
        case FALSE:
          return _false();
        case POP:
          return _pop();
        case GET_LOCAL:
          return _get_local();
        case SET_LOCAL:
          return _set_local();
        case GET_GLOBAL:
          return _get_global();
        case DEFINE_GLOBAL:
          return _define_global();
        case SET_GLOBAL:
          return _set_global();
        case GET_UPVALUE:
          return _get_upvalue();
        case SET_UPVALUE:
          return _set_upvalue();
        case GET_PROPERTY:
          return _get_property();
        case SET_PROPERTY:
          return _set_property();
        case GET_SUPER:
          return _get_super();
        case EQUAL:
          return _equal();
        case GREATER:
          return _greater();
        case LESS:
          return _less();
        case ADD:
          return _add();
        case SUBTRACT:
          return _subtract();
        case MULTIPLY:
          return _multiply();
        case DIVIDE:
          return _divide();
        case NOT:
          return _not();
        case NEGATE:
          return _negate();
        case PRINT:
          return _print();
        case JUMP:
          return _jump();
        case JUMP_IF_FALSE:
          return _jump_if_false();
        case LOOP:
          return _loop();
        case CALL:
          return _call();
        case INVOKE:
          return _invoke();
        case SUPER_INVOKE:
          return _super_invoke();
        case CLOSURE:
          return _closure();
        case CLOSE_UPVALUE:
          return _close_upvalue();
        case RETURN:
          return _return();
        case CLASS:
          return _class();
        case INHERIT:
          return _inherit();
        case METHOD:
          return _method();
        default:
          SILVA_EXPECT(false,
                       MAJOR,
                       "unknown instruction '{}' at instruction-pointer {}",
                       uint8_t(chunk.bytecode[ip]),
                       ip);
      }
    }

    expected_t<void> go()
    {
      while (ip < bytecode.size()) {
        SILVA_EXPECT_FWD(any(), "while executing instruction {}", chunk.origin_info_at_instr(ip));
      }
      return {};
    }
  };

  expected_t<void> vm_t::run(const chunk_t& chunk)
  {
    runner_t runner{chunk, chunk.bytecode, *this};
    SILVA_EXPECT_FWD_PLAIN(runner.go());
    return {};
  }

  expected_t<string_t> vm_t::to_string() const
  {
    return {};
  }
}
