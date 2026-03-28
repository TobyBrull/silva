#include "syntax.hpp"

#include "seed.hpp"
#include "seed_tokenizer.hpp"

#include "zoo/fern/fern.hpp"

namespace silva {
  parser_t as_parser(const seed::interpreter_t* si)
  {
    return [si](const tokenization_ptr_t tp, const name_id_t ni) {
      return si->apply(tp, ni);
    };
  }

  expected_t<name_id_t> infer_goal_rule_name(syntax_farm_t& sf, const filepath_t& fsp)
  {
    const string_t ext = fsp.extension().string();
    SILVA_EXPECT(ext.size() >= 2 && ext.front() == '.',
                 MAJOR,
                 "Filename {} did not have an extension",
                 fsp.string());
    string_t lang = ext.substr(1);
    lang[0]       = std::toupper(lang[0]);
    return sf.name_id_of(lang);
  }

  unique_ptr_t<seed::interpreter_t> standard_seed_interpreter(syntax_farm_ptr_t sfp)
  {
    array_t<tuple_t<filepath_t, string_t>> sources = {
        {"seed.seed", string_t{seed::seed_str}},
        {"axe.seed", string_t{seed::axe_str}},
        {"tokenizer.seed", string_t{seed::seed_tokenizer_str}},
        {"tokenizers.seed", string_t{seed::bootstrap_tokenizers_str}},
        {"fern.seed", string_t{fern::seed_str}},
        {"silva.seed", string_t{seed_str}},
    };

    auto retval = std::make_unique<seed::interpreter_t>(sfp);
    for (const auto& [fp, txt]: sources) {
      SILVA_EXPECT_ASSERT(retval->add_seed_text(fp, txt));
    }

    using callback_t = seed::interpreter_t::callback_t;
    auto callback    = callback_t::make<&seed::interpreter_t::add_seed_copy>(retval.get());
    retval->parse_callbacks[sfp->name_id_of("Seed")] = callback;
    return retval;
  }
}
