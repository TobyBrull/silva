#pragma once

#include "canopy/expected.hpp"
#include "canopy/file_location.hpp"

#include "fragmentization_data.hpp"

namespace silva {
  struct fragmentization_t : public menhir_t {
    filesystem_path_t filepath;
    string_t source_code;
    array_t<file_location_t> fragment_locations;

    // The first offset will always be zero. The last fragment goes from "back()" to the end of the
    // "source_code"
    array_t<index_t> fragment_start_indexes;
    array_t<fragment_category_t> categories;

    friend void pretty_write_impl(const fragmentization_t&, byte_sink_t*);
  };

  expected_t<unique_ptr_t<fragmentization_t>> fragmentize_load(filesystem_path_t);
  expected_t<unique_ptr_t<fragmentization_t>> fragmentize(filesystem_path_t descriptive_path,
                                                          string_t source_code);
}
