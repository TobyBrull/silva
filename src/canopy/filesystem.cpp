#include "filesystem.hpp"

#include "assert.hpp"

#include <unistd.h>

#include <fstream>
#include <sstream>

namespace silva {
  std::string read_file(const filesystem_path_t& filename)
  {
    std::ifstream file(filename);
    SILVA_ASSERT(file.is_open());
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
  }

  bool write_file(const filesystem_path_t& filename, const string_view_t content)
  {
    std::ofstream file(filename);
    SILVA_ASSERT(file.is_open());
    file << content;
    return file.good();
  }

  std::optional<std::string> run_shell_command_sync(const std::string& command) noexcept
  {
    std::array<char, 128> buffer;
    std::string output;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
    if (!pipe) {
      return std::nullopt;
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
      output += buffer.data();
    }
    const int exit_code = pclose(pipe.release());
    if (exit_code != 0) {
      return std::nullopt;
    }
    return {std::move(output)};
  }

  temp_dir_t::~temp_dir_t()
  {
    if (!dir_path.empty()) {
      const auto rv = std::filesystem::remove_all(dir_path);
    }
  }

  temp_dir_t::temp_dir_t(const filesystem_path_t& dir_path_pattern)
  {
    std::string dpp    = dir_path_pattern;
    const char* result = mkdtemp(dpp.data());
    SILVA_ASSERT(result);
    dir_path = result;
  }

  const filesystem_path_t& temp_dir_t::get_dir_path() const
  {
    return dir_path;
  }

  void graphviz_show_sync(const string_view_t dotfile)
  {
    temp_dir_t td;
    const filesystem_path_t path_dot = td.get_dir_path() / "silva.dot";
    const filesystem_path_t path_png = td.get_dir_path() / "silva.png";
    SILVA_ASSERT(write_file(path_dot, dotfile));
    run_shell_command_sync(fmt::format("dot -Tpng {} -o {}", path_dot.string(), path_png.string()));
    run_shell_command_sync(fmt::format("eog {}", path_png.string()));
  }
}
