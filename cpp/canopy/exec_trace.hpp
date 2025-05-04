#pragma once

#include "source_location.hpp"
#include "time.hpp"
#include "tree.hpp"
#include "tree_nursery.hpp"

namespace silva {
  template<typename T>
  struct exec_trace_t {
    struct item_t {
      time_point_t tp_entry;
      time_point_t tp_exit;
      source_location_t sl;
      index_t depth = 0;
      T data        = {};
    };
    vector_t<item_t> items;
    index_t depth = 0;

    struct scope_t;
    template<typename... Args>
    [[nodiscard]] scope_t scope(source_location_t, Args&&...);

    struct node_t;
    template<typename... RootNodeArgs>
    expected_t<tree_t<node_t>> as_tree(RootNodeArgs&&...) const;
  };

#define SILVA_EXEC_TRACE_SCOPE(exec_trace, ...) \
  exec_trace.scope(source_location_t::current() __VA_OPT__(, ) __VA_ARGS__)
}

// IMPLEMENTATION

namespace silva {
  template<typename T>
  struct exec_trace_t<T>::scope_t {
    exec_trace_t* et   = nullptr;
    index_t item_index = 0;

    T* operator->();

    ~scope_t();
  };

  template<typename T>
  struct exec_trace_t<T>::node_t : public tree_node_t {
    exec_trace_t<T>::item_t item;
  };

  template<typename T>
  T* exec_trace_t<T>::scope_t::operator->()
  {
    return &(et->items[item_index].data);
  }

  template<typename T>
  template<typename... Args>
  exec_trace_t<T>::scope_t exec_trace_t<T>::scope(source_location_t sloc, Args&&... args)
  {
    depth += 1;
    const index_t item_index = items.size();
    items.push_back(item_t{
        .tp_entry = time_point_t::now(),
        .tp_exit  = time_point_none,
        .sl       = sloc,
        .depth    = depth,
        .data     = T{std::forward<Args>(args)...},
    });
    return scope_t{.et = this, .item_index = item_index};
  }

  template<typename T>
  exec_trace_t<T>::scope_t::~scope_t()
  {
    et->depth -= 1;
    et->items[item_index].tp_exit = time_point_t::now();
  }

  namespace impl {
    struct exec_state_t : public tree_nursery_state_t {};

    template<typename T>
    struct exec_tree_nursery_t
      : public tree_nursery_t<typename exec_trace_t<T>::node_t,
                              exec_state_t,
                              exec_tree_nursery_t<T>> {
      using exec_node_t = typename exec_trace_t<T>::node_t;
      using stake_t     = tree_nursery_t<typename exec_trace_t<T>::node_t,
                                         exec_state_t,
                                         exec_tree_nursery_t<T>>::stake_t;
      vector_t<stake_t> stakes;

      template<typename... Args>
      exec_tree_nursery_t(Args&&... args)
      {
        stakes.push_back(this->stake());
        stakes.back().create_node();
        stakes.back().proto_node.item.data = T{std::forward<Args>(args)...};
      }

      expected_t<exec_node_t*> push_stake()
      {
        stakes.push_back(this->stake());
        stakes.back().create_node();
        return {&stakes.back().proto_node};
      }

      expected_t<void> pop_stake()
      {
        const index_t n = stakes.size();
        SILVA_EXPECT(n >= 2, MINOR);
        stakes[n - 2].add_proto_node(stakes[n - 1].commit());
        stakes.pop_back();
        return {};
      }

      expected_t<tree_t<exec_node_t>> finish() &&
      {
        SILVA_EXPECT(stakes.size() == 1, MINOR);
        stakes.back().commit();
        return std::move(this->tree);
      }
    };
  }

  template<typename T>
  template<typename... RootNodeArgs>
  expected_t<tree_t<typename exec_trace_t<T>::node_t>>
  exec_trace_t<T>::as_tree(RootNodeArgs&&... rnargs) const
  {
    impl::exec_tree_nursery_t<T> nursery(std::forward<RootNodeArgs>(rnargs)...);
    const auto pop_stack = [&nursery](const index_t depth) -> expected_t<void> {
      while (nursery.stakes.size() >= 2 && nursery.stakes.back().proto_node.item.depth >= depth) {
        nursery.pop_stake();
      }
      return {};
    };
    for (const auto& item: items) {
      SILVA_EXPECT_FWD(pop_stack(item.depth));
      SILVA_EXPECT_FWD(nursery.push_stake())->item = item;
    }
    SILVA_EXPECT_FWD(pop_stack(0));
    return std::move(nursery).finish();
  }
}
