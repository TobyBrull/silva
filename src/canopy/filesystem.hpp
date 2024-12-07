#pragma once

#include "types.hpp"

namespace silva {
  string_t read_file(const filesystem_path_t& filename);
  bool write_file(const filesystem_path_t& filename, string_view_t content);

  optional_t<string_t> run_shell_command_sync(const std::string& command) noexcept;

  class temp_dir_t {
    filesystem_path_t dir_path;

   public:
    ~temp_dir_t();
    [[nodiscard]] temp_dir_t(const filesystem_path_t& dir_path_pattern = "tmp_XXXXXX");

    temp_dir_t(const temp_dir_t&)            = delete;
    temp_dir_t& operator=(const temp_dir_t&) = delete;

    const filesystem_path_t& get_dir_path() const;
  };

  void graphviz_show_sync(string_view_t);
}
