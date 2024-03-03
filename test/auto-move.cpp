#include "ruzhouxie/basic_adaptors.h"
#include "test_tool.h"
#include <array>
#include <ruzhouxie\tensor.h>
#include <utility>

namespace rzx = ruzhouxie;

struct trace_t
{
    trace_t() { std::puts("trace_t();"); };
    trace_t(const trace_t&) { std::puts("trace_t(const trace_t&);"); }
    trace_t(trace_t&&) { std::puts("trace_t(trace_t&&);"); }
    trace_t& operator=(const trace_t&) { std::puts("trace_t& operator=(const trace_t&);"); return *this; }
    trace_t& operator=(trace_t&&) { std::puts("trace_t& operator=(trace_t&&);"); return *this; }
    ~trace_t() { std::puts("~trace_t();"); }
};

int main()
{
    trace_t trace{};

    std::puts("==================");
    std::array<trace_t, 3> result = +(std::move(trace) | rzx::as_ref | rzx::repeat<3>);
    std::puts("==================");
}