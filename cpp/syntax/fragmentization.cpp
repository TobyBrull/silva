#include "fragmentization.hpp"

#include "canopy/filesystem.hpp"
#include "canopy/pretty_write.hpp"
#include "canopy/unicode.hpp"

namespace silva {
  using enum codepoint_category_t;
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
    auto retval      = std::make_unique<fragmentization_t>();
    retval->filepath = std::move(descriptive_path);

    const auto f = [&](const unicode::codepoint_t cp,
                       const index_t bo,
                       const index_t len) -> expected_t<void> {
      const codepoint_category_t cc = codepoint_category_table[cp];
      SILVA_EXPECT(cc != Forbidden, MINOR, "Forbidden codepoint 0x{:04x}", cp);
      if (cc == Newline) {
      }
      else if (cc == Space) {
      }
      else if (cc == Operator) {
      }
      else if (cc == ParenthesisLeft) {
      }
      else if (cc == ParenthesisRight) {
      }
      else if (cc == XID_Continue) {
      }
      else if (cc == XID_Start) {
      }
      return {};
    };
    auto res = unicode::for_each_codepoint(source_code, f);
    SILVA_EXPECT_FWD(std::move(res));

    return {std::move(retval)};
  }

  void pretty_write_impl(const fragmentization_t& self, byte_sink_t* stream)
  {
    const index_t n = self.categories.size();
    SILVA_ASSERT(n == self.locations.size());
    for (index_t idx = 0; idx < n; ++idx) {
      const index_t start_index = self.locations[idx].byte_offset;
      const index_t end_index =
          (idx + 1 < n) ? self.locations[idx + 1].byte_offset : self.source_code.size();
      const string_view_t sv =
          string_view_t{self.source_code}.substr(start_index, end_index - start_index);
      const fragment_category_t fc = self.categories[idx];
      stream->format("[{:3}] [", start_index);
      silva::pretty_write(fc, stream);
      stream->format("] [{}]\n", sv);
    }
  }
}
