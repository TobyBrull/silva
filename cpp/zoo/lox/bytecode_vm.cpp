#include "bytecode_vm.hpp"

namespace silva::lox::bytecode {

  using enum opcode_t;

  vm_t::vm_t(syntax_ward_ptr_t swp, object_pool_t* object_pool, byte_sink_t* print_target)
    : swp(swp), object_pool(object_pool), print_target(print_target)
  {
  }

  string_t vm_t::call_frames_to_string() const
  {
    string_t retval;
    for (const auto& cf: call_frames) {
      if (cf.func.is_nullptr()) {
        retval += fmt::format("  - main_script\n");
      }
      else {
        retval += fmt::format("  - {} instr {}\n", pretty_string(cf.func), cf.ip);
      }
    }
    return retval;
  }

  expected_t<object_ref_t*> stack_by(vm_t& vm, const index_t offset)
  {
    const index_t idx = vm.call_frames.back().stack_offset + offset;
    SILVA_EXPECT(0 <= idx && idx < vm.stack.size(),
                 RUNTIME,
                 "Stack access with offset={} idx={} stack-size={} is out-of-bounds",
                 offset,
                 idx,
                 vm.stack.size());
    return &vm.stack[idx];
  }

  struct runner_t {
    vm_t& vm;

    index_t curr_index_in_instr(const index_t offset = 0)
    {
      const auto& ccf = vm.call_frames.back();
      return bit_cast_ptr<index_t>(&ccf.chunk->bytecode[ccf.ip + 1 + offset * sizeof(index_t)]);
    }
    index_t& curr_ip()
    {
      auto& ccf = vm.call_frames.back();
      return ccf.ip;
    }
    vector_t<object_ref_t> curr_constant_table()
    {
      auto& ccf = vm.call_frames.back();
      return ccf.chunk->constant_table;
    }
    parse_tree_span_t curr_info_at_instr()
    {
      auto& ccf = vm.call_frames.back();
      return ccf.chunk->origin_info_at_instr(ccf.ip);
    }
    opcode_t curr_opcode()
    {
      auto& ccf = vm.call_frames.back();
      return opcode_t(ccf.chunk->bytecode[ccf.ip]);
    }
    const chunk_t& curr_chunk()
    {
      auto& ccf = vm.call_frames.back();
      return *ccf.chunk;
    }

    expected_t<void> _constant()
    {
      const index_t idx = curr_index_in_instr();
      const auto& ct    = curr_constant_table();
      vm.stack.push_back(ct[idx]);
      curr_ip() += 5;
      return {};
    }
    expected_t<void> _nil()
    {
      vm.stack.push_back(vm.object_pool->const_nil);
      curr_ip() += 1;
      return {};
    }
    expected_t<void> _true()
    {
      vm.stack.push_back(vm.object_pool->const_true);
      curr_ip() += 1;
      return {};
    }
    expected_t<void> _false()
    {
      vm.stack.push_back(vm.object_pool->const_false);
      curr_ip() += 1;
      return {};
    }
    expected_t<void> _pop()
    {
      vm.stack.pop_back();
      curr_ip() += 1;
      return {};
    }
    expected_t<void> _get_local()
    {
      const index_t stack_idx = curr_index_in_instr();
      SILVA_EXPECT(0 <= stack_idx && stack_idx < vm.stack.size(),
                   RUNTIME,
                   "{} stack index {} not inside stack GET_LOCAL",
                   curr_info_at_instr(),
                   stack_idx);
      vm.stack.push_back(*SILVA_EXPECT_FWD(stack_by(vm, stack_idx)));
      curr_ip() += 5;
      return {};
    }
    expected_t<void> _set_local()
    {
      const index_t stack_idx = curr_index_in_instr();
      SILVA_EXPECT(0 <= stack_idx && stack_idx < vm.stack.size(),
                   RUNTIME,
                   "{} stack index {} not inside stack in SET_LOCAL",
                   curr_info_at_instr(),
                   stack_idx);
      *SILVA_EXPECT_FWD(stack_by(vm, stack_idx)) = vm.stack.back();
      curr_ip() += 5;
      return {};
    }
    expected_t<void> _get_global()
    {
      const index_t ti = curr_index_in_instr();
      auto it          = vm.globals.find(ti);
      SILVA_EXPECT(it != vm.globals.end(),
                   RUNTIME,
                   "{} couldn't find global variable {}",
                   curr_info_at_instr(),
                   vm.swp->token_id_wrap(ti));
      vm.stack.push_back(it->second);
      curr_ip() += 5;
      return {};
    }
    expected_t<void> _define_global()
    {
      const index_t ti = curr_index_in_instr();
      SILVA_EXPECT(vm.stack.size() >= 1,
                   RUNTIME,
                   "{} bytecode instruction DEFINE_GLOBAL needs non-empty stack",
                   curr_info_at_instr());
      vm.globals[ti] = vm.stack.back();
      vm.stack.pop_back();
      curr_ip() += 5;
      return {};
    }
    expected_t<void> _set_global()
    {
      const index_t ti = curr_index_in_instr();
      SILVA_EXPECT(vm.stack.size() >= 1,
                   RUNTIME,
                   "{} bytecode instruction SET_GLOBAL needs non-empty stack",
                   curr_info_at_instr());
      auto it = vm.globals.find(ti);
      SILVA_EXPECT(it != vm.globals.end(),
                   RUNTIME,
                   "{} bytecode instruction SET_GLOBAL tried to assign to global variable {} which "
                   "was not previously defined",
                   curr_info_at_instr(),
                   vm.swp->token_id_wrap(ti));
      it->second = vm.stack.back();
      curr_ip() += 5;
      return {};
    }
    expected_t<void> _get_property() { SILVA_EXPECT(false, ASSERT); }
    expected_t<void> _set_property() { SILVA_EXPECT(false, ASSERT); }
    expected_t<void> _get_super() { SILVA_EXPECT(false, ASSERT); }

#define SIMPLE_BINARY_OP(here_name, obj_name)                                           \
  expected_t<void> here_name()                                                          \
  {                                                                                     \
    SILVA_EXPECT(vm.stack.size() >= 2,                                                  \
                 RUNTIME,                                                               \
                 "{} bytecode instruction 'obj_name' needs at least two stack entries", \
                 curr_info_at_instr());                                                 \
    auto rhs     = vm.stack.back();                                                     \
    auto lhs     = vm.stack[vm.stack.size() - 2];                                       \
    auto new_val = SILVA_EXPECT_FWD_PLAIN(obj_name(*vm.object_pool, lhs, rhs));         \
    vm.stack.pop_back();                                                                \
    vm.stack.pop_back();                                                                \
    vm.stack.push_back(std::move(new_val));                                             \
    curr_ip() += 1;                                                                     \
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
                 curr_info_at_instr());                                      \
    auto lhs     = vm.stack.back();                                          \
    auto new_val = SILVA_EXPECT_FWD(obj_name(*vm.object_pool, lhs));         \
    vm.stack.pop_back();                                                     \
    vm.stack.push_back(std::move(new_val));                                  \
    curr_ip() += 1;                                                          \
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
                   curr_info_at_instr());
      auto x = vm.stack.back();
      vm.print_target->format("{}\n", pretty_string(std::move(x)));
      vm.stack.pop_back();
      curr_ip() += 1;
      return {};
    }
    expected_t<void> _jump()
    {
      const index_t offset = curr_index_in_instr();
      curr_ip() += offset;
      return {};
    }
    expected_t<void> _jump_if_false()
    {
      const index_t offset = curr_index_in_instr();
      SILVA_EXPECT(vm.stack.size() >= 1,
                   RUNTIME,
                   "{} bytecode instruction JUMP_IF_FALSE needs non-empty stack",
                   curr_info_at_instr());
      if (!vm.stack.back()->is_truthy()) {
        curr_ip() += offset;
      }
      else {
        curr_ip() += 5;
      }
      return {};
    }
    expected_t<void> _call()
    {
      const index_t num_args = curr_index_in_instr();
      SILVA_EXPECT(vm.stack.size() >= num_args + 1,
                   RUNTIME,
                   "not enough elements on stack for call to function with {} arguments",
                   num_args);
      const index_t sp = vm.stack.size() - 1 - num_args;
      const auto& func = vm.stack[sp];
      SILVA_EXPECT(func->holds_function(), RUNTIME, "object {} cannot be called");
      const function_t& ffunc = std::get<function_t>(func->data);
      curr_ip() += 5;
      vm.call_frames.push_back(vm_t::call_frame_t{
          .func         = func,
          .chunk        = ffunc.chunk.get(),
          .ip           = 0,
          .stack_offset = sp,
      });
      return {};
    }
    expected_t<void> _invoke() { SILVA_EXPECT(false, ASSERT); }
    expected_t<void> _super_invoke() { SILVA_EXPECT(false, ASSERT); }

    expected_t<void> _get_upvalue() { SILVA_EXPECT(false, ASSERT); }
    expected_t<void> _set_upvalue() { SILVA_EXPECT(false, ASSERT); }
    expected_t<void> _closure() { SILVA_EXPECT(false, ASSERT); }
    expected_t<void> _close_upvalue() { SILVA_EXPECT(false, ASSERT); }

    expected_t<void> _return()
    {
      SILVA_EXPECT(!vm.stack.empty(), RUNTIME, "stack empty for return statement");
      object_ref_t retval = vm.stack.back();
      const auto& ccf     = vm.call_frames.back();
      vm.stack.resize(ccf.stack_offset);
      vm.stack.push_back(retval);
      vm.call_frames.pop_back();
      return {};
    }
    expected_t<void> _class() { SILVA_EXPECT(false, ASSERT); }
    expected_t<void> _inherit() { SILVA_EXPECT(false, ASSERT); }
    expected_t<void> _method() { SILVA_EXPECT(false, ASSERT); }

    expected_t<void> any()
    {
      switch (curr_opcode()) {
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
                       uint8_t(curr_opcode()),
                       curr_ip());
      }
    }

    expected_t<void> go()
    {
      while (curr_ip() < curr_chunk().bytecode.size()) {
        SILVA_EXPECT_FWD(any(),
                         "while executing instruction {} (corresponding to {})",
                         curr_ip(),
                         curr_info_at_instr());
      }
      return {};
    }
  };

  expected_t<void> vm_t::run(const chunk_t& chunk)
  {
    call_frames.push_back(vm_t::call_frame_t{
        .func         = {},
        .chunk        = &chunk,
        .ip           = 0,
        .stack_offset = 0,
    });
    runner_t runner{*this};
    SILVA_EXPECT_FWD(runner.go(), "callstack:\n{}", call_frames_to_string());
    SILVA_EXPECT(call_frames.size() == 1,
                 MAJOR,
                 "non-trivial callstack state at end of vm_t::run()");
    call_frames.pop_back();
    return {};
  }

  expected_t<string_t> vm_t::to_string() const
  {
    return {};
  }
}
