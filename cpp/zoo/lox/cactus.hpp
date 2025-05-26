#pragma once

#include "canopy/expected.hpp"
#include "canopy/sprite.hpp"
#include "canopy/types.hpp"

namespace silva {
  template<typename Key, typename Value>
  class cactus_arm_t;

  // Alternative names: parent pointer tree, spaghetti stack, cactus stack or saguaro stack, cf.
  // https://en.wikipedia.org/wiki/Parent_pointer_tree
  template<typename Key, typename Value>
  class cactus_t : public menhir_t {
    struct new_scope_t {};
    struct arm_t {
      index_t ref_count    = 0;
      index_t parent_index = -1;
      variant_t<none_t, new_scope_t, pair_t<Key, Value>> data;
      index_t next_free = -1;
    };
    vector_t<arm_t> arms;
    index_t next_free = -1;

    index_t alloc_arm();

    void free_arm(index_t);
    friend class cactus_arm_t<Key, Value>;

   public:
    cactus_t();

    cactus_arm_t<Key, Value> root();
  };
  template<typename Key, typename Value>
  using cactus_ptr_t = ptr_t<cactus_t<Key, Value>>;

  template<typename Key, typename Value>
  class cactus_arm_t {
    cactus_ptr_t<Key, Value> cactus;
    index_t idx = -1;

    friend class cactus_t<Key, Value>;
    cactus_arm_t(cactus_ptr_t<Key, Value>, index_t);

   public:
    cactus_arm_t() = default;
    ~cactus_arm_t();

    cactus_arm_t(cactus_arm_t&&);
    cactus_arm_t(const cactus_arm_t&);
    cactus_arm_t& operator=(cactus_arm_t&&);
    cactus_arm_t& operator=(const cactus_arm_t&);

    bool is_nullptr() const;
    void clear();

    // Return unexpected if the Key is *not* already defined somewhere along this arm to the root.
    expected_t<Value*> get(const Key&) const;
    expected_t<void> assign(const Key&, Value) const;

    cactus_arm_t new_scope() const;

    // Return unexpected if the Key is *already* defined in this scope.
    expected_t<cactus_arm_t> define(const Key&, Value) const;
  };
}

// IMPLEMENTATION

namespace silva {

  // cactus_t

  template<typename Key, typename Value>
  cactus_t<Key, Value>::cactus_t()
  {
    arms.push_back(arm_t{
        .ref_count    = 1,
        .parent_index = -1,
        .data         = new_scope_t{},
        .next_free    = -1,
    });
  }

  template<typename Key, typename Value>
  index_t cactus_t<Key, Value>::alloc_arm()
  {
    index_t idx{};
    if (next_free == -1) {
      idx = arms.size();
      arms.emplace_back();
    }
    else {
      idx       = next_free;
      next_free = arms[idx].next_free;
    }
    return idx;
  }

  template<typename Key, typename Value>
  void cactus_t<Key, Value>::free_arm(const index_t idx)
  {
    SILVA_ASSERT(arms[idx].ref_count == 0);
    arms[idx].data      = none;
    arms[idx].next_free = next_free;
    next_free           = idx;
  }

  template<typename Key, typename Value>
  cactus_arm_t<Key, Value> cactus_t<Key, Value>::root()
  {
    return cactus_arm_t(ptr(), 0);
  }

  // cactus_arm_t

  template<typename Key, typename Value>
  cactus_arm_t<Key, Value>::cactus_arm_t(cactus_ptr_t<Key, Value> cp, const index_t idx)
    : cactus(std::move(cp)), idx(idx)
  {
    cactus->arms[idx].ref_count += 1;
  }

  template<typename Key, typename Value>
  cactus_arm_t<Key, Value>::~cactus_arm_t()
  {
    clear();
  }

  template<typename Key, typename Value>
  cactus_arm_t<Key, Value>::cactus_arm_t(cactus_arm_t&& other)
    : cactus(std::move(other.cactus)), idx(std::exchange(other.idx, -1))
  {
  }
  template<typename Key, typename Value>
  cactus_arm_t<Key, Value>::cactus_arm_t(const cactus_arm_t& other)
    : cactus(other.cactus), idx(other.idx)
  {
    if (!cactus.is_nullptr()) {
      cactus->object_datas[idx].ref_count += 1;
    }
  }
  template<typename Key, typename Value>
  cactus_arm_t<Key, Value>& cactus_arm_t<Key, Value>::operator=(cactus_arm_t&& other)
  {
    if (this != &other) {
      clear();
      cactus = std::move(other.cactus);
      idx    = std::exchange(other.idx, -1);
    }
    return *this;
  }
  template<typename Key, typename Value>
  cactus_arm_t<Key, Value>& cactus_arm_t<Key, Value>::operator=(const cactus_arm_t& other)
  {
    if (this != &other) {
      clear();
      cactus = other.cactus;
      idx    = other.idx;
      if (!cactus.is_nullptr()) {
        cactus->object_datas[idx].ref_count += 1;
      }
    }
    return *this;
  }

  template<typename Key, typename Value>
  bool cactus_arm_t<Key, Value>::is_nullptr() const
  {
    return cactus.is_nullptr();
  }
  template<typename Key, typename Value>
  void cactus_arm_t<Key, Value>::clear()
  {
    if (!cactus.is_nullptr()) {
      cactus->arms[idx].ref_count -= 1;
      if (cactus->arms[idx].ref_count == 0) {
        cactus->free_arm(idx);
      }
      cactus.clear();
      idx = -1;
    }
  }

  template<typename Key, typename Value>
  expected_t<Value*> cactus_arm_t<Key, Value>::get(const Key& k) const
  {
    index_t curr_idx = idx;
    while (true) {
      typename cactus_t<Key, Value>::arm_t* arm = &cactus->arms[curr_idx];
      if (auto* x = std::get_if<pair_t<Key, Value>>(&arm->data); x) {
        if (x->first == k) {
          return &(x->second);
        }
      }
      if (curr_idx == 0) {
        break;
      }
      curr_idx = arm->parent_index;
    }
    SILVA_EXPECT(false, MINOR, "couldn't find key");
  }
  template<typename Key, typename Value>
  expected_t<void> cactus_arm_t<Key, Value>::assign(const Key& k, Value v) const
  {
    Value* tgt_value_ptr = SILVA_EXPECT_FWD_PLAIN(get(k));
    SILVA_EXPECT(!tgt_value_ptr, ASSERT);
    *tgt_value_ptr = std::move(v);
  }
  template<typename Key, typename Value>
  cactus_arm_t<Key, Value> cactus_arm_t<Key, Value>::new_scope() const
  {
    const auto new_idx                 = cactus->alloc_arm();
    cactus->arms[new_idx].parent_index = idx;
    cactus->arms[new_idx].data         = typename cactus_t<Key, Value>::new_scope_t{};
    return cactus_arm_t(cactus, new_idx);
  }
  template<typename Key, typename Value>
  expected_t<cactus_arm_t<Key, Value>> cactus_arm_t<Key, Value>::define(const Key& k, Value v) const
  {
    using new_scope_t = typename cactus_t<Key, Value>::new_scope_t;
    index_t curr_idx  = idx;
    while (true) {
      const typename cactus_t<Key, Value>::arm_t* arm = &cactus->arms[curr_idx];
      if (const auto* x = std::get_if<pair_t<Key, Value>>(&arm->data); x) {
        SILVA_EXPECT(x->first != k, MINOR, "key already defined in the current scope");
      }
      if (const auto* x = std::get_if<new_scope_t>(&arm->data); x) {
        break;
      }
      if (curr_idx == 0) {
        break;
      }
      curr_idx = arm->parent_index;
    }

    const auto new_idx                 = cactus->alloc_arm();
    cactus->arms[new_idx].parent_index = idx;
    cactus->arms[new_idx].data         = pair_t<Key, Value>{k, std::move(v)};
    return cactus_arm_t(cactus, new_idx);
  }
}
