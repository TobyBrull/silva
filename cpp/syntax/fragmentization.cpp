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

  constexpr bool is_ascii_digit(const unicode::codepoint_t cp)
  {
    return '0' <= cp && cp < '9';
  }
  constexpr bool is_ascii_alpha(const unicode::codepoint_t cp)
  {
    return ('a' <= cp && cp < 'z') || ('A' <= cp && cp < 'Z');
  }

  struct codepoint_wrap_t {
    unicode::codepoint_t codepoint = 0;
    codepoint_category_t category  = codepoint_category_t::Forbidden;
    file_location_t location;

    friend void pretty_write_impl(const codepoint_wrap_t& x, byte_sink_t* byte_sink)
    {
      byte_sink->format("codepoint[ {}, 0x{:04x}, {}] at ",
                        unicode::utf8_encode_one(x.codepoint),
                        uint32_t(x.codepoint),
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
        const unicode::codepoint_data_t ud = SILVA_EXPECT_FWD(std::move(maybe_ud));
        const codepoint_category_t cc      = codepoint_category_table[ud.codepoint];
        categorized_codepoint_data_t cc2{ud, cc, loc};
        SILVA_EXPECT(cc != Forbidden, MINOR, "Forbidden {}", cc2.to_wrap());
        ccd.emplace_back(std::move(cc2));
        if (ud.codepoint == U'\n') {
          loc.line_num += 1;
          loc.column = 0;
        }
        else {
          loc.column += 1;
        }
        loc.byte_offset += ud.len;
      }
      SILVA_EXPECT(!ccd.empty() && ccd.back().codepoint == U'\n',
                   MINOR,
                   "source-code expected to end with newline");
      n = ccd.size();
      ccd.push_back(categorized_codepoint_data_t{
          unicode::codepoint_data_t{
              .codepoint   = U'\0',
              .byte_offset = index_t(retval->source_code.size()),
              .len         = 0,
          },
          Forbidden,
          loc,
      });
      return {};
    }

    void emit(const index_t idx, const fragment_category_t fc)
    {
      retval->fragments.push_back(fragmentization_t::fragment_t{
          .category = fc,
          .location = ccd[idx].location,
      });
    }

    array_t<categorized_codepoint_data_t> parentheses;
    array_t<index_t> indents = {0};

    expected_t<bool> recognize_indent(const index_t idx, const index_t indent)
    {
      SILVA_EXPECT(indent >= 0, ASSERT);
      SILVA_EXPECT(!indents.empty(), ASSERT);
      if (indents.back() < indent) {
        emit(idx, INDENT);
        indents.push_back(indent);
        return true;
      }
      else if (indents.back() > indent) {
        while (indents.back() > indent) {
          emit(idx, DEDENT);
          indents.pop_back();
        }
        SILVA_EXPECT(indents.back() == indent,
                     MINOR,
                     "inconsistent indent: indent at {} doesn't match any previous indent",
                     ccd[idx].location);
        return true;
      }
      return false;
    }

    expected_t<void> run()
    {
      SILVA_EXPECT_FWD(recognize_various_following_newline());
      while (i < n) {
        if (ccd[i].category == Newline) {
          if (parentheses.empty()) {
            emit(i++, NEWLINE);
          }
          SILVA_EXPECT_FWD(recognize_various_following_newline());
        }
        else if (ccd[i].category == Space) {
          emit(i, WHITESPACE);
          while (i < n && ccd[i].category == Space) {
            ++i;
          }
        }
        else if (ccd[i].category == ParenthesisLeft) {
          parentheses.push_back(ccd[i]);
          emit(i++, PAREN_LEFT);
        }
        else if (ccd[i].category == ParenthesisRight) {
          const auto expected_open_paren_it = opposite_parenthesis.find(ccd[i].codepoint);
          SILVA_EXPECT(expected_open_paren_it != opposite_parenthesis.end(), ASSERT);
          const unicode::codepoint_t expected_open_paren = expected_open_paren_it->second;
          SILVA_EXPECT(!parentheses.empty() && parentheses.back().codepoint == expected_open_paren,
                       MINOR,
                       "Mismatching parentheses between {} and {}",
                       parentheses.back().location,
                       ccd[i].location);
          parentheses.pop_back();
          emit(i++, PAREN_RIGHT);
        }
        else if (ccd[i].category == Operator) {
          if (ccd[i].codepoint == U'#') {
            SILVA_EXPECT_FWD(recognize_comment());
            continue;
          }
          if (!SILVA_EXPECT_FWD(try_recognize_string())) {
            emit(i++, OPERATOR);
          }
        }
        else if (ccd[i].category == XID_Start) {
          SILVA_EXPECT_FWD(recognize_identifier());
        }
        else if (is_ascii_digit(ccd[i].codepoint)) {
          SILVA_EXPECT_FWD(recognize_number());
        }
        else {
          SILVA_EXPECT(false, MINOR, "fragmentization doesn't allow {}", ccd[i].to_wrap());
        }
      }
      SILVA_EXPECT(parentheses.empty(),
                   MINOR,
                   "Unmatched parenthesis at {}",
                   parentheses.back().location);
      SILVA_EXPECT_FWD(recognize_indent(n, 0));
      return {};
    }

    expected_t<void> recognize_identifier()
    {
      SILVA_EXPECT(ccd[i].category == XID_Start, ASSERT);
      emit(i, IDENTIFIER);
      while (i < n && (ccd[i].category == XID_Start || ccd[i].category == XID_Continue)) {
        ++i;
      }
      return {};
    }

    expected_t<void> recognize_number()
    {
      SILVA_EXPECT(is_ascii_digit(ccd[i].codepoint), ASSERT);
      emit(i, NUMBER);
      const auto is_number_cont = [](const unicode::codepoint_t cp) -> bool {
        if (is_ascii_digit(cp)) {
          return true;
        }
        if (cp == U'.' || cp == U'\'' || cp == '+' || cp == '-') {
          return true;
        }
        if (is_ascii_alpha(cp)) {
          return true;
        }
        return false;
      };
      while (i < n && is_number_cont(ccd[i].codepoint)) {
        ++i;
      }
      return {};
    }

    expected_t<bool> try_recognize_string()
    {
      if (i < n && (ccd[i].codepoint == U'"' || ccd[i].codepoint == U'\'')) {
        emit(i, STRING);
        const unicode::codepoint_t delim = ccd[i].codepoint;
        i++;
        while (i < n) {
          if (ccd[i].codepoint == U'\\') {
            SILVA_EXPECT(i + 1 < n, MINOR, "expected character after '\\' at {}", ccd[i].location);
            i += 2;
          }
          else if (ccd[i].codepoint == delim) {
            i += 1;
            break;
          }
          else {
            i += 1;
          }
        }
        return true;
      }
      else if (i + 1 < n && (ccd[i].codepoint == U'\\' && ccd[i + 1].codepoint == U'\\')) {
        emit(i, STRING);
        while (i < n && ccd[i].category != Newline) {
          ++i;
        }
        return true;
      }
      return false;
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
      if (i == n || !parentheses.empty()) {
        if (prev_i < i) {
          emit(prev_i, WHITESPACE);
        }
      }
      else {
        if (prev_i < i - last_indent) {
          emit(prev_i, WHITESPACE);
          prev_i = i - last_indent;
        }
        const bool emitted_indent = SILVA_EXPECT_FWD(recognize_indent(prev_i, last_indent));
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

  void pretty_write_impl(const fragmentization_t::fragment_t& ff, byte_sink_t* stream)
  {
    stream->format("{} {}", silva::pretty_string(ff.category), silva::pretty_string(ff.location));
  }

  void pretty_write_impl(const fragmentization_t& self, byte_sink_t* stream)
  {
    const index_t n = self.fragments.size();
    for (index_t idx = 0; idx < n; ++idx) {
      const auto& [cat, loc]    = self.fragments[idx];
      const index_t start_index = loc.byte_offset;
      const index_t end_index =
          (idx + 1 < n) ? self.fragments[idx + 1].location.byte_offset : self.source_code.size();
      const string_view_t sv =
          string_view_t{self.source_code}.substr(start_index, end_index - start_index);
      const fragment_category_t fc = cat;
      stream->format("{:8} {:11}", silva::pretty_string(loc), silva::pretty_string(fc));
      if (std::ranges::all_of(sv, [](const char c) { return c != ' ' && c != '\n'; })) {
        stream->format(" {:20}", sv);
      }
      else {
        stream->format(" {:20}", "");
      }
      stream->format("[{}]", hexdump(sv));
      stream->write_str("\n");
    }
  }
}
