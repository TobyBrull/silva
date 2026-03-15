#include "seed_tokenizer.hpp"

namespace silva::seed {

  expected_t<tokenization_ptr_t> tokenizer_t::apply(syntax_ward_ptr_t,
                                                    const fragmentization_t&) const
  {
    return {};
  }

  expected_t<tokenizer_t>
  tokenizer_create(syntax_ward_ptr_t, name_id_t tokenizer_name, parse_tree_span_t)
  {
    return {};
  }
}
