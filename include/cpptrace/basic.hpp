#ifndef CPPTRACE_BASIC_HPP
#define CPPTRACE_BASIC_HPP

#include <cpptrace/forward.hpp>

#include <limits>
#include <string>
#include <vector>
#include <iosfwd>

#ifdef _WIN32
#define CPPTRACE_EXPORT_ATTR __declspec(dllexport)
#define CPPTRACE_IMPORT_ATTR __declspec(dllimport)
#else
#define CPPTRACE_EXPORT_ATTR __attribute__((visibility("default")))
#define CPPTRACE_IMPORT_ATTR __attribute__((visibility("default")))
#endif

#ifdef CPPTRACE_STATIC_DEFINE
#  define CPPTRACE_EXPORT
#  define CPPTRACE_NO_EXPORT
#else
#  ifndef CPPTRACE_EXPORT
#    ifdef cpptrace_lib_EXPORTS
        /* We are building this library */
#      define CPPTRACE_EXPORT CPPTRACE_EXPORT_ATTR
#    else
        /* We are using this library */
#      define CPPTRACE_EXPORT CPPTRACE_IMPORT_ATTR
#    endif
#  endif
#endif

#ifdef _MSC_VER
 #define CPPTRACE_FORCE_NO_INLINE __declspec(noinline)
#else
 #define CPPTRACE_FORCE_NO_INLINE __attribute__((noinline))
#endif

#ifdef _MSC_VER
#pragma warning(push)
// warning C4251: using non-dll-exported type in dll-exported type, firing on std::vector<frame_ptr> and others for some
// reason
// 4275 is the same thing but for base classes
#pragma warning(disable: 4251; disable: 4275)
#endif

namespace cpptrace {
    struct CPPTRACE_EXPORT raw_trace {
        std::vector<frame_ptr> frames;
        static raw_trace current(std::size_t skip = 0);
        static raw_trace current(std::size_t skip, std::size_t max_depth);
        object_trace resolve_object_trace() const;
        stacktrace resolve() const;
        void clear();
        bool empty() const noexcept;

        using iterator = std::vector<frame_ptr>::iterator;
        using const_iterator = std::vector<frame_ptr>::const_iterator;
        inline iterator begin() noexcept { return frames.begin(); }
        inline iterator end() noexcept { return frames.end(); }
        inline const_iterator begin() const noexcept { return frames.begin(); }
        inline const_iterator end() const noexcept { return frames.end(); }
        inline const_iterator cbegin() const noexcept { return frames.cbegin(); }
        inline const_iterator cend() const noexcept { return frames.cend(); }
    };

    struct CPPTRACE_EXPORT object_frame {
        frame_ptr raw_address;
        frame_ptr object_address;
        std::string object_path;
    };

    struct CPPTRACE_EXPORT object_trace {
        std::vector<object_frame> frames;
        static object_trace current(std::size_t skip = 0);
        static object_trace current(std::size_t skip, std::size_t max_depth);
        stacktrace resolve() const;
        void clear();
        bool empty() const noexcept;

        using iterator = std::vector<object_frame>::iterator;
        using const_iterator = std::vector<object_frame>::const_iterator;
        inline iterator begin() noexcept { return frames.begin(); }
        inline iterator end() noexcept { return frames.end(); }
        inline const_iterator begin() const noexcept { return frames.begin(); }
        inline const_iterator end() const noexcept { return frames.end(); }
        inline const_iterator cbegin() const noexcept { return frames.cbegin(); }
        inline const_iterator cend() const noexcept { return frames.cend(); }
    };

    // This represents a nullable integer type
    // The max value of the type is used as a sentinel
    // This is used over std::optional because the library is C++11 and also std::optional is a bit heavy-duty for this
    // use.
    template<typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
    struct nullable {
        T raw_value;
        nullable& operator=(T value) {
            raw_value = value;
            return *this;
        }
        bool has_value() const noexcept {
            return raw_value != (std::numeric_limits<T>::max)();
        }
        T& value() noexcept {
            return raw_value;
        }
        const T& value() const noexcept {
            return raw_value;
        }
        T value_or(T alternative) const noexcept {
            return has_value() ? raw_value : alternative;
        }
        void swap(nullable& other) noexcept {
            std::swap(raw_value, other.raw_value);
        }
        void reset() noexcept {
            raw_value = (std::numeric_limits<T>::max)();
        }
        bool operator==(const nullable& other) const noexcept {
            return raw_value == other.raw_value;
        }
        bool operator!=(const nullable& other) const noexcept {
            return raw_value != other.raw_value;
        }
        constexpr static nullable null() noexcept {
            return { (std::numeric_limits<T>::max)() };
        }
    };

    struct CPPTRACE_EXPORT stacktrace_frame {
        frame_ptr raw_address;
        frame_ptr object_address;
        nullable<std::uint32_t> line;
        nullable<std::uint32_t> column;
        std::string filename;
        std::string symbol;
        bool is_inline;

        bool operator==(const stacktrace_frame& other) const {
            return raw_address == other.raw_address
                && object_address == other.object_address
                && line == other.line
                && column == other.column
                && filename == other.filename
                && symbol == other.symbol;
        }

        bool operator!=(const stacktrace_frame& other) const {
            return !operator==(other);
        }

        object_frame get_object_info() const;

        std::string to_string() const;
        std::string to_string(bool color) const;
        friend std::ostream& operator<<(std::ostream& stream, const stacktrace_frame& frame);
    };

    struct CPPTRACE_EXPORT stacktrace {
        std::vector<stacktrace_frame> frames;
        static stacktrace current(std::size_t skip = 0);
        static stacktrace current(std::size_t skip, std::size_t max_depth);
        void print() const;
        void print(std::ostream& stream) const;
        void print(std::ostream& stream, bool color) const;
        void print_with_snippets() const;
        void print_with_snippets(std::ostream& stream) const;
        void print_with_snippets(std::ostream& stream, bool color) const;
        void clear();
        bool empty() const noexcept;
        std::string to_string(bool color = false) const;
        friend std::ostream& operator<<(std::ostream& stream, const stacktrace& trace);

        using iterator = std::vector<stacktrace_frame>::iterator;
        using const_iterator = std::vector<stacktrace_frame>::const_iterator;
        inline iterator begin() noexcept { return frames.begin(); }
        inline iterator end() noexcept { return frames.end(); }
        inline const_iterator begin() const noexcept { return frames.begin(); }
        inline const_iterator end() const noexcept { return frames.end(); }
        inline const_iterator cbegin() const noexcept { return frames.cbegin(); }
        inline const_iterator cend() const noexcept { return frames.cend(); }
    private:
        void print(std::ostream& stream, bool color, bool newline_at_end, const char* header) const;
        void print_with_snippets(std::ostream& stream, bool color, bool newline_at_end, const char* header) const;
        friend void print_terminate_trace();
    };

    CPPTRACE_EXPORT raw_trace generate_raw_trace(std::size_t skip = 0);
    CPPTRACE_EXPORT raw_trace generate_raw_trace(std::size_t skip, std::size_t max_depth);
    CPPTRACE_EXPORT object_trace generate_object_trace(std::size_t skip = 0);
    CPPTRACE_EXPORT object_trace generate_object_trace(std::size_t skip, std::size_t max_depth);
    CPPTRACE_EXPORT stacktrace generate_trace(std::size_t skip = 0);
    CPPTRACE_EXPORT stacktrace generate_trace(std::size_t skip, std::size_t max_depth);

    // Path max isn't so simple, so I'm choosing 4096 which seems to encompass what all major OS's expect and should be
    // fine in all reasonable cases.
    // https://eklitzke.org/path-max-is-tricky
    // https://insanecoding.blogspot.com/2007/11/pathmax-simply-isnt.html
    #define CPPTRACE_PATH_MAX 4096

    // safe tracing interface
    // signal-safe
    CPPTRACE_EXPORT std::size_t safe_generate_raw_trace(
        frame_ptr* buffer,
        std::size_t size,
        std::size_t skip = 0
    );
    // signal-safe
    CPPTRACE_EXPORT std::size_t safe_generate_raw_trace(
        frame_ptr* buffer,
        std::size_t size,
        std::size_t skip,
        std::size_t max_depth
    );
    struct CPPTRACE_EXPORT safe_object_frame {
        frame_ptr raw_address;
        // This ends up being the real object address. It was named at a time when I thought the object base address
        // still needed to be added in
        frame_ptr address_relative_to_object_start;
        char object_path[CPPTRACE_PATH_MAX + 1];
        // To be called outside a signal handler. Not signal safe.
        object_frame resolve() const;
    };
    // signal-safe
    CPPTRACE_EXPORT void get_safe_object_frame(frame_ptr address, safe_object_frame* out);
    CPPTRACE_EXPORT bool can_signal_safe_unwind();
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
