#include "syntax.hpp"

#include "seed.hpp"

#include "gallery/fern/fern.hpp"

namespace silva {
  expected_t<name_id_t> infer_goal_rule_name(syntax_ward_t& sw, const filesystem_path_t& fsp)
  {
    const string_t ext = fsp.extension().string();
    SILVA_EXPECT(ext.size() >= 2 && ext.front() == '.',
                 MAJOR,
                 "Filename {} did not have an extension",
                 fsp.string());
    string_t lang = ext.substr(1);
    lang[0]       = std::toupper(lang[0]);
    return sw.name_id_of(lang);
  }

  unique_ptr_t<seed_engine_t> standard_seed_engine(syntax_ward_ptr_t swp)
  {
    auto retval = std::make_unique<seed_engine_t>(swp);
    SILVA_EXPECT_ASSERT(retval->add_complete_file("seed.seed", seed_seed));
    SILVA_EXPECT_ASSERT(retval->add_complete_file("seed-axe.seed", seed_axe_seed));
    SILVA_EXPECT_ASSERT(retval->add_complete_file("fern.seed", fern_seed));
    SILVA_EXPECT_ASSERT(retval->add_complete_file("silva.seed", silva_seed));
    retval->parse_callbacks[swp->name_id_of("Seed")] =
        seed_engine_t::callback_t::make<&seed_engine_t::add_copy>(retval.get());
    return retval;
  }
}
