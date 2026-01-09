#ifndef LIBSUGARX_STRING_H
#define LIBSUGARX_STRING_H

#include <array>
#include <format>
#include <functional>
#include <string_view>

namespace libsugarx
{
    template<std::size_t N>
    class fixed_string;

    using string_hash = std::hash<std::string_view>;
    // do NOT compare with string_hash
    struct consteval_string_hash
    {
        struct value
        {
            std::size_t v;
            explicit consteval value(std::size_t value) noexcept : v(value) {}
            auto operator<=>(const value &other) const = default;
        };
        template <std::size_t N>
        [[nodiscard]]
        consteval value operator()(const char (&str)[N]) const noexcept
        {
            std::size_t h = 0xcbf29ce484222325ULL;
            for (std::size_t i = 0; i < N - 1; ++i)
            {
                h ^= static_cast<unsigned char>(str[i]);
                h *= 0x100000001b3ULL;
            }
            return value(h);
        }
    };

    template<std::size_t N>
    class bounded_buffer_iterator
    {
        char* ptr_;
        std::size_t remaining_;

    public:
        using iterator_category = std::output_iterator_tag;
        using value_type = char;
        using difference_type = std::ptrdiff_t;
        using pointer = void;
        using reference = void;

        explicit bounded_buffer_iterator(char* buffer) : ptr_(buffer), remaining_(N - 1)
        {
            if (N == 0) throw std::logic_error("N must be >= 1");
        }

        bounded_buffer_iterator& operator=(char c)
        {
            if (remaining_ > 0)
            {
                *ptr_++ = c;
                --remaining_;
            }
            return *this;
        }

        bounded_buffer_iterator& operator*() { return *this; }
        bounded_buffer_iterator& operator++() { return *this; }
        bounded_buffer_iterator operator++(int) { return *this; }

        char* end_ptr() const { return ptr_; }
    };

    template<std::size_t N, typename... Args>
    [[nodiscard]]
    fixed_string<N> string_format(std::string_view fmt, Args&&... args)
    {
        fixed_string<N> buffer;
        buffer.format(fmt, std::forward<Args>(args)...);

        return buffer;
    }

    /*
    class fixed_string
    wrapper of std::array<char, N>
    to make it more easy to use.
    */
    template<std::size_t N>
    class fixed_string
    {
        std::array<char, N> buffer{};
    public:
        constexpr fixed_string() noexcept { static_assert(N > 1, "String buffer must own enough memory size."); buffer[0] = '\0'; }
        template<std::size_t N2>
        constexpr fixed_string(const char (&other)[N2]) noexcept { static_assert(N > 1, "String buffer must own enough memory size."); const_copy(other); }
        template<std::size_t N2>
        constexpr fixed_string(const fixed_string<N2>& other) noexcept { static_assert(N > 1, "String buffer must own enough memory size."); const_copy(other); }

        template<std::size_t N2>
        fixed_string(fixed_string<N2>& other) { static_assert(N > 1, "String buffer must own enough memory size."); copy(other); }

        template<std::size_t N2>
        constexpr void const_copy(const fixed_string<N2>& other) noexcept
        {
            constexpr std::size_t len = std::min(N2 - 1, N - 1);
            for(std::size_t i = 0; i < len; ++i)
            {
                buffer[i] = other.buffer[i];
            }
            buffer[len] = '\0';
        }

        template<std::size_t N2>
        constexpr void const_copy(const char (&other)[N2]) noexcept
        {
            constexpr std::size_t len = std::min(N2 - 1, N - 1);
            for(std::size_t i = 0; i < len; ++i)
            {
                buffer[i] = other[i];
            }
            buffer[len] = '\0';
        }

        template<std::size_t N2>
        void copy(const fixed_string<N2>& other)
        {
            std::size_t len = std::min(N2 - 1, N - 1);
            std::copy(other.data(), other.data() + len, buffer.data());
            buffer[len] = '\0';
        }

        void copy(std::string_view other) 
        {
            std::size_t len = std::min(other.size(), N - 1);
            std::copy(other.data(), other.data() + len, buffer.data());
            buffer[len] = '\0';
        }

        void concat(const char& chr) noexcept
        {
            std::size_t current_len = length();
            std::ptrdiff_t available = N - current_len - 1;

            if(available > 0)
            {
                buffer[current_len] = chr;
                buffer[current_len + 1] = '\0';
            }
        }

        template<std::size_t N2>
        void concat(const fixed_string<N2>& other)
        {
            std::size_t current_len = length();
            std::size_t append_len = other.length();
            std::ptrdiff_t available = N - current_len - 1;

            if(available > 0 && append_len > 0)
            {
                std::size_t copy_len = (append_len < available) ? append_len : available;
                std::copy(other.data(), other.data() + copy_len, data() + current_len);
                buffer[current_len + copy_len] = '\0';
            }
        }

        template<typename... Args>
        void format(std::string_view fmt, Args&&... args)
        {
            if constexpr (sizeof...(Args) == 0)
            {
                copy(fmt);
                return;
            }
            
            try
            {
                bounded_buffer_iterator<N> iter(data());
                auto end = std::vformat_to(iter, fmt, std::make_format_args(args...));
                *(end.end_ptr()) = '\0';
            }
            catch (const std::exception& e)
            {
                throw std::format_error(e.what());
            }
        }
        

        std::size_t length() { return std::char_traits<char>::length(data()); }
        std::size_t length() const { return std::char_traits<char>::length(data()); }

        std::array<char, N> &buffer_data() { return buffer; }
        std::array<char, N> &buffer_data() const { return buffer; }

        char *data() { return buffer.data(); }
        const char *data() const { return buffer.data(); }

        char &operator[](std::size_t index) { return buffer[index]; }
        constexpr char &operator[](std::size_t index) const { return buffer[index]; }

        char &at(std::size_t index) { return buffer.at(index); }
        constexpr char &at(std::size_t index) const { return buffer.at(index); }
        // is empty
        bool empty() { return length() == 0; }

        void clear() { buffer[0] = '\0'; }

        std::string_view view() const { return std::string_view(data(), length()); }

        template<std::size_t N2>
        const fixed_string<N> &operator=(const fixed_string<N2>& other) { copy(other); return *this; } 
        const fixed_string<N> &operator=(std::string_view other) { copy(other); return *this; }

        template<std::size_t N2>
        bool operator==(const fixed_string<N2>& other) const { return std::equal(buffer.begin(), buffer.begin() + length(), other.buffer.begin()); } 
        bool operator==(std::string_view other) const { return view() == other; }

        operator std::string_view() { return view(); }
        operator std::string_view() const { return view(); }
    };

};


namespace std
{
    template<std::size_t N>
    struct formatter<libsugarx::fixed_string<N>, char> : formatter<string_view, char>
    {
        constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin())
        {
            return formatter<string_view, char>::parse(ctx);
        }

        template<typename FormatContext>
        auto format(const libsugarx::fixed_string<N>& buf, FormatContext& ctx) const -> decltype(ctx.out())
        {
            return formatter<string_view, char>::format(static_cast<std::string_view>(buf), ctx);
        }
    };

    template<std::size_t N>
    struct hash<libsugarx::fixed_string<N>>
    {
        constexpr size_t operator()(const libsugarx::fixed_string<N> &buf) const noexcept
        {
            return std::hash<std::string_view>{}(buf.view());
        }
    };

}

#endif // LIBSUGARX_STRING_H