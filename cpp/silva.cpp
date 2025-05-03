#include "silva.hpp"

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
}
