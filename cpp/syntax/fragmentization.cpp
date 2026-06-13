#include "fragmentization.hpp"

#include "canopy/expected.hpp"
#include "canopy/filesystem.hpp"
#include "canopy/pretty_write.hpp"
#include "canopy/unicode.hpp"

namespace silva {
  using enum codepoint_category_t;
  using enum fragment_category_t;

  token_id_t fragment_category_to_token_id(syntax_farm_t& sf, const fragment_category_t fc)
  {
    static const auto vals = enum_hashmap_to_string<fragment_category_t>();
    return sf.token_id(vals.at(fc));
  }

  constexpr static array_fixed_t<unicode::codepoint_t, 1> xid_additional_internal = {U'-'};

  constexpr bool is_ascii_digit(const unicode::codepoint_t cp)
  {
    return '0' <= cp && cp <= '9';
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

    expected_t<void>
    init(filepath_t filepath, string_t source_code, array_t<categorized_codepoint_data_t> orig_ccd)
    {
      retval->filepath    = std::move(filepath);
      retval->source_code = std::move(source_code);
      ccd                 = std::move(orig_ccd);
      SILVA_EXPECT(ccd.size() >= 1, ASSERT);
      n = ccd.size();
      return {};
    }

    array_t<categorized_codepoint_data_t> ccd;
    index_t i = 0;
    index_t n = 0;

    expected_t<void> emit(const index_t idx, fragment_category_t fc)
    {
      if (fc == NEWLINE) {
        if (!languages.back().saw_nontrivial_since_last_newline) {
          fc = WHITESPACE;
        }
        languages.back().saw_nontrivial_since_last_newline = false;
      }
      else if (is_fragment_category_real(fc)) {
        if (!languages.empty()) {
          languages.back().saw_nontrivial_since_last_newline = true;
        }
      }
      retval->fragments.push_back(fragment_t{
          .category = fc,
          .location = ccd[idx].location,
      });
      return {};
    }

    struct language_data_t {
      bool uses_angle_quotes                 = false;
      index_t multiline_lang_depth           = 0;
      bool saw_nontrivial_since_last_newline = false;

      array_t<categorized_codepoint_data_t> parentheses;
      array_t<index_t> indents = {0};
    };

    array_t<language_data_t> languages;

    expected_t<bool> recognize_indent(const index_t idx, const index_t indent)
    {
      SILVA_EXPECT(indent >= 0, ASSERT);
      SILVA_EXPECT(idx >= indent, ASSERT);
      const index_t start_idx = idx - indent;
      auto& indents           = languages.back().indents;
      SILVA_EXPECT(!indents.empty(), ASSERT);
      if (indents.back() < indent) {
        SILVA_EXPECT_FWD(emit(start_idx, INDENT));
        indents.push_back(indent);
        return true;
      }
      else if (indents.back() > indent) {
        while (indents.back() > indent) {
          SILVA_EXPECT_FWD(emit(start_idx, DEDENT));
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

    struct start_of_line_info_t {
      index_t multiline_lang_depth = 0;
      index_t indent               = 0;
      index_t new_i_linefeed       = 0;
      index_t new_i                = 0;
      bool is_empty                = false;
      bool is_multiline_str        = false;

      friend auto operator<=>(const start_of_line_info_t&, const start_of_line_info_t&) = default;
    };
    expected_t<start_of_line_info_t>
    find_start_of_line_info(const index_t break_multiline_lang_depth) const
    {
      start_of_line_info_t retval;
      index_t loc_i    = i;
      index_t loc_i_lf = i;
      retval.is_empty  = true;
      while (loc_i < n) {
        if (ccd[loc_i].codepoint == U' ') {
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
          loc_i_lf = loc_i;
        }
        else if (ccd[loc_i].codepoint == U'¶') {
          retval.is_multiline_str = true;
          retval.is_empty         = false;
          break;
        }
        else if (ccd[loc_i].codepoint == U'\n' || ccd[loc_i].codepoint == U'#' ||
                 ccd[loc_i].codepoint == U'»') {
          break;
        }
        else {
          retval.is_empty = false;
          break;
        }
      }
      retval.new_i          = loc_i;
      retval.new_i_linefeed = loc_i_lf;
      return retval;
    }

    void skip_to_end_of_line()
    {
      while (i < n && ccd[i].codepoint != U'\n') {
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
        SILVA_EXPECT(ccd[i].codepoint == U'\n', ASSERT);
        ++i;
        const start_of_line_info_t ns =
            SILVA_EXPECT_FWD(find_start_of_line_info(languages.back().multiline_lang_depth));
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
            constexpr static array_fixed_t<unicode::codepoint_t, 12> escape_seqs = {
                U'a',
                U'b',
                U'e',
                U'f',
                U'n',
                U'r',
                U't',
                U'v',
                U'\\',
                U'\'',
                U'"',
                U'?',
            };
            SILVA_EXPECT(is_one_of<12>(ccd[i + 1].codepoint, escape_seqs),
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
        while (i < n && ccd[i].codepoint != U'\n') {
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

    expected_t<start_of_line_info_t> run_language(const bool uses_angle_quotes,
                                                  const index_t multiline_lang_depth_inc)
    {
      const index_t prev_multiline_lang_depth =
          languages.empty() ? 0 : languages.back().multiline_lang_depth;
      languages.push_back(language_data_t{
          .uses_angle_quotes    = uses_angle_quotes,
          .multiline_lang_depth = prev_multiline_lang_depth + multiline_lang_depth_inc,
      });

      {
        start_of_line_info_t first_ns = SILVA_EXPECT_FWD(find_start_of_line_info(0));
        if (!first_ns.is_empty) {
          SILVA_EXPECT_FWD(recognize_indent(first_ns.new_i, first_ns.indent));
        }
        i = first_ns.new_i;
      }

      bool did_just_emit_newline     = false;
      bool did_just_recognize_indent = false;
      start_of_line_info_t ns;
      while (i < n) {
        did_just_emit_newline     = false;
        did_just_recognize_indent = false;
        if (ccd[i].codepoint == U'\n') {
          const index_t newline_i = i;
          i += 1;
          ns = SILVA_EXPECT_FWD(find_start_of_line_info(languages.back().multiline_lang_depth));
          if (!languages.back().parentheses.empty()) {
            SILVA_EXPECT(languages.back().multiline_lang_depth == ns.multiline_lang_depth,
                         MINOR,
                         "Expected multi-line language to continue at {} due to parenthesis at {}",
                         ccd[i].location,
                         languages.back().parentheses.back().location);
            SILVA_EXPECT_FWD(emit(newline_i, LINEFEED));
            i = ns.new_i_linefeed;
            continue;
          }
          SILVA_EXPECT_FWD(emit(newline_i, NEWLINE));
          did_just_emit_newline = true;
          i                     = ns.new_i;
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
            did_just_recognize_indent = true;
          }
          ns = {};
        }
        else if (ccd[i].codepoint == U' ') {
          while (i < n && ccd[i].codepoint == U' ') {
            SILVA_EXPECT_FWD(emit(i, SPACE));
            ++i;
          }
        }
        else if (ccd[i].category == ParenthesisLeft) {
          if (ccd[i].codepoint == U'«') {
            const index_t opening_i = i;
            SILVA_EXPECT_FWD(emit(i++, LANG_BEGIN));
            const start_of_line_info_t up_ns = SILVA_EXPECT_FWD(run_language(true, 0));
            SILVA_EXPECT(i < n && ccd[i].codepoint == U'»',
                         MINOR,
                         "opening '«' at {} had no matching '»', expected at {}",
                         ccd[opening_i].location,
                         ccd[i].location);
            SILVA_EXPECT(up_ns == start_of_line_info_t{}, MINOR);
            SILVA_EXPECT_FWD(emit(i++, LANG_END));
          }
          else {
            languages.back().parentheses.push_back(ccd[i]);
            SILVA_EXPECT_FWD(emit(i++, PARENTHESIS));
          }
        }
        else if (ccd[i].category == ParenthesisRight) {
          if (ccd[i].codepoint == U'»') {
            SILVA_EXPECT(languages.back().uses_angle_quotes,
                         MINOR,
                         "unexpected '»' at {}",
                         ccd[i].location);
            break;
          }
          else {
            const auto expected_open_paren_it = opposite_parenthesis.find(ccd[i].codepoint);
            SILVA_EXPECT(expected_open_paren_it != opposite_parenthesis.end(), ASSERT);
            const unicode::codepoint_t expected_open_paren = expected_open_paren_it->second;
            auto& parentheses                              = languages.back().parentheses;
            SILVA_EXPECT(!parentheses.empty(),
                         MINOR,
                         "closing parenthesis without matching opening parenthesis at {}",
                         ccd[i].location);
            SILVA_EXPECT(parentheses.back().codepoint == expected_open_paren,
                         MINOR,
                         "mismatching parentheses between {} and {}",
                         parentheses.back().location,
                         ccd[i].location);
            parentheses.pop_back();
            SILVA_EXPECT_FWD(emit(i++, PARENTHESIS));
          }
        }
        else if (ccd[i].category == Operator) {
          if (ccd[i].codepoint == U'⎢') {
            SILVA_EXPECT_FWD(emit(i++, LANG_BEGIN));
            const start_of_line_info_t up_ns = SILVA_EXPECT_FWD(run_language(false, 1));
            const index_t final_i            = std::min(i, n - 1);
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
          if (ccd[i].codepoint == U'\\' && i + 1 < n && ccd[i + 1].codepoint == U'\n') {
            SILVA_EXPECT_FWD(emit(i, WHITESPACE));
            i += 2;
            continue;
          }
          if (!SILVA_EXPECT_FWD(try_recognize_string())) {
            SILVA_EXPECT_FWD(emit(i++, OPERATOR));
          }
        }
        else if (is_ascii_digit(ccd[i].codepoint)) {
          SILVA_EXPECT_FWD(emit(i++, DIGIT));
        }
        else if (ccd[i].category == XID_Lowercase) {
          SILVA_EXPECT_FWD(emit(i++, ID_LOWER));
        }
        else if (ccd[i].category == XID_Uppercase) {
          SILVA_EXPECT_FWD(emit(i++, ID_UPPER));
        }
        else if (ccd[i].category == XID_Start) {
          SILVA_EXPECT_FWD(emit(i++, ID_START__NOT_ID_LOWER_AND_NOT_ID_UPPER));
        }
        else if (ccd[i].category == XID_Continue) {
          SILVA_EXPECT_FWD(emit(i++, ID_CONTINUE__NOT_ID_START_AND_NOT_DIGIT));
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
      if (!did_just_emit_newline) {
        SILVA_EXPECT_FWD(emit(final_i, NEWLINE));
      }
      if (!did_just_recognize_indent) {
        SILVA_EXPECT_FWD(recognize_indent(final_i, 0));
      }

      SILVA_EXPECT(!languages.empty(), ASSERT);
      languages.pop_back();

      return ns;
    }

    expected_t<void> run()
    {
      SILVA_EXPECT_FWD(emit(0, LANG_BEGIN));
      const start_of_line_info_t up_ns = SILVA_EXPECT_FWD(run_language(false, 0));
      SILVA_EXPECT(languages.empty(), MINOR);
      SILVA_EXPECT(up_ns == start_of_line_info_t{}, MINOR);
      SILVA_EXPECT_FWD(emit(n - 1, LANG_END));
      return {};
    }
  };

  fragment_span_t::fragment_span_t(fragmentization_ptr_t fp) : fp(fp), end(fp->fragments.size()) {}

  fragment_span_t::fragment_span_t(fragmentization_ptr_t fp, const index_t begin, const index_t end)
    : fp(fp), begin(begin), end(end)
  {
  }

  fragment_span_t::operator span_t<const fragment_t>()
  {
    return span_t<const fragment_t>(fp->fragments.data() + begin, end - begin);
  }

  string_t escape_string(string_t retval)
  {
    for (index_t i = 0; i < retval.size(); ++i) {
      if (retval[i] == '\n') {
        retval.replace(i, 1, "\\n");
      }
    }
    return retval;
  }

  expected_t<unique_ptr_t<fragmentization_t>> fragmentize_unique(filepath_t filepath,
                                                                 string_t source_code)
  {
    auto ccd = SILVA_EXPECT_FWD(categorize_codepoints(source_code));
    fragmentizer_t ff;
    SILVA_EXPECT_FWD(ff.init(filepath, std::move(source_code), std::move(ccd)));
    SILVA_EXPECT_FWD(ff.run(), "while fragmentizing [{}]", filepath);
    SILVA_EXPECT(ff.i == ff.n,
                 MINOR,
                 "Incomplete fragmentization; stopped at {}",
                 ff.ccd[ff.i].location);
    return std::move(ff.retval);
  }

  expected_t<fragmentization_ptr_t>
  fragmentize(syntax_farm_ptr_t sfp, filepath_t filepath, string_t source_code)
  {
    auto retval = SILVA_EXPECT_FWD(fragmentize_unique(std::move(filepath), std::move(source_code)));
    retval->sfp = sfp;
    return sfp->add(std::move(retval));
  }

  expected_t<fragmentization_ptr_t> fragmentize_load(syntax_farm_ptr_t sfp, filepath_t filepath)
  {
    string_t source_code     = SILVA_EXPECT_FWD(read_file(filepath));
    fragmentization_ptr_t fp = SILVA_EXPECT_FWD_PLAIN(
        fragmentize(std::move(sfp), std::move(filepath), std::move(source_code)));
    return fp;
  }

  expected_t<fragmented_token_t> fragmented_token(syntax_farm_ptr_t sfp, string_view_t sv)
  {
    const token_id_t ti = sfp->token_id(sv);
    array_t<fragmented_token_t::item_t> items;
    for (auto maybe_ud: unicode::utf8_decode_generator(sv)) {
      const unicode::codepoint_data_t ud = SILVA_EXPECT_FWD(std::move(maybe_ud));
      const codepoint_category_t cc      = codepoint_category_table[ud.codepoint];
      fragment_category_t fc             = INVALID;
      if (is_ascii_digit(ud.codepoint)) {
        fc = DIGIT;
      }
      else if (cc == Operator) {
        fc = OPERATOR;
      }
      else if (cc == ParenthesisLeft || cc == ParenthesisRight) {
        fc = PARENTHESIS;
      }
      else if (cc == XID_Lowercase) {
        fc = ID_LOWER;
      }
      else if (cc == XID_Uppercase) {
        fc = ID_UPPER;
      }
      else if (cc == XID_Start) {
        fc = ID_START__NOT_ID_LOWER_AND_NOT_ID_UPPER;
      }
      else if (cc == XID_Continue) {
        fc = ID_CONTINUE__NOT_ID_START_AND_NOT_DIGIT;
      }
      else if (cc == Space) {
        fc = SPACE;
      }
      else {
        SILVA_EXPECT(false, MINOR, "fragmented_token: unsupported codepoint in [{}]", sv);
      }
      items.push_back(fragmented_token_t::item_t{.category = fc, .codepoint = ud.codepoint});
    }

    return fragmented_token_t{
        .token_id = ti,
        .items    = std::move(items),
    };
  }

  void pretty_write_impl(const fragment_t& ff, byte_sink_t* stream)
  {
    stream->format("{} {}", silva::pretty_string(ff.category), silva::pretty_string(ff.location));
  }

  index_t fragmentization_t::size() const
  {
    return fragments.size();
  }

  file_location_t fragmentization_t::location_at(const index_t idx) const
  {
    if (idx < size()) {
      return fragments[idx].location;
    }
    return file_location_eof;
  }

  string_view_t fragmentization_t::get_fragment_text(const index_t frag_idx) const
  {
    const index_t start = fragments[frag_idx].location.byte_offset;
    const index_t end   = (frag_idx + 1 < fragments.size())
        ? fragments[frag_idx + 1].location.byte_offset
        : source_code.size();
    return string_view_t{source_code}.substr(start, end - start);
  }

  index_t fragmentization_t::get_fragment_byte_offset(const index_t frag_idx) const
  {
    if (frag_idx < size()) {
      return fragments[frag_idx].location.byte_offset;
    }
    else {
      return source_code.size();
    }
  }

  expected_t<unicode::codepoint_t>
  fragmentization_t::get_unique_codepoint(const index_t frag_idx) const
  {
    const auto frag = fragments[frag_idx];
    SILVA_EXPECT(is_fragment_category_simple(frag.category), MINOR);
    const index_t bo         = frag.location.byte_offset;
    const string_view_t subs = string_view_t{source_code}.substr(bo);
    const auto [cp, new_idx] = SILVA_EXPECT_FWD(unicode::utf8_decode_one(subs));
    SILVA_EXPECT(get_fragment_byte_offset(frag_idx + 1) == bo + new_idx, MAJOR);
    return cp;
  }

  expected_t<index_t> fragmentization_t::advance_language(const index_t start) const
  {
    SILVA_EXPECT(fragments[start].category == LANG_BEGIN, MAJOR);
    const index_t n = fragments.size();
    index_t depth   = 1;
    index_t idx     = start + 1;
    while (idx < n && depth > 0) {
      if (fragments[idx].category == LANG_BEGIN) {
        depth++;
      }
      else if (fragments[idx].category == LANG_END) {
        depth--;
      }
      idx++;
    }
    SILVA_EXPECT(depth == 0, MINOR, "non matching LANG_BEGIN/LANG_END fragments");
    return idx;
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

  void pretty_write_impl(const fragment_location_t& self, byte_sink_t* stream)
  {
    if (self.fp.is_nullptr()) {
      stream->write_str("unknown token-location");
      return;
    }
    stream->format("{}:", self.fp->filepath.filename().string());
    silva::pretty_write(self.fp->location_at(self.fragment_index), stream);
  }
}
