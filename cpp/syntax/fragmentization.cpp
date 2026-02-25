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

  template<index_t N>
  constexpr bool is_one_of(const unicode::codepoint_t cp,
                           const array_fixed_t<unicode::codepoint_t, N> cps)
  {
    const auto it = std::ranges::find(cps, cp);
    return (it != cps.end());
  }

  struct categorized_codepoint_data_t : public unicode::codepoint_data_t {
    codepoint_category_t category = codepoint_category_t::Forbidden;
    file_location_t location;

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

    codepoint_wrap_t to_wrap() const
    {
      return codepoint_wrap_t{
          .codepoint = codepoint,
          .category  = category,
          .location  = location,
      };
    }
  };

  expected_t<array_t<categorized_codepoint_data_t>>
  categorize_codepoints(const string_t& source_code)
  {
    array_t<categorized_codepoint_data_t> retval;
    file_location_t loc;
    for (auto maybe_ud: unicode::utf8_decode_generator(source_code)) {
      const unicode::codepoint_data_t ud = SILVA_EXPECT_FWD(std::move(maybe_ud));
      const codepoint_category_t cc      = codepoint_category_table[ud.codepoint];
      categorized_codepoint_data_t cc2{ud, cc, loc};
      SILVA_EXPECT(cc != Forbidden, MINOR, "Forbidden {}", cc2.to_wrap());
      retval.emplace_back(std::move(cc2));
      if (ud.codepoint == U'\n') {
        loc.line_num += 1;
        loc.column = 0;
      }
      else {
        loc.column += 1;
      }
      loc.byte_offset += ud.len;
    }
    SILVA_EXPECT(!retval.empty() && retval.back().codepoint == U'\n',
                 MINOR,
                 "source-code expected to end with newline");
    return {std::move(retval)};
  }

  struct fragmentizer_t {
    unique_ptr_t<fragmentization_t> retval = std::make_unique<fragmentization_t>();

    expected_t<void> init(filesystem_path_t descriptive_path,
                          string_t source_code,
                          array_t<categorized_codepoint_data_t> orig_ccd)
    {
      retval->filepath    = std::move(descriptive_path);
      retval->source_code = std::move(source_code);
      ccd                 = std::move(orig_ccd);
      SILVA_EXPECT(ccd.size() >= 1, ASSERT);
      n = ccd.size();
      return {};
    }

    array_t<categorized_codepoint_data_t> ccd;
    index_t i = 0;
    index_t n = 0;

    index_t last_real_fragment = -1;
    expected_t<void> emit(const index_t idx, fragment_category_t fc)
    {
      if (fc == NEWLINE) {
        SILVA_EXPECT(last_real_fragment >= 0, ASSERT);
        const auto last_real_cat = retval->fragments[last_real_fragment].category;
        if (last_real_cat == LANG_BEGIN || last_real_cat == NEWLINE) {
          fc = WHITESPACE;
        }
      }
      if (fc == WHITESPACE) {
        SILVA_EXPECT(!retval->fragments.empty(), ASSERT);
        const fragment_category_t last_cat = retval->fragments.back().category;
        if (last_cat == WHITESPACE || last_cat == NEWLINE) {
          return {};
        }
      }
      if (is_fragment_category_real(fc)) {
        last_real_fragment = retval->fragments.size();
      }
      retval->fragments.push_back(fragment_t{
          .category = fc,
          .location = ccd[idx].location,
      });
      return {};
    }

    struct language_data_t {
      bool uses_angle_quotes       = false;
      index_t multiline_lang_depth = 0;

      array_t<categorized_codepoint_data_t> parentheses;
      array_t<index_t> indents = {0};
    };

    array_t<language_data_t> languages;

    expected_t<void> run()
    {
      SILVA_EXPECT_FWD(emit(0, LANG_BEGIN));
      const newline_state_t up_ns = SILVA_EXPECT_FWD(run_language(false, 0));
      SILVA_EXPECT(languages.empty(), MINOR);
      SILVA_EXPECT(up_ns == newline_state_t{}, MINOR);
      SILVA_EXPECT_FWD(emit(n - 1, LANG_END));
      return {};
    }

    expected_t<bool> recognize_indent(const index_t idx, const index_t indent)
    {
      SILVA_EXPECT(indent >= 0, ASSERT);
      auto& indents = languages.back().indents;
      SILVA_EXPECT(!indents.empty(), ASSERT);
      if (indents.back() < indent) {
        SILVA_EXPECT_FWD(emit(idx, INDENT));
        indents.push_back(indent);
        return true;
      }
      else if (indents.back() > indent) {
        while (indents.back() > indent) {
          SILVA_EXPECT_FWD(emit(idx, DEDENT));
          indents.pop_back();
        }
        SILVA_EXPECT(!indents.empty() && indents.back() == indent,
                     MINOR,
                     "inconsistent indent: indent at {} doesn't match any previous indent",
                     ccd[idx].location);
        return true;
      }
      return false;
    }

    struct newline_state_t {
      index_t multiline_lang_depth = 0;
      index_t indent               = 0;
      index_t new_i                = 0;
      bool is_empty                = false;
      bool is_multiline_str        = false;

      friend auto operator<=>(const newline_state_t&, const newline_state_t&) = default;
    };
    expected_t<newline_state_t> find_newline_state(const index_t break_multiline_lang_depth)
    {
      newline_state_t retval;
      index_t loc_i   = i;
      retval.is_empty = true;
      while (loc_i < n) {
        if (ccd[loc_i].category == Space) {
          retval.indent += 1;
          ++loc_i;
        }
        else if (ccd[loc_i].codepoint == U'⎢') {
          if (retval.multiline_lang_depth == break_multiline_lang_depth) {
            retval.is_empty = false;
            break;
          }
          retval.indent = 0;
          retval.multiline_lang_depth += 1;
          ++loc_i;
        }
        else if (ccd[loc_i].codepoint == U'¶') {
          retval.is_multiline_str = true;
          retval.is_empty         = false;
          break;
        }
        else if (ccd[loc_i].category == Newline || ccd[loc_i].codepoint == U'#' ||
                 ccd[loc_i].codepoint == U'»') {
          break;
        }
        else {
          retval.is_empty = false;
          break;
        }
      }
      retval.new_i = loc_i;
      return retval;
    }

    expected_t<newline_state_t> run_language(const bool uses_angle_quotes,
                                             const index_t multiline_lang_depth_inc)
    {
      const index_t prev_multiline_lang_depth =
          languages.empty() ? 0 : languages.back().multiline_lang_depth;
      languages.push_back(language_data_t{
          .uses_angle_quotes    = uses_angle_quotes,
          .multiline_lang_depth = prev_multiline_lang_depth + multiline_lang_depth_inc,
      });

      {
        newline_state_t first_ns = SILVA_EXPECT_FWD(find_newline_state(0));
        if (!first_ns.is_empty) {
          SILVA_EXPECT_FWD(recognize_indent(first_ns.new_i, first_ns.indent));
        }
        i = first_ns.new_i;
      }

      newline_state_t ns;
      while (i < n) {
        if (ccd[i].category == Newline) {
          const index_t newline_i = i;
          i += 1;
          ns = SILVA_EXPECT_FWD(find_newline_state(languages.back().multiline_lang_depth));
          if (!languages.back().parentheses.empty()) {
            SILVA_EXPECT(languages.back().multiline_lang_depth == ns.multiline_lang_depth,
                         MINOR,
                         "Expected multi-line language to continue at {} due to parenthesis at {}",
                         ccd[i].location,
                         languages.back().parentheses.back().location);
            SILVA_EXPECT_FWD(emit(newline_i, WHITESPACE));
            i = ns.new_i;
            continue;
          }
          SILVA_EXPECT_FWD(emit(newline_i, NEWLINE));
          if (i < ns.new_i) {
            SILVA_EXPECT_FWD(emit(i, WHITESPACE));
          }
          i = ns.new_i;
          if (ns.multiline_lang_depth < languages.back().multiline_lang_depth) {
            SILVA_EXPECT(languages.back().uses_angle_quotes == false,
                         MINOR,
                         "LANGUAGE started by '«' must be finished by '»' before outer multi-line "
                         "language may be finished at {}",
                         ccd[newline_i].location);
            break;
          }
          if (!ns.is_empty) {
            SILVA_EXPECT_FWD(recognize_indent(ns.new_i, ns.indent));
          }
          ns = {};
        }
        else if (ccd[i].category == Space) {
          SILVA_EXPECT_FWD(emit(i, WHITESPACE));
          while (i < n && ccd[i].category == Space) {
            ++i;
          }
        }
        else if (ccd[i].category == ParenthesisLeft) {
          if (ccd[i].codepoint == U'«') {
            const index_t opening_i = i;
            SILVA_EXPECT_FWD(emit(i++, LANG_BEGIN));
            const newline_state_t up_ns = SILVA_EXPECT_FWD(run_language(true, 0));
            SILVA_EXPECT(i < n && ccd[i].codepoint == U'»',
                         MINOR,
                         "opening '«' at {} had no matching '»', expected at {}",
                         ccd[opening_i].location,
                         ccd[i].location);
            SILVA_EXPECT(up_ns == newline_state_t{}, MINOR);
            SILVA_EXPECT_FWD(emit(i++, LANG_END));
          }
          else {
            languages.back().parentheses.push_back(ccd[i]);
            SILVA_EXPECT_FWD(emit(i++, PAREN_LEFT));
          }
        }
        else if (ccd[i].category == ParenthesisRight) {
          if (ccd[i].codepoint == U'»') {
            SILVA_EXPECT(languages.back().uses_angle_quotes,
                         MINOR,
                         "Unexpected '»' at {}",
                         ccd[i].location);
            break;
          }
          else {
            const auto expected_open_paren_it = opposite_parenthesis.find(ccd[i].codepoint);
            SILVA_EXPECT(expected_open_paren_it != opposite_parenthesis.end(), ASSERT);
            const unicode::codepoint_t expected_open_paren = expected_open_paren_it->second;
            auto& parentheses                              = languages.back().parentheses;
            SILVA_EXPECT(!parentheses.empty() &&
                             parentheses.back().codepoint == expected_open_paren,
                         MINOR,
                         "Mismatching parentheses between {} and {}",
                         parentheses.back().location,
                         ccd[i].location);
            parentheses.pop_back();
            SILVA_EXPECT_FWD(emit(i++, PAREN_RIGHT));
          }
        }
        else if (ccd[i].category == Operator) {
          if (ccd[i].codepoint == U'⎢') {
            SILVA_EXPECT_FWD(emit(i++, LANG_BEGIN));
            const newline_state_t up_ns = SILVA_EXPECT_FWD(run_language(false, 1));
            const index_t final_i       = std::min(i, n - 1);
            SILVA_EXPECT_FWD(emit(final_i, LANG_END));
            SILVA_EXPECT_FWD(emit(final_i, NEWLINE));
            if (up_ns.multiline_lang_depth < languages.back().multiline_lang_depth) {
              SILVA_EXPECT(
                  languages.back().uses_angle_quotes == false,
                  MINOR,
                  "LANGUAGE started by '«' must be finished by '»' before outer multi-line "
                  "language may be finished at {}",
                  ccd[final_i].location);
              break;
            }
            continue;
          }
          if (ccd[i].codepoint == U'#') {
            SILVA_EXPECT_FWD(recognize_comment());
            continue;
          }
          if (ccd[i].codepoint == U'¶') {
            SILVA_EXPECT_FWD(recognize_multiline_string());
            continue;
          }
          if (ccd[i].codepoint == U'\\' && i + 1 < n && ccd[i + 1].category == Newline) {
            if (retval->fragments.empty() || retval->fragments.back().category != WHITESPACE) {
              SILVA_EXPECT_FWD(emit(i, WHITESPACE));
            }
            i += 2;
            continue;
          }
          if (!SILVA_EXPECT_FWD(try_recognize_string())) {
            SILVA_EXPECT_FWD(emit(i++, OPERATOR));
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
      SILVA_EXPECT(languages.back().parentheses.empty(),
                   MINOR,
                   "Unmatched parenthesis at {}",
                   languages.back().parentheses.back().location);
      const index_t final_i = std::min(i, n - 1);
      SILVA_EXPECT_FWD(emit(final_i, NEWLINE));
      SILVA_EXPECT_FWD(recognize_indent(final_i, 0));

      SILVA_EXPECT(!languages.empty(), ASSERT);
      languages.pop_back();

      return ns;
    }

    expected_t<void> recognize_identifier()
    {
      SILVA_EXPECT(ccd[i].category == XID_Start, ASSERT);
      SILVA_EXPECT_FWD(emit(i, IDENTIFIER));
      while (i < n && (ccd[i].category == XID_Start || ccd[i].category == XID_Continue)) {
        ++i;
      }
      return {};
    }

    expected_t<void> recognize_number()
    {
      SILVA_EXPECT(is_ascii_digit(ccd[i].codepoint), ASSERT);
      SILVA_EXPECT_FWD(emit(i, NUMBER));
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

    void skip_to_end_of_line()
    {
      while (i < n && ccd[i].category != Newline) {
        ++i;
      }
    }

    expected_t<void> recognize_multiline_string()
    {
      SILVA_EXPECT(ccd[i].codepoint == U'¶', ASSERT);
      SILVA_EXPECT_FWD(emit(i, STRING));
      while (true) {
        skip_to_end_of_line();
        SILVA_EXPECT(i < n, ASSERT);
        SILVA_EXPECT(ccd[i].category == Newline, ASSERT);
        ++i;
        const newline_state_t ns =
            SILVA_EXPECT_FWD(find_newline_state(languages.back().multiline_lang_depth));
        if (ns.multiline_lang_depth != languages.back().multiline_lang_depth ||
            !ns.is_multiline_str) {
          --i;
          break;
        }
        i = ns.new_i;
      }
      return {};
    }

    expected_t<bool> try_recognize_string()
    {
      if (i < n && (ccd[i].codepoint == U'"' || ccd[i].codepoint == U'\'')) {
        SILVA_EXPECT_FWD(emit(i, STRING));
        const unicode::codepoint_t delim = ccd[i].codepoint;
        i++;
        while (i < n) {
          if (ccd[i].codepoint == U'\\') {
            SILVA_EXPECT(i + 1 < n,
                         MINOR,
                         "expected character after '\\' in string at {}",
                         ccd[i].location);
            const static array_fixed_t<unicode::codepoint_t, 3> escape_seqs = {U'n', U'\'', U'\"'};
            SILVA_EXPECT(is_one_of<3>(ccd[i + 1].codepoint, escape_seqs),
                         MINOR,
                         "unexpected escape sequence at {}, allowed escape sequences: {}",
                         ccd[i].location,
                         escape_seqs);
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
        SILVA_EXPECT_FWD(emit(i, STRING));
        while (i < n && ccd[i].category != Newline) {
          ++i;
        }
        return true;
      }
      return false;
    }

    bool is_comment_start(const index_t idx) { return (idx < n && ccd[idx].codepoint == U'#'); }

    // Leaves "i" on newline or at EOF.
    expected_t<bool> recognize_comment()
    {
      SILVA_EXPECT(is_comment_start(i), ASSERT);
      SILVA_EXPECT_FWD(emit(i, COMMENT));
      skip_to_end_of_line();
      return {};
    }
  };

  expected_t<unique_ptr_t<fragmentization_t>> fragmentize(filesystem_path_t descriptive_path,
                                                          string_t source_code)
  {
    auto ccd = SILVA_EXPECT_FWD(categorize_codepoints(source_code));
    fragmentizer_t ff;
    SILVA_EXPECT_FWD(ff.init(std::move(descriptive_path), std::move(source_code), std::move(ccd)));
    SILVA_EXPECT_FWD(ff.run());
    SILVA_EXPECT(ff.i == ff.n,
                 MINOR,
                 "Incomplete fragmentization; stopped at {}",
                 ff.ccd[ff.i].location);
    return std::move(ff.retval);
  }

  void pretty_write_impl(const fragment_t& ff, byte_sink_t* stream)
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
