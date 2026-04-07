#pragma once

#include "syntax/syntax_farm.hpp"

namespace silva::fern {
  struct lexicon_t : public silva::lexicon_t {
   private:
    lexicon_t(syntax_farm_ptr_t sfp) : silva::lexicon_t(sfp)
    {
      language_name = sfp->token_id("Fern");
    }
    friend struct silva::syntax_farm_t;

   public:
    const token_id_t ti_none  = sfp->token_id("none");
    const token_id_t ti_true  = sfp->token_id("true");
    const token_id_t ti_false = sfp->token_id("false");

    const token_id_t ti_number = sfp->token_id("number");
    const token_id_t ti_string = sfp->token_id("string");

    const name_id_t ni_fern     = sfp->name_id_of("Fern");
    const name_id_t ni_lbl_item = sfp->name_id_of(ni_fern, "LabeledItem");
    const name_id_t ni_label    = sfp->name_id_of(ni_fern, "Label");
    const name_id_t ni_value    = sfp->name_id_of(ni_fern, "Value");
  };
  using lexicon_ptr_t = ptr_t<const lexicon_t>;
}
