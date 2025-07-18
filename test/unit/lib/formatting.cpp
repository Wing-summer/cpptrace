#include <gtest/gtest.h>
#include <gtest/gtest-matchers.h>
#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>

#include "utils/microfmt.hpp"
#include "utils/utils.hpp"

#include <cstdint>

#ifdef TEST_MODULE
import cpptrace;
#else
#include <cpptrace/formatting.hpp>
#endif

using cpptrace::detail::split;
using testing::ElementsAre;

namespace {

#if UINTPTR_MAX > 0xffffffff
 #define ADDR_PREFIX "00000000"
 #define PADDING_TAG "        "
 #define INLINED_TAG "(inlined)         "
#else
 #define PADDING_TAG ""
 #define ADDR_PREFIX ""
 #define INLINED_TAG "(inlined) "
#endif

cpptrace::stacktrace make_test_stacktrace(size_t count = 1) {
    cpptrace::stacktrace trace;
    for(size_t i = 0; i < count; i++) {
        trace.frames.push_back({0x1, 0x1001, {20}, {30}, "foo.cpp", "foo()", false});
        trace.frames.push_back({0x2, 0x1002, {30}, {40}, "bar.cpp", "bar()", false});
        trace.frames.push_back({0x3, 0x1003, {40}, {25}, "foo.cpp", "main", false});
    }
    return trace;
}

TEST(FormatterTest, Basic) {
    auto res = split(cpptrace::get_default_formatter().format(make_test_stacktrace()), "\n");
    EXPECT_THAT(
        res,
        ElementsAre(
            "Stack trace (most recent call first):",
            "#0 0x" ADDR_PREFIX "00000001 in foo() at foo.cpp:20:30",
            "#1 0x" ADDR_PREFIX "00000002 in bar() at bar.cpp:30:40",
            "#2 0x" ADDR_PREFIX "00000003 in main at foo.cpp:40:25"
        )
    );
}

TEST(FormatterTest, Inlines) {
    auto trace = make_test_stacktrace();
    trace.frames[1].is_inline = true;
    trace.frames[1].raw_address = 0;
    trace.frames[1].object_address = 0;
    auto res = split(cpptrace::get_default_formatter().format(trace), "\n");
    EXPECT_THAT(
        res,
        ElementsAre(
            "Stack trace (most recent call first):",
            "#0 0x" ADDR_PREFIX "00000001 in foo() at foo.cpp:20:30",
            "#1 " INLINED_TAG " in bar() at bar.cpp:30:40",
            "#2 0x" ADDR_PREFIX "00000003 in main at foo.cpp:40:25"
        )
    );
}

TEST(FormatterTest, Header) {
    auto formatter = cpptrace::formatter{}
        .header("Stack trace:");
    auto res = split(formatter.format(make_test_stacktrace()), "\n");
    EXPECT_THAT(
        res,
        ElementsAre(
            "Stack trace:",
            "#0 0x" ADDR_PREFIX "00000001 in foo() at foo.cpp:20:30",
            "#1 0x" ADDR_PREFIX "00000002 in bar() at bar.cpp:30:40",
            "#2 0x" ADDR_PREFIX "00000003 in main at foo.cpp:40:25"
        )
    );
}

TEST(FormatterTest, NoColumn) {
    auto formatter = cpptrace::formatter{}
        .columns(false);
    auto res = split(formatter.format(make_test_stacktrace()), "\n");
    EXPECT_THAT(
        res,
        ElementsAre(
            "Stack trace (most recent call first):",
            "#0 0x" ADDR_PREFIX "00000001 in foo() at foo.cpp:20",
            "#1 0x" ADDR_PREFIX "00000002 in bar() at bar.cpp:30",
            "#2 0x" ADDR_PREFIX "00000003 in main at foo.cpp:40"
        )
    );
}

TEST(FormatterTest, ObjectAddresses) {
    auto formatter = cpptrace::formatter{}
        .addresses(cpptrace::formatter::address_mode::object);
    auto res = split(formatter.format(make_test_stacktrace()), "\n");
    EXPECT_THAT(
        res,
        ElementsAre(
            "Stack trace (most recent call first):",
            "#0 0x" ADDR_PREFIX "00001001 in foo() at foo.cpp:20:30",
            "#1 0x" ADDR_PREFIX "00001002 in bar() at bar.cpp:30:40",
            "#2 0x" ADDR_PREFIX "00001003 in main at foo.cpp:40:25"
        )
    );
}

TEST(FormatterTest, NoAddresses) {
    auto formatter = cpptrace::formatter{}
        .addresses(cpptrace::formatter::address_mode::none);
    auto res = split(formatter.format(make_test_stacktrace()), "\n");
    EXPECT_THAT(
        res,
        ElementsAre(
            "Stack trace (most recent call first):",
            "#0 in foo() at foo.cpp:20:30",
            "#1 in bar() at bar.cpp:30:40",
            "#2 in main at foo.cpp:40:25"
        )
    );
}

TEST(FormatterTest, BreakBeforeFilename) {
    auto formatter = cpptrace::formatter{}
        .break_before_filename(true);
    EXPECT_THAT(
        split(formatter.format(make_test_stacktrace()), "\n"),
        ElementsAre(
            "Stack trace (most recent call first):",
            "#0 0x" ADDR_PREFIX "00000001 in foo()",
            "     " PADDING_TAG "         at foo.cpp:20:30",
            "#1 0x" ADDR_PREFIX "00000002 in bar()",
            "     " PADDING_TAG "         at bar.cpp:30:40",
            "#2 0x" ADDR_PREFIX "00000003 in main",
            "     " PADDING_TAG "         at foo.cpp:40:25"
        )
    );
}

TEST(FormatterTest, BreakBeforeFilenameNoAddresses) {
    auto formatter = cpptrace::formatter{}
        .break_before_filename(true)
        .addresses(cpptrace::formatter::address_mode::none);
    // Check that if no address is present, the filename indent is reduced
    EXPECT_THAT(
        split(formatter.format(make_test_stacktrace()), "\n"),
        ElementsAre(
            "Stack trace (most recent call first):",
            "#0 in foo()",
            "   at foo.cpp:20:30",
            "#1 in bar()",
            "   at bar.cpp:30:40",
            "#2 in main",
            "   at foo.cpp:40:25"
        )
    );
}

TEST(FormatterTest, BreakBeforeFilenameInlines) {
    auto formatter = cpptrace::formatter{}
        .break_before_filename(true);

    // Check that indentation is computed correctly when elements are inlined
    auto trace = make_test_stacktrace();
    trace.frames[1].is_inline = true;
    EXPECT_THAT(
        split(formatter.format(trace), "\n"),
        ElementsAre(
            "Stack trace (most recent call first):",
            "#0 0x" ADDR_PREFIX "00000001 in foo()",
            "     " PADDING_TAG "         at foo.cpp:20:30",
            "#1 " INLINED_TAG " in bar()",
            "     " PADDING_TAG "         at bar.cpp:30:40",
            "#2 0x" ADDR_PREFIX "00000003 in main",
            "     " PADDING_TAG "         at foo.cpp:40:25"
        )
    );
}

TEST(FormatterTest, BreakBeforeFilenameLongTrace) {
    // Check that indentation is computed correctly for longer traces (where the
    // frame number is padded)
    auto formatter = cpptrace::formatter{}
        .break_before_filename(true);

    EXPECT_THAT(
        split(formatter.format(make_test_stacktrace(4)), "\n"),
        ElementsAre(
            "Stack trace (most recent call first):",
            "#0  0x" ADDR_PREFIX "00000001 in foo()",
            "     " PADDING_TAG "          at foo.cpp:20:30",
            "#1  0x" ADDR_PREFIX "00000002 in bar()",
            "     " PADDING_TAG "          at bar.cpp:30:40",
            "#2  0x" ADDR_PREFIX "00000003 in main",
            "     " PADDING_TAG "          at foo.cpp:40:25",
            "#3  0x" ADDR_PREFIX "00000001 in foo()",
            "     " PADDING_TAG "          at foo.cpp:20:30",
            "#4  0x" ADDR_PREFIX "00000002 in bar()",
            "     " PADDING_TAG "          at bar.cpp:30:40",
            "#5  0x" ADDR_PREFIX "00000003 in main",
            "     " PADDING_TAG "          at foo.cpp:40:25",
            "#6  0x" ADDR_PREFIX "00000001 in foo()",
            "     " PADDING_TAG "          at foo.cpp:20:30",
            "#7  0x" ADDR_PREFIX "00000002 in bar()",
            "     " PADDING_TAG "          at bar.cpp:30:40",
            "#8  0x" ADDR_PREFIX "00000003 in main",
            "     " PADDING_TAG "          at foo.cpp:40:25",
            "#9  0x" ADDR_PREFIX "00000001 in foo()",
            "     " PADDING_TAG "          at foo.cpp:20:30",
            "#10 0x" ADDR_PREFIX "00000002 in bar()",
            "     " PADDING_TAG "          at bar.cpp:30:40",
            "#11 0x" ADDR_PREFIX "00000003 in main",
            "     " PADDING_TAG "          at foo.cpp:40:25"
        )
    );
}

TEST(FormatterTest, BreakBeforeFilenameColors) {
    // Check that indentation is computed correctly with colors enabled
    // (If microfmt is updated to count the number of characters printed,
    // it will need to _exclude_ colors for the purposes of computing
    // alignment)
    auto formatter = cpptrace::formatter{}
        .break_before_filename(true)
        .colors(cpptrace::formatter::color_mode::always);

    EXPECT_THAT(
        split(formatter.format(make_test_stacktrace()), "\n"),
        ElementsAre(
            "Stack trace (most recent call first):",
            "#0 \x1B[34m0x" ADDR_PREFIX "00000001\x1B[0m in \x1B[33mfoo()\x1B[0m",
            "     " PADDING_TAG "         at \x1B[32mfoo.cpp\x1B[0m:\x1B[34m20\x1B[0m:\x1B[34m30\x1B[0m",
            "#1 \x1B[34m0x" ADDR_PREFIX "00000002\x1B[0m in \x1B[33mbar()\x1B[0m",
            "     " PADDING_TAG "         at \x1B[32mbar.cpp\x1B[0m:\x1B[34m30\x1B[0m:\x1B[34m40\x1B[0m",
            "#2 \x1B[34m0x" ADDR_PREFIX "00000003\x1B[0m in \x1B[33mmain\x1B[0m",
            "     " PADDING_TAG "         at \x1B[32mfoo.cpp\x1B[0m:\x1B[34m40\x1B[0m:\x1B[34m25\x1B[0m"
        )
    );
}

TEST(FormatterTest, PathShortening) {
    cpptrace::stacktrace trace;
    trace.frames.push_back({0x1, 0x1001, {20}, {30}, "/home/foo/foo.cpp", "foo()", false});
    trace.frames.push_back({0x2, 0x1002, {30}, {40}, "/bar.cpp", "bar()", false});
    trace.frames.push_back({0x3, 0x1003, {40}, {25}, "baz/foo.cpp", "main", false});
    trace.frames.push_back({0x3, 0x1003, {50}, {25}, "C:\\foo\\bar\\baz.cpp", "main", false});
    auto formatter = cpptrace::formatter{}
        .paths(cpptrace::formatter::path_mode::basename);
    auto res = split(formatter.format(trace), "\n");
    EXPECT_THAT(
        res,
        ElementsAre(
            "Stack trace (most recent call first):",
            "#0 0x" ADDR_PREFIX "00000001 in foo() at foo.cpp:20:30",
            "#1 0x" ADDR_PREFIX "00000002 in bar() at bar.cpp:30:40",
            "#2 0x" ADDR_PREFIX "00000003 in main at foo.cpp:40:25",
            "#3 0x" ADDR_PREFIX "00000003 in main at baz.cpp:50:25"
        )
    );
}

#ifndef CPPTRACE_NO_TEST_SNIPPETS
TEST(FormatterTest, Snippets) {
    cpptrace::stacktrace trace;
    unsigned line = __LINE__ + 1;
    trace.frames.push_back({0x1, 0x1001, {line}, {20}, __FILE__, "foo()", false});
    trace.frames.push_back({0x2, 0x1002, {line + 1}, {}, __FILE__, "foo()", false});
    auto formatter = cpptrace::formatter{}
        .snippets(true);
    auto res = split(formatter.format(trace), "\n");
    EXPECT_THAT(
        res,
        ElementsAre(
            "Stack trace (most recent call first):",
            // frame 1
            cpptrace::microfmt::format("#0 0x" ADDR_PREFIX "00000001 in foo() at {}:{}:20", __FILE__, line),
            cpptrace::microfmt::format("     {}:     cpptrace::stacktrace trace;", line - 2),
            cpptrace::microfmt::format("     {}:     unsigned line = __LINE__ + 1;", line - 1),
            cpptrace::microfmt::format(
                "   > {}:     trace.frames.push_back({0x1, 0x1001, {line}, {{20}}, __FILE__, \"foo()\", false});",
                line
            ),
            cpptrace::microfmt::format("                             ^"),
            cpptrace::microfmt::format(
                "     {}:     trace.frames.push_back({0x2, 0x1002, {line + 1}, {{}}, __FILE__, \"foo()\", false});",
                line + 1
            ),
            cpptrace::microfmt::format("     {}:     auto formatter = cpptrace::formatter{{}}", line + 2),
            // frame 2
            cpptrace::microfmt::format("#1 0x" ADDR_PREFIX "00000002 in foo() at {}:{}", __FILE__, line + 1),
            cpptrace::microfmt::format("     {}:     unsigned line = __LINE__ + 1;", line - 1),
            cpptrace::microfmt::format(
                "     {}:     trace.frames.push_back({0x1, 0x1001, {line}, {{20}}, __FILE__, \"foo()\", false});",
                line
            ),
            cpptrace::microfmt::format(
                "   > {}:     trace.frames.push_back({0x2, 0x1002, {line + 1}, {{}}, __FILE__, \"foo()\", false});",
                line + 1
            ),
            cpptrace::microfmt::format("     {}:     auto formatter = cpptrace::formatter{{}}", line + 2),
            cpptrace::microfmt::format("     {}:         .snippets(true);", line + 3)
        )
    );
    formatter.snippet_context(1);
    res = split(formatter.format(trace), "\n");
    EXPECT_THAT(
        res,
        ElementsAre(
            "Stack trace (most recent call first):",
            // frame 1
            cpptrace::microfmt::format("#0 0x" ADDR_PREFIX "00000001 in foo() at {}:{}:20", __FILE__, line),
            cpptrace::microfmt::format("     {}:     unsigned line = __LINE__ + 1;", line - 1),
            cpptrace::microfmt::format(
                "   > {}:     trace.frames.push_back({0x1, 0x1001, {line}, {{20}}, __FILE__, \"foo()\", false});",
                line
            ),
            cpptrace::microfmt::format("                             ^"),
            cpptrace::microfmt::format(
                "     {}:     trace.frames.push_back({0x2, 0x1002, {line + 1}, {{}}, __FILE__, \"foo()\", false});",
                line + 1
            ),
            // frame 2
            cpptrace::microfmt::format("#1 0x" ADDR_PREFIX "00000002 in foo() at {}:{}", __FILE__, line + 1),
            cpptrace::microfmt::format(
                "     {}:     trace.frames.push_back({0x1, 0x1001, {line}, {{20}}, __FILE__, \"foo()\", false});",
                line
            ),
            cpptrace::microfmt::format(
                "   > {}:     trace.frames.push_back({0x2, 0x1002, {line + 1}, {{}}, __FILE__, \"foo()\", false});",
                line + 1
            ),
            cpptrace::microfmt::format("     {}:     auto formatter = cpptrace::formatter{{}}", line + 2)
        )
    );
}
#endif

TEST(FormatterTest, Colors) {
    auto formatter = cpptrace::formatter{}
        .colors(cpptrace::formatter::color_mode::always);
    auto res = split(formatter.format(make_test_stacktrace()), "\n");
    EXPECT_THAT(
        res,
        ElementsAre(
            "Stack trace (most recent call first):",
            "#0 \x1B[34m0x" ADDR_PREFIX "00000001\x1B[0m in \x1B[33mfoo()\x1B[0m at \x1B[32mfoo.cpp\x1B[0m:\x1B[34m20\x1B[0m:\x1B[34m30\x1B[0m",
            "#1 \x1B[34m0x" ADDR_PREFIX "00000002\x1B[0m in \x1B[33mbar()\x1B[0m at \x1B[32mbar.cpp\x1B[0m:\x1B[34m30\x1B[0m:\x1B[34m40\x1B[0m",
            "#2 \x1B[34m0x" ADDR_PREFIX "00000003\x1B[0m in \x1B[33mmain\x1B[0m at \x1B[32mfoo.cpp\x1B[0m:\x1B[34m40\x1B[0m:\x1B[34m25\x1B[0m"
        )
    );
}

TEST(FormatterTest, Filtering) {
    auto formatter = cpptrace::formatter{}
        .filter([] (const cpptrace::stacktrace_frame& frame) -> bool {
            return frame.filename.find("foo.cpp") != std::string::npos;
        });
    auto res = split(formatter.format(make_test_stacktrace()), "\n");
    EXPECT_THAT(
        res,
        ElementsAre(
            "Stack trace (most recent call first):",
            "#0 0x" ADDR_PREFIX "00000001 in foo() at foo.cpp:20:30",
            "#1 (filtered)",
            "#2 0x" ADDR_PREFIX "00000003 in main at foo.cpp:40:25"
        )
    );
}

TEST(FormatterTest, DontShowFilteredFrames) {
    auto formatter = cpptrace::formatter{}
        .filter([] (const cpptrace::stacktrace_frame& frame) -> bool {
            return frame.filename.find("foo.cpp") != std::string::npos;
        })
        .filtered_frame_placeholders(false);
    auto res = split(formatter.format(make_test_stacktrace()), "\n");
    EXPECT_THAT(
        res,
        ElementsAre(
            "Stack trace (most recent call first):",
            "#0 0x" ADDR_PREFIX "00000001 in foo() at foo.cpp:20:30",
            "#2 0x" ADDR_PREFIX "00000003 in main at foo.cpp:40:25"
        )
    );
}

TEST(FormatterTest, Transforming) {
    auto formatter = cpptrace::formatter{}
        .transform([](cpptrace::stacktrace_frame frame) {
            static size_t count = 0;
            frame.symbol = cpptrace::microfmt::format("sym{}", count++);
            return frame;
        });
    auto res = split(formatter.format(make_test_stacktrace()), "\n");
    EXPECT_THAT(
        res,
        ElementsAre(
            "Stack trace (most recent call first):",
            "#0 0x" ADDR_PREFIX "00000001 in sym0 at foo.cpp:20:30",
            "#1 0x" ADDR_PREFIX "00000002 in sym1 at bar.cpp:30:40",
            "#2 0x" ADDR_PREFIX "00000003 in sym2 at foo.cpp:40:25"
        )
    );

    auto frame_res = split(formatter.format(make_test_stacktrace().frames[1]), "\n");
    EXPECT_THAT(
        frame_res,
        ElementsAre(
            "0x" ADDR_PREFIX "00000002 in sym3 at bar.cpp:30:40"
        )
    );
}

TEST(FormatterTest, TransformingRvalueRef) {
    auto formatter = cpptrace::formatter{}
        .transform([](cpptrace::stacktrace_frame&& frame) {
            static size_t count = 0;
            frame.symbol = cpptrace::microfmt::format("sym{}", count++);
            return std::move(frame);
        });
    auto res = split(formatter.format(make_test_stacktrace()), "\n");
    EXPECT_THAT(
        res,
        ElementsAre(
            "Stack trace (most recent call first):",
            "#0 0x" ADDR_PREFIX "00000001 in sym0 at foo.cpp:20:30",
            "#1 0x" ADDR_PREFIX "00000002 in sym1 at bar.cpp:30:40",
            "#2 0x" ADDR_PREFIX "00000003 in sym2 at foo.cpp:40:25"
        )
    );

    auto frame_res = split(formatter.format(make_test_stacktrace().frames[1]), "\n");
    EXPECT_THAT(
        frame_res,
        ElementsAre(
            "0x" ADDR_PREFIX "00000002 in sym3 at bar.cpp:30:40"
        )
    );
}

TEST(FormatterTest, MoveSemantics) {
    auto formatter = cpptrace::formatter{}
        .filter([] (const cpptrace::stacktrace_frame& frame) -> bool {
            return frame.filename.find("foo.cpp") != std::string::npos;
        });
    auto formatter2 = std::move(formatter);
    auto res = split(formatter2.format(make_test_stacktrace()), "\n");
    EXPECT_THAT(
        res,
        ElementsAre(
            "Stack trace (most recent call first):",
            "#0 0x" ADDR_PREFIX "00000001 in foo() at foo.cpp:20:30",
            "#1 (filtered)",
            "#2 0x" ADDR_PREFIX "00000003 in main at foo.cpp:40:25"
        )
    );
    cpptrace::formatter formatter3;
    formatter3 = std::move(formatter);
    auto res2 = split(formatter2.format(make_test_stacktrace()), "\n");
    EXPECT_THAT(
        res2,
        ElementsAre(
            "Stack trace (most recent call first):",
            "#0 0x" ADDR_PREFIX "00000001 in foo() at foo.cpp:20:30",
            "#1 (filtered)",
            "#2 0x" ADDR_PREFIX "00000003 in main at foo.cpp:40:25"
        )
    );
}

TEST(FormatterTest, CopySemantics) {
    auto formatter = cpptrace::formatter{}
        .filter([] (const cpptrace::stacktrace_frame& frame) -> bool {
            return frame.filename.find("foo.cpp") != std::string::npos;
        });
    auto formatter2 = formatter;
    auto res = split(formatter2.format(make_test_stacktrace()), "\n");
    EXPECT_THAT(
        res,
        ElementsAre(
            "Stack trace (most recent call first):",
            "#0 0x" ADDR_PREFIX "00000001 in foo() at foo.cpp:20:30",
            "#1 (filtered)",
            "#2 0x" ADDR_PREFIX "00000003 in main at foo.cpp:40:25"
        )
    );
    cpptrace::formatter formatter3;
    formatter3 = formatter;
    auto res2 = split(formatter2.format(make_test_stacktrace()), "\n");
    EXPECT_THAT(
        res2,
        ElementsAre(
            "Stack trace (most recent call first):",
            "#0 0x" ADDR_PREFIX "00000001 in foo() at foo.cpp:20:30",
            "#1 (filtered)",
            "#2 0x" ADDR_PREFIX "00000003 in main at foo.cpp:40:25"
        )
    );
}

TEST(FormatterTest, PrettySymbols) {
    auto normal_formatter = cpptrace::formatter{}
        .symbols(cpptrace::formatter::symbol_mode::full);
    cpptrace::stacktrace trace;
    trace.frames.push_back({0x1, 0x1001, {20}, {30}, "foo.cpp", "foo(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >)", false});
    auto res = split(normal_formatter.format(trace), "\n");
    EXPECT_THAT(
        res,
        ElementsAre(
            "Stack trace (most recent call first):",
            "#0 0x" ADDR_PREFIX "00000001 in foo(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >) at foo.cpp:20:30"
        )
    );
    auto pretty_formatter = cpptrace::formatter{}
        .symbols(cpptrace::formatter::symbol_mode::pretty);
    res = split(pretty_formatter.format(trace), "\n");
    EXPECT_THAT(
        res,
        ElementsAre(
            "Stack trace (most recent call first):",
            "#0 0x" ADDR_PREFIX "00000001 in foo(std::string) at foo.cpp:20:30"
        )
    );
}

TEST(FormatterTest, PrunedSymbols) {
    auto normal_formatter = cpptrace::formatter{}
        .symbols(cpptrace::formatter::symbol_mode::full);
    cpptrace::stacktrace trace;
    trace.frames.push_back({0x1, 0x1001, {20}, {30}, "foo.cpp", "ns::S<float, int>::foo(int, int, int)", false});
    auto res = split(normal_formatter.format(trace), "\n");
    EXPECT_THAT(
        res,
        ElementsAre(
            "Stack trace (most recent call first):",
            "#0 0x" ADDR_PREFIX "00000001 in ns::S<float, int>::foo(int, int, int) at foo.cpp:20:30"
        )
    );
    auto pruned_formatter = cpptrace::formatter{}
        .symbols(cpptrace::formatter::symbol_mode::pruned);
    res = split(pruned_formatter.format(trace), "\n");
    EXPECT_THAT(
        res,
        ElementsAre(
            "Stack trace (most recent call first):",
            "#0 0x" ADDR_PREFIX "00000001 in ns::S::foo at foo.cpp:20:30"
        )
    );
}

}
