#pragma once

#include "expected.hpp"
#include "types.hpp"

namespace silva {
  struct source_code_t : public menhir_t {
    const string_t filename;
    const string_t text;

    source_code_t(string_t fn, string_t tx);

    unique_ptr_t<source_code_t> copy() const;

    static expected_t<unique_ptr_t<source_code_t>> load(filesystem_path_t);
  };

  struct source_location_t {
    const source_code_t* source_code = nullptr;

    index_t line   = 0;
    index_t column = 0;
  };
}
