#pragma once

#include "expected.hpp"
#include "types.hpp"

namespace silva {
  expected_t<string_t> read_file(const filepath_t&);
  expected_t<void> write_file(const filepath_t&, string_view_t content);

  optional_t<string_t> run_shell_command_sync(const string_t& command) noexcept;

  class temp_dir_t {
    filepath_t dir_path;

   public:
    ~temp_dir_t();
    [[nodiscard]] temp_dir_t(const filepath_t& dir_path_pattern = "tmp_XXXXXX");

    temp_dir_t(const temp_dir_t&)            = delete;
    temp_dir_t& operator=(const temp_dir_t&) = delete;

    const filepath_t& get_dir_path() const;
  };

  void graphviz_show_sync(string_view_t);
}
