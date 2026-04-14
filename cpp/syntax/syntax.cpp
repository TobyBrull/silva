#include "syntax.hpp"

#include "seed.hpp"
#include "seed_tokenizer.hpp"

#include "zoo/fern/fern.hpp"

namespace silva {
  parser_t as_parser(seed::interpreter_t* si)
  {
    return [si](fragment_span_t fs, const name_id_t ni) {
      return si->apply(std::move(fs), ni);
    };
  }

  unique_ptr_t<seed::interpreter_t> standard_seed_interpreter(syntax_farm_ptr_t sfp)
  {
    array_t<tuple_t<filepath_t, string_t>> sources = {
        {"tokenizers.seed", string_t{seed::bootstrap_tokenizers_str}},
        {"seed.seed", string_t{seed::seed_str}},
        {"axe.seed", string_t{seed::axe_str}},
        {"tokenizer.seed", string_t{seed::seed_tokenizer_str}},
        {"fern.seed", string_t{fern::seed_str}},
        {"silva.seed", string_t{seed_str}},
    };

    auto retval = std::make_unique<seed::interpreter_t>(sfp);
    for (const auto& [fp, txt]: sources) {
      SILVA_EXPECT_ASSERT(retval->add_seed_text(fp, txt));
    }

    return retval;
  }
}
