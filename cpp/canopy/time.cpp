#include "time.hpp"

#include <chrono>
#include <format>

using namespace std::chrono;

namespace silva {
  namespace {
    using sys_days_t  = time_point<system_clock, days>;
    using sys_secs_t  = time_point<system_clock, seconds>;
    using sys_nanos_t = time_point<system_clock, nanoseconds>;

    system_clock::time_point nanos_to_chrono_system_clock(const time_repr_t nanos_since_epoch)
    {
      const nanoseconds chrono_ns(nanos_since_epoch);
      const sys_nanos_t chrono_tp(chrono_ns);
      return time_point_cast<system_clock::duration>(chrono_tp);
    }

    time_repr_t chrono_system_clock_to_nanos(const system_clock::time_point tp)
    {
      const auto ns_tp = time_point_cast<nanoseconds>(tp);
      return ns_tp.time_since_epoch().count();
    }

    struct nano_parser_t {
      string_view_t input_str;
      time_repr_t retval = 0;

      template<typename Dur>
      void add(const Dur dur)
      {
        retval += duration_cast<nanoseconds>(dur).count();
      }

      expected_t<void> digest_date()
      {
        SILVA_EXPECT(input_str.size() >= 10, MINOR);
        const string_view_t value_str = input_str.substr(0, 10);
        input_str                     = input_str.substr(10);
        std::istringstream in{string_t{value_str}};
        year_month_day ymd;
        in >> parse("%Y-%m-%d", ymd);
        add(sys_days_t{ymd}.time_since_epoch());
        return {};
      }

      template<typename Dur>
      expected_t<void> digest_var(const char end_char)
      {
        index_t i = 0;
        while (i < input_str.size() && input_str[i] != end_char) {
          SILVA_EXPECT(std::isdigit(input_str[i]), MINOR);
          i += 1;
        }
        const string_view_t value_str = input_str.substr(0, i);
        input_str                     = input_str.substr(i);
        const time_repr_t value       = std::stoi(string_t{value_str});
        add(days{value});
        return {};
      }

      expected_t<void> digest_char(const char expected_char)
      {
        SILVA_EXPECT(!input_str.empty() && input_str.front() == expected_char, MINOR);
        input_str = input_str.substr(1);
        return {};
      }

      template<typename Dur>
      expected_t<void> digest(const index_t expected_num_chars)
      {
        SILVA_EXPECT(input_str.size() >= expected_num_chars, MINOR);
        for (index_t i = 0; i < expected_num_chars; ++i) {
          SILVA_EXPECT(std::isdigit(input_str[i]), MINOR);
        }
        const string_view_t value_str = input_str.substr(0, expected_num_chars);
        input_str                     = input_str.substr(expected_num_chars);
        const time_repr_t value       = std::stoi(string_t{value_str});
        add(Dur(value));
        return {};
      }

      expected_t<void> end()
      {
        SILVA_EXPECT(input_str.empty(), MINOR);
        return {};
      }
    };
  }

  time_span_t::time_span_t(const time_repr_t nanos) : nanos(nanos) {}

  string_t time_span_t::to_string_default() const
  {
    const auto [secs, total_ns] = std::lldiv(nanos, 1'000'000'000);
    const auto [ms, total_us]   = std::lldiv(total_ns, 1'000'000);
    const auto [us, ns]         = std::lldiv(total_us, 1'000);
    const seconds c_total_secs{secs};
    const days c_days         = duration_cast<days>(c_total_secs);
    const seconds c_secs      = (c_total_secs - c_days);
    const string_t c_secs_str = std::format("{:%T}", c_secs);
    return fmt::format("{}/{}/{:03}.{:03}.{:03}", c_days.count(), c_secs_str, ms, us, ns);
  }

  expected_t<time_span_t> time_span_t::from_string_default(const string_view_t input_str)
  {
    nano_parser_t dp{.input_str = input_str};
    SILVA_EXPECT_FWD(dp.template digest_var<days>('/'));
    SILVA_EXPECT_FWD(dp.digest_char('/'));
    SILVA_EXPECT_FWD(dp.template digest<hours>(2));
    SILVA_EXPECT_FWD(dp.digest_char(':'));
    SILVA_EXPECT_FWD(dp.template digest<minutes>(2));
    SILVA_EXPECT_FWD(dp.digest_char(':'));
    SILVA_EXPECT_FWD(dp.template digest<seconds>(2));
    SILVA_EXPECT_FWD(dp.digest_char('/'));
    SILVA_EXPECT_FWD(dp.template digest<milliseconds>(3));
    SILVA_EXPECT_FWD(dp.digest_char('.'));
    SILVA_EXPECT_FWD(dp.template digest<microseconds>(3));
    SILVA_EXPECT_FWD(dp.digest_char('.'));
    SILVA_EXPECT_FWD(dp.template digest<nanoseconds>(3));
    SILVA_EXPECT_FWD(dp.end());
    return time_span_t{dp.retval};
  }

  std::ostream& operator<<(std::ostream& os, const time_span_t& ts)
  {
    return os << ts.to_string_default();
  }

  time_point_t::time_point_t(const time_repr_t nanos_since_epoch)
    : nanos_since_epoch(nanos_since_epoch)
  {
  }

  bool time_point_t::is_none() const
  {
    return nanos_since_epoch == time_point_none.nanos_since_epoch;
  }

  time_point_t time_point_t::now()
  {
    const auto sys_now         = system_clock::now();
    const time_repr_t repr_now = duration_cast<nanoseconds>(sys_now.time_since_epoch()).count();
    return time_point_t{repr_now};
  }

  string_t time_point_t::to_string_default() const
  {
    if (is_none()) {
      return fmt::format("time_point_none");
    }
    else {
      const auto [secs, total_ns] = std::lldiv(nanos_since_epoch, 1'000'000'000);
      const auto [ms, total_us]   = std::lldiv(total_ns, 1'000'000);
      const auto [us, ns]         = std::lldiv(total_us, 1'000);
      const sys_secs_t sys_secs{seconds{secs}};
      const auto sys_secs_str = std::format("{:%Y-%m-%d/%H:%M:%S}", sys_secs);
      return fmt::format("{}/{:03}.{:03}.{:03}", sys_secs_str, ms, us, ns);
    }
  }

  string_t time_point_t::to_string_ostream() const
  {
    if (is_none()) {
      return fmt::format("time_point_none");
    }
    else {
      return std::format("{:%F %T}", nanos_to_chrono_system_clock(nanos_since_epoch));
    }
  }

  expected_t<time_point_t> time_point_t::from_string_default(string_view_t input_str)
  {
    nano_parser_t dp{.input_str = input_str};
    SILVA_EXPECT_FWD(dp.digest_date());
    SILVA_EXPECT_FWD(dp.digest_char('/'));
    SILVA_EXPECT_FWD(dp.template digest<hours>(2));
    SILVA_EXPECT_FWD(dp.digest_char(':'));
    SILVA_EXPECT_FWD(dp.template digest<minutes>(2));
    SILVA_EXPECT_FWD(dp.digest_char(':'));
    SILVA_EXPECT_FWD(dp.template digest<seconds>(2));
    SILVA_EXPECT_FWD(dp.digest_char('/'));
    SILVA_EXPECT_FWD(dp.template digest<milliseconds>(3));
    SILVA_EXPECT_FWD(dp.digest_char('.'));
    SILVA_EXPECT_FWD(dp.template digest<microseconds>(3));
    SILVA_EXPECT_FWD(dp.digest_char('.'));
    SILVA_EXPECT_FWD(dp.template digest<nanoseconds>(3));
    SILVA_EXPECT_FWD(dp.end());
    return time_point_t{dp.retval};
  }

  expected_t<time_point_t> time_point_t::from_string_ostream(string_view_t input_str)
  {
    nano_parser_t dp{.input_str = input_str};
    SILVA_EXPECT_FWD(dp.digest_date());
    SILVA_EXPECT_FWD(dp.digest_char(' '));
    SILVA_EXPECT_FWD(dp.template digest<hours>(2));
    SILVA_EXPECT_FWD(dp.digest_char(':'));
    SILVA_EXPECT_FWD(dp.template digest<minutes>(2));
    SILVA_EXPECT_FWD(dp.digest_char(':'));
    SILVA_EXPECT_FWD(dp.template digest<seconds>(2));
    SILVA_EXPECT_FWD(dp.digest_char('.'));
    SILVA_EXPECT_FWD(dp.template digest<nanoseconds>(9));
    SILVA_EXPECT_FWD(dp.end());
    return time_point_t{dp.retval};
  }

  std::ostream& operator<<(std::ostream& os, const time_point_t& tp)
  {
    return os << tp.to_string_default();
  }

  time_point_t operator"" _time_point(const char* str, std::size_t size)
  {
    const string_view_t sv{str, size};
    return SILVA_EXPECT_ASSERT(time_point_t::from_string_default(sv));
  }

  time_span_t operator"" _time_span(const char* str, std::size_t size)
  {
    const string_view_t sv{str, size};
    return SILVA_EXPECT_ASSERT(time_span_t::from_string_default(sv));
  }

  time_span_t operator-(const time_span_t lhs, const time_span_t rhs)
  {
    return time_span_t{lhs.nanos - rhs.nanos};
  }
  time_span_t operator+(const time_span_t lhs, const time_span_t rhs)
  {
    return time_span_t{lhs.nanos + rhs.nanos};
  }
  time_span_t operator-(const time_point_t lhs, const time_point_t rhs)
  {
    return time_span_t{lhs.nanos_since_epoch - rhs.nanos_since_epoch};
  }
  time_point_t operator+(const time_point_t lhs, const time_span_t rhs)
  {
    return time_point_t{lhs.nanos_since_epoch + rhs.nanos};
  }
  time_point_t operator-(const time_point_t lhs, const time_span_t rhs)
  {
    return time_point_t{lhs.nanos_since_epoch - rhs.nanos};
  }
}
