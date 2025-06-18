#include "bytecode_vm.hpp"

namespace silva::lox::bytecode {
  expected_t<object_ref_t> vm_t::run(const chunk_t&)
  {
    return pool.make(11.0);
  }

  expected_t<string_t> vm_t::to_string() const
  {
    return {};
  }
}
