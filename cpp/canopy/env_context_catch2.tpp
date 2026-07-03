#include "env_context.hpp"

#include <catch2/catch_all.hpp>
#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>

namespace silva::test {
  struct env_context_catch2_t : public Catch::EventListenerBase {
    using Catch::EventListenerBase::EventListenerBase;

    env_context_t env_context;

    void testRunStarting(const Catch::TestRunInfo&) override
    {
      env_context_fill_environ(&env_context);
    }
  };
}

CATCH_REGISTER_LISTENER(silva::test::env_context_catch2_t)
