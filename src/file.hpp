#pragma once

#include <filesystem>
#include <string>

namespace silva {
  std::string read_file(const std::filesystem::path& filename);
  bool write_file(const std::filesystem::path& filename, std::string_view content);

  std::optional<std::string> run_shell_command_sync(const std::string& command) noexcept;

  class temp_dir_t {
    std::filesystem::path dir_path;

   public:
    ~temp_dir_t();
    [[nodiscard]] temp_dir_t(const std::filesystem::path& dir_path_pattern = "tmp_XXXXXX");

    temp_dir_t(const temp_dir_t&)            = delete;
    temp_dir_t& operator=(const temp_dir_t&) = delete;

    const std::filesystem::path& get_dir_path() const;
  };

  void graphviz_show_sync(std::string_view);
}
