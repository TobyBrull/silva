#pragma once

#include "bytecode.hpp"

#include "canopy/byte_sink.hpp"

namespace silva::lox::bytecode {
  struct vm_t {
    syntax_ward_ptr_t swp;
    object_pool_t* object_pool = nullptr;
    vector_t<object_ref_t> stack;
    vector_t<object_ref_t> open_upvalues;
    hash_map_t<token_id_t, object_ref_t> globals;

    struct call_frame_t {
      object_ref_t closure;
      object_ref_t func;
      const chunk_t* chunk = nullptr;
      index_t ip           = 0;
      index_t stack_offset = 0;
    };
    vector_t<call_frame_t> call_frames;
    string_t call_frames_to_string() const;

    byte_sink_t* print_target = nullptr;

    vm_t(syntax_ward_ptr_t, object_pool_t*, byte_sink_t*);

    expected_t<void> run(const chunk_t&);

    expected_t<string_t> to_string() const;
  };
}
