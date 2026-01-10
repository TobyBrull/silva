#include "fragmentization.hpp"

#include "canopy/expected.hpp"
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

  struct codepoint_wrap_t {
    unicode::codepoint_t codepoint = 0;
    codepoint_category_t category  = codepoint_category_t::Forbidden;
    file_location_t location;

    friend void pretty_write_impl(const codepoint_wrap_t& x, byte_sink_t* byte_sink)
    {
      byte_sink->format("codepoint[{}, 0x{:04x}, {}] at ",
                        unicode::utf8_encode_one(x.codepoint),
                        int(x.codepoint),
                        rfl::enum_to_string(x.category));
      silva::pretty_write(x.location, byte_sink);
    }
  };

  struct fragmentizer_t {
    unique_ptr_t<fragmentization_t> retval = std::make_unique<fragmentization_t>();

    fragmentizer_t(filesystem_path_t descriptive_path, string_t source_code)
    {
      retval->filepath    = std::move(descriptive_path);
      retval->source_code = std::move(source_code);
    }

    struct categorized_codepoint_data_t : public unicode::codepoint_data_t {
      codepoint_category_t category = codepoint_category_t::Forbidden;
      file_location_t location;

      codepoint_wrap_t to_wrap() const
      {
        return codepoint_wrap_t{
            .codepoint = codepoint,
            .category  = category,
            .location  = location,
        };
      }
    };
    array_t<categorized_codepoint_data_t> ccd;
    index_t i = 0;
    index_t n = 0;
    expected_t<void> init()
    {
      file_location_t loc;
      for (auto maybe_ud: unicode::utf8_decode_generator(retval->source_code)) {
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
      n = ccd.size();
      return {};
    }

    void emit(const index_t idx, const fragment_category_t fc)
    {
      retval->locations.push_back(ccd[idx].location);
      retval->categories.push_back(fc);
    }

    index_t prev_indent = 0;
    bool recognize_indent(const index_t idx, const index_t indent)
    {
      bool retval = false;
      if (prev_indent < indent) {
        emit(idx, DEDENT);
        retval = true;
      }
      else if (prev_indent > indent) {
        emit(idx, INDENT);
        retval = true;
      }
      prev_indent = indent;
      return retval;
    }

    expected_t<void> run()
    {
      SILVA_EXPECT_FWD(recognize_various_following_newline());
      while (i < n) {
        if (ccd[i].category == Newline) {
          emit(i++, NEWLINE);
          SILVA_EXPECT_FWD(recognize_various_following_newline());
          continue;
        }
        if (ccd[i].category == ParenthesisLeft) {
          emit(i++, PARENTHESIS_LEFT);
        }
        else if (ccd[i].category == ParenthesisRight) {
          emit(i++, PARENTHESIS_RIGHT);
        }
        else if (ccd[i].category == Operator) {
          if (ccd[i].codepoint == U'"' || ccd[i].codepoint == U'\'') {
            SILVA_EXPECT_FWD(recognize_string());
          }
          emit(i++, OPERATOR);
        }
        else if (ccd[i].category == XID_Start) {
          SILVA_EXPECT_FWD(recognize_identifier());
        }
        else if (ccd[i].category == XID_Continue) {
          SILVA_EXPECT_FWD(recognize_number());
        }
        else {
          SILVA_EXPECT(false, MINOR, "fragmentization doesn't allow {}", ccd[i].to_wrap());
        }
      }
      recognize_indent(n, 0);
      return {};
    }

    expected_t<void> recognize_identifier()
    {
      emit(i++, IDENTIFIER);
      return {};
    }

    expected_t<void> recognize_number()
    {
      SILVA_EXPECT('0' <= ccd[i].codepoint && ccd[i].codepoint < '9',
                   MINOR,
                   "fragmentization doesn't allow {}",
                   ccd[i].to_wrap());
      emit(i++, NUMBER);
      return {};
    }

    expected_t<void> recognize_string()
    {
      emit(i++, STRING);
      return {};
    }

    // Gobbles up all whitespace and comments after a newline. Leaves "i" at the first codepoint
    // that is not a whitespace, newline, or part of a comment.
    expected_t<void> recognize_various_following_newline()
    {
      index_t prev_i      = i;
      index_t last_indent = 0;
      while (i < n) {
        if (is_comment_start(i)) {
          if (prev_i < i) {
            emit(prev_i, WHITESPACE);
          }
          SILVA_EXPECT_FWD(recognize_comment());
          prev_i = i;
        }
        else if (ccd[i].category == Space) {
          last_indent += 1;
          ++i;
        }
        else if (ccd[i].category == Newline) {
          last_indent = 0;
          ++i;
        }
        else {
          break;
        }
      }
      if (i == n) {
        if (prev_i < i) {
          emit(prev_i, WHITESPACE);
        }
      }
      else {
        if (prev_i < i - last_indent) {
          emit(prev_i, WHITESPACE);
          prev_i = i - last_indent;
        }
        const bool emitted_indent = recognize_indent(prev_i, last_indent);
        if (!emitted_indent && last_indent > 0) {
          emit(prev_i, WHITESPACE);
        }
      }
      return {};
    };

    bool is_comment_start(const index_t idx) { return (idx < n && ccd[idx].codepoint == U'#'); }

    // Leaves "i" on newline or at EOF.
    expected_t<bool> recognize_comment()
    {
      SILVA_EXPECT(is_comment_start(i), ASSERT);
      emit(i, COMMENT);
      while (i < n && ccd[i].category != Newline) {
        ++i;
      }
      return {};
    }
  };

  expected_t<unique_ptr_t<fragmentization_t>> fragmentize(filesystem_path_t descriptive_path,
                                                          string_t source_code)
  {
    fragmentizer_t ff(std::move(descriptive_path), std::move(source_code));
    SILVA_EXPECT_FWD(ff.init());
    SILVA_EXPECT_FWD(ff.run());
    return std::move(ff.retval);
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
