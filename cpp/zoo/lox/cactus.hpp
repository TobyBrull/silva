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
      index_t ref_count = 0;
      cactus_arm_t<Key, Value> parent;
      hash_map_t<Key, Value> hashmap;
      index_t next_free = -1;
    };
    vector_t<arm_t> arms;
    index_t next_free = -1;
    index_t size_occ  = 0;

    index_t alloc_arm();

    void free_arm(index_t);
    friend class cactus_arm_t<Key, Value>;

   public:
    cactus_t();

    index_t size_total() const;
    index_t size_occupied() const;

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

    friend bool operator==(const cactus_arm_t&, const cactus_arm_t&) = default;

    // Returns unexpected if the Key cannot be found somewhere along this arm to the root.
    Value* get(const Key&) const;
    Value* get_at(const Key&, index_t distance) const;

    // Returns unexpected if the Key cannot be found somewhere along this arm to the root and
    // "define_if_unavailable" is "false".
    expected_t<Value*> set(const Key&, Value, bool define_if_unavailable = false) const;

    cactus_arm_t make_child_arm() const;

    // Returns unexpected if the Key is already defined in this arm.
    expected_t<Value*> define(const Key&, Value) const;
  };
}

// IMPLEMENTATION

namespace silva {

  // cactus_t

  template<typename Key, typename Value>
  cactus_t<Key, Value>::cactus_t()
  {
    arms.push_back(arm_t{
        .ref_count = 1,
        .next_free = -1,
    });
    size_occ = 1;
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
    size_occ += 1;
    return idx;
  }

  template<typename Key, typename Value>
  void cactus_t<Key, Value>::free_arm(const index_t idx)
  {
    SILVA_ASSERT(arms[idx].ref_count == 0);
    arms[idx].hashmap.clear();
    arms[idx].next_free = next_free;
    arms[idx].parent.clear();
    next_free = idx;
    size_occ -= 1;
  }

  template<typename Key, typename Value>
  index_t cactus_t<Key, Value>::size_total() const
  {
    return arms.size();
  }
  template<typename Key, typename Value>
  index_t cactus_t<Key, Value>::size_occupied() const
  {
    return size_occ;
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
      cactus->arms[idx].ref_count += 1;
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
        cactus->arms[idx].ref_count += 1;
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
  Value* cactus_arm_t<Key, Value>::get(const Key& k) const
  {
    index_t curr_idx = idx;
    while (true) {
      auto& arm     = cactus->arms[curr_idx];
      const auto it = arm.hashmap.find(k);
      if (it != arm.hashmap.end()) {
        return &(it->second);
      }
      if (curr_idx == 0) {
        break;
      }
      curr_idx = arm.parent.idx;
    }
    return nullptr;
  }

  template<typename Key, typename Value>
  Value* cactus_arm_t<Key, Value>::get_at(const Key& k, index_t dist) const
  {
    index_t curr_idx = idx;
    while (dist > 0 && curr_idx != 0) {
      auto& arm = cactus->arms[curr_idx];
      curr_idx  = arm.parent.idx;
      dist -= 1;
    }
    if (dist > 0) {
      return nullptr;
    }
    auto& arm     = cactus->arms[curr_idx];
    const auto it = arm.hashmap.find(k);
    if (it != arm.hashmap.end()) {
      return &(it->second);
    }
    else {
      return nullptr;
    }
  }

  template<typename Key, typename Value>
  expected_t<Value*>
  cactus_arm_t<Key, Value>::set(const Key& k, Value v, const bool define_if_unavailable) const
  {
    index_t curr_idx = idx;
    while (true) {
      auto& arm     = cactus->arms[curr_idx];
      const auto it = arm.hashmap.find(k);
      if (it != arm.hashmap.end()) {
        it->second = std::move(v);
        return &(it->second);
      }
      if (curr_idx == 0) {
        break;
      }
      curr_idx = arm.parent.idx;
    }
    SILVA_EXPECT(define_if_unavailable, MINOR, "key not in scope");
    return define(k, std::move(v));
  }

  template<typename Key, typename Value>
  cactus_arm_t<Key, Value> cactus_arm_t<Key, Value>::make_child_arm() const
  {
    const auto new_idx           = cactus->alloc_arm();
    cactus->arms[new_idx].parent = *this;
    return cactus_arm_t(cactus, new_idx);
  }

  template<typename Key, typename Value>
  expected_t<Value*> cactus_arm_t<Key, Value>::define(const Key& k, Value v) const
  {
    auto& arm                 = cactus->arms[idx];
    const auto [it, inserted] = arm.hashmap.emplace(k, v);
    SILVA_EXPECT(inserted, MINOR, "key already defined in the current scope");
    return &(it->second);
  }
}
