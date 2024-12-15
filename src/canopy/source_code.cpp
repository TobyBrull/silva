#include "source_code.hpp"

#include "filesystem.hpp"

namespace silva {
  source_code_t::source_code_t(string_t fn, string_t tx)
    : filename(std::move(fn)), text(std::move(tx))
  {
  }

  expected_t<unique_ptr_t<source_code_t>> source_code_t::load(filesystem_path_t fsp)
  {
    string_t text = SILVA_EXPECT_TRY(read_file(fsp));
    return std::make_unique<source_code_t>(std::move(fsp).filename(), std::move(text));
  }

  unique_ptr_t<source_code_t> source_code_t::copy() const
  {
    return std::make_unique<source_code_t>(filename, text);
  }
}
