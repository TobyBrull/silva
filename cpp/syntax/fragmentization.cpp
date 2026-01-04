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

    struct categorized_codepoint_data_t : public unicode::codepoint_data_t {
      codepoint_category_t category = codepoint_category_t::Forbidden;
      file_location_t location;
    };
    file_location_t loc;
    array_t<categorized_codepoint_data_t> ccd;
    for (auto maybe_ud: unicode::utf8_decode_generator(source_code)) {
      const auto ud                 = SILVA_EXPECT_FWD(std::move(maybe_ud));
      const codepoint_category_t cc = codepoint_category_table[ud.codepoint];
      SILVA_EXPECT(cc != Forbidden, MINOR, "Forbidden codepoint 0x{:04x}", ud.codepoint);
      ccd.emplace_back(ud, cc, loc);
      if (ud.codepoint == U'\n') {
        loc.line_num += 1;
        loc.column = 0;
      }
      else {
        loc.column += 1;
      }
      loc.byte_offset += ud.len;
    }

    index_t ccd_idx = 0;
    const index_t n = ccd.size();
    while (ccd_idx < n) {
      if (ccd[ccd_idx].category == Newline) {
        retval->categories.push_back(NEWLINE);
        retval->locations.push_back(ccd[ccd_idx].location);
        do {
          ++ccd_idx;
        } while (ccd_idx < n && ccd[ccd_idx].category == Newline);
      }
      else {
        ++ccd_idx;
      }
    }

    retval->source_code = std::move(source_code);
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
      stream->format("] [{}]\n", hexdump(sv));
    }
  }
}
