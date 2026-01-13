#ifndef LIBSUGARX_STRING_H
#define LIBSUGARX_STRING_H

#include <array>
#include <format>
#include <functional>
#include <string_view>

#include "sugar_types.h"

namespace libsugarx
{
	template<std::size_t N>
	class fixed_string;

	// do NOT compare with std::hash<std::string_view>
	struct sugarx_string_hash
	{
		struct value
		{
			std::size_t v;
			explicit constexpr value(std::size_t value) noexcept : v(value) {}
			auto operator<=>(const value &other) const = default;
		};

		[[nodiscard]]
		constexpr value operator()(std::string_view str) const noexcept
		{
			std::size_t h = 0xcbf29ce484222325ULL;
			for(std::size_t i = 0; i < str.length() - 1; ++i)
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
		char *ptr_;
		std::size_t remaining_;

	public:
		using iterator_category = std::output_iterator_tag;
		using value_type = char;
		using difference_type = std::ptrdiff_t;
		using pointer = void;
		using reference = void;

		constexpr explicit bounded_buffer_iterator(char *buffer) : ptr_(buffer), remaining_(N - 1)
		{
			static_assert(N > 0, "the size of buffer iterator should be greater than 0.");
		}

		constexpr bounded_buffer_iterator &operator=(char c)
		{
			if(remaining_ > 0)
			{
				*ptr_++ = c;
				--remaining_;
			}
			return *this;
		}

		constexpr bounded_buffer_iterator &operator*() { return *this; }
		constexpr bounded_buffer_iterator &operator++() { return *this; }
		constexpr bounded_buffer_iterator operator++(int) { return *this; }

		constexpr char *end_ptr() const { return ptr_; }
	};

	template<std::size_t N, typename... Args>
	[[nodiscard]]
	constexpr fixed_string<N> string_format(std::string_view fmt, Args &&...args)
	{
		fixed_string<N> buffer;
		buffer.format(fmt, std::forward<Args>(args)...);

		return buffer;
	}

	template<std::size_t N>
	constexpr static void string_append_byte(fixed_string<N> &str, std::byte byte)
	{
		static const char hex_digits[] = "0123456789abcdef";
		unsigned char c = static_cast<unsigned char>(byte);
		str.concat(hex_digits[c >> 4]);
		str.concat(hex_digits[c & 0x0F]);
	};

	template<std::size_t N>
	[[nodiscard]]
	constexpr static fixed_string<N> bytes_to_hex_string(const_data_span bytes)
	{
		fixed_string<N> result;
		for(std::size_t i = 0; i < bytes.size() && i < N / 2; i++)
			string_append_byte(result, bytes[i]);
		return result;
	};

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
		constexpr fixed_string() noexcept
		{
			static_assert(N > 1, "String buffer must own enough memory size.");
			buffer[0] = '\0';
		}
		constexpr fixed_string(std::string_view other) noexcept
		{
			static_assert(N > 1, "String buffer must own enough memory size.");
			copy(other);
		}

		constexpr bool copy(std::string_view other) noexcept
		{
			std::size_t len = std::min(other.size(), N - 1);
			std::copy(other.data(), other.data() + len, buffer.data());
			buffer[len] = '\0';
			return true;
		}

		constexpr void concat(const char &chr) noexcept
		{
			std::size_t current_len = length();
			std::ptrdiff_t available = N - current_len - 1;

			if(available > 0)
			{
				buffer[current_len] = chr;
				buffer[current_len + 1] = '\0';
			}
		}

		constexpr void concat(std::string_view other)
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
		/*
		return true on success
		*/
		constexpr bool format(std::string_view fmt, Args &&...args)
		{
			if constexpr(sizeof...(Args) == 0)
			{
				return copy(fmt);
			}

			try
			{
				bounded_buffer_iterator<N> iter(data());
				auto end = std::vformat_to(iter, fmt, std::make_format_args(args...));
				*(end.end_ptr()) = '\0';
			}
			catch(const std::exception &e)
			{
				return false;
			}
			return true;
		}

		constexpr std::size_t find(std::string_view sub, std::size_t pos = 0U) const noexcept { return view().find(sub, pos); }
		constexpr bool starts_with(std::string_view sub) const noexcept { return view().starts_with(sub); }

		constexpr std::size_t max_size() const { return N; }
		constexpr std::size_t length() const { return std::char_traits<char>::length(data()); }

		constexpr std::array<char, N> &buffer_data() { return buffer; }
		constexpr std::array<char, N> &buffer_data() const { return buffer; }

		constexpr char *data() { return buffer.data(); }
		const char *data() const { return buffer.data(); }

		constexpr char &operator[](std::size_t index) { return buffer[index]; }
		constexpr char &operator[](std::size_t index) const { return buffer[index]; }

		constexpr char &at(std::size_t index) { return buffer.at(index); }
		constexpr char &at(std::size_t index) const { return buffer.at(index); }
		// is empty
		constexpr bool empty() { return length() == 0; }

		constexpr void clear() { buffer[0] = '\0'; }

		constexpr std::string_view view() const { return std::string_view(data(), length()); }

		constexpr fixed_string<N> &operator=(std::string_view other)
		{
			copy(other);
			return *this;
		}
		constexpr bool operator==(std::string_view other) const { return view() == other; }

		constexpr operator std::string_view() { return view(); }
		constexpr operator std::string_view() const { return view(); }
	};

}; // namespace libsugarx

namespace std
{
	template<std::size_t N>
	struct formatter<libsugarx::fixed_string<N>, char> : formatter<string_view, char>
	{
		constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin())
		{
			return formatter<string_view, char>::parse(ctx);
		}

		template<typename FormatContext>
		auto format(const libsugarx::fixed_string<N> &buf, FormatContext &ctx) const -> decltype(ctx.out())
		{
			return formatter<string_view, char>::format(static_cast<std::string_view>(buf), ctx);
		}
	};

	template<std::size_t N>
	struct hash<libsugarx::fixed_string<N>>
	{
		[[nodiscard]]
		size_t operator()(const libsugarx::fixed_string<N> &buf) const noexcept
		{
			return std::hash<std::string_view>{}(buf.view());
		}
	};

} // namespace std

#endif // LIBSUGARX_STRING_H