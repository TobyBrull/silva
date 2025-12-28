#include "fragmentization.hpp"

#include "canopy/filesystem.hpp"
#include "canopy/pretty_write.hpp"

namespace silva {
  using enum fragment_category_t;

  expected_t<unique_ptr_t<fragmentization_t>> fragmentize_load(filesystem_path_t filepath)
  {
    string_t source_code = SILVA_EXPECT_FWD(read_file(filepath));
    auto retval = SILVA_EXPECT_FWD_PLAIN(fragmentize(std::move(filepath), std::move(source_code)));
    return retval;
  }

  expected_t<unique_ptr_t<fragmentization_t>> fragmentize(filesystem_path_t descriptive_path,
                                                          string_t source_code)
  {
    const index_t n  = source_code.size();
    auto retval      = std::make_unique<fragmentization_t>();
    retval->filepath = std::move(descriptive_path);
    index_t idx      = 0;
    while (idx < n) {
      idx += 1;
    }
    return {std::move(retval)};
  }

  void pretty_write_impl(const fragmentization_t& self, byte_sink_t* stream)
  {
    const index_t n = self.fragment_start_indexes.size();
    SILVA_ASSERT(n == self.categories.size());
    for (index_t idx = 0; idx < n; ++idx) {
      const index_t start_index = self.fragment_start_indexes[idx];
      const index_t end_index =
          (idx + 1 < n) ? self.fragment_start_indexes[idx + 1] : self.source_code.size();
      const string_view_t sv =
          string_view_t{self.source_code}.substr(start_index, end_index - start_index);
      const fragment_category_t fc = self.categories[idx];
      stream->format("[{:3}] [", start_index);
      silva::pretty_write(fc, stream);
      stream->format("] [{}]\n", sv);
    }
  }
}
