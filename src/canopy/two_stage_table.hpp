#pragma once

#include "array.hpp"
#include "expected.hpp"

#include <type_traits>

namespace silva {
  template<typename Int, typename T, index_t NumLowerBits>
  struct two_stage_table_t {
    static_assert(std::is_integral_v<Int>);
    static_assert(0 < NumLowerBits);
    constexpr inline static index_t LowerBitMask = ((1 << NumLowerBits) - 1);

    array_t<index_t> stage_1;
    array_t<index_t> stage_2;
    array_t<T> stage_3;

    auto& operator[](this auto&&, Int);

    index_t key_size() const;

    expected_t<void> validate() const;
  };
}

// IMPLEMENTATION

namespace silva {
  template<typename Int, typename T, index_t NumLowerBits>
  auto& two_stage_table_t<Int, T, NumLowerBits>::operator[](this auto&& self, const Int key)
  {
    const index_t stage_1_offset = (key >> NumLowerBits);
    const index_t stage_1_result = self.stage_1[stage_1_offset];
    const index_t stage_2_offset = (key & LowerBitMask);
    const index_t stage_2_result = self.stage_2[stage_1_result + stage_2_offset];
    return self.stage_3[stage_2_result];
  }

  template<typename Int, typename T, index_t NumLowerBits>
  index_t two_stage_table_t<Int, T, NumLowerBits>::key_size() const
  {
    const index_t retval = (index_t(stage_1.size()) << NumLowerBits);
    return retval;
  }

  template<typename Int, typename T, index_t NumLowerBits>
  expected_t<void> two_stage_table_t<Int, T, NumLowerBits>::validate() const
  {
    for (index_t hi = 0; hi < stage_1.size(); ++hi) {
      SILVA_EXPECT(0 <= stage_1[hi], MINOR);
      SILVA_EXPECT((stage_1[hi] & LowerBitMask) == 0, MINOR);
      SILVA_EXPECT(stage_1[hi] + LowerBitMask < stage_2.size(), MINOR);
    }
    for (index_t lo = 0; lo < stage_2.size(); ++lo) {
      SILVA_EXPECT(0 <= stage_2[lo] && stage_2[lo] < stage_3.size(), MINOR);
    }
    return {};
  }
}
