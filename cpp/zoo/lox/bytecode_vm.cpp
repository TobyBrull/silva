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
      vm.stack.push_back(chunk.constant_table[index_t(bytecode[ip + 1])]);
      ip += 2;
      return {};
    }

    expected_t<void> nil() { return {}; }
    expected_t<void> _true() { return {}; }
    expected_t<void> _false() { return {}; }
    expected_t<void> _pop() { return {}; }
    expected_t<void> _get_local() { return {}; }
    expected_t<void> _set_local() { return {}; }
    expected_t<void> _get_global() { return {}; }
    expected_t<void> _define_global() { return {}; }
    expected_t<void> _set_global() { return {}; }
    expected_t<void> _get_upvalue() { return {}; }
    expected_t<void> _set_upvalue() { return {}; }
    expected_t<void> _get_property() { return {}; }
    expected_t<void> _set_property() { return {}; }
    expected_t<void> _get_super() { return {}; }
    expected_t<void> _equal() { return {}; }
    expected_t<void> _greater() { return {}; }
    expected_t<void> _less() { return {}; }
    expected_t<void> _add() { return {}; }
    expected_t<void> _subtract() { return {}; }
    expected_t<void> _multiply() { return {}; }
    expected_t<void> _divide() { return {}; }
    expected_t<void> _not() { return {}; }
    expected_t<void> _negate() { return {}; }
    expected_t<void> _print() { return {}; }
    expected_t<void> _jump() { return {}; }
    expected_t<void> _jump_if_false() { return {}; }
    expected_t<void> _loop() { return {}; }
    expected_t<void> _call() { return {}; }
    expected_t<void> _invoke() { return {}; }
    expected_t<void> _super_invoke() { return {}; }
    expected_t<void> _closure() { return {}; }
    expected_t<void> _close_upvalue() { return {}; }
    expected_t<void> _return() { return {}; }
    expected_t<void> _class() { return {}; }
    expected_t<void> _inherit() { return {}; }
    expected_t<void> _method() { return {}; }

    expected_t<void> any()
    {
      switch (opcode_t(chunk.bytecode[ip])) {
        case CONSTANT:
          return _constant();
        case NIL:
          return nil();
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
  };

  expected_t<void> vm_t::run(const chunk_t& chunk)
  {
    runner_t runner{chunk, chunk.bytecode, *this};
    SILVA_EXPECT_FWD(runner.any());
    return {};
  }

  expected_t<string_t> vm_t::to_string() const
  {
    return {};
  }
}
