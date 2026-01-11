#ifndef LIBSUGARX_LAZYTABLE_H
#define LIBSUGARX_LAZYTABLE_H

#include <algorithm>
#include <functional>
#include <optional>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace libsugarx
{
    template<typename Key, typename Value>
    class lazy_flat_table_proxy
    {
        Key key_;
        std::optional<Value> value_;
    public:

        constexpr lazy_flat_table_proxy() noexcept = default;
        constexpr ~lazy_flat_table_proxy() noexcept = default;

        constexpr lazy_flat_table_proxy(lazy_flat_table_proxy<Key, Value>&& other) noexcept = default;
        constexpr lazy_flat_table_proxy<Key, Value>& operator=(lazy_flat_table_proxy&& other) noexcept = default;

        lazy_flat_table_proxy(const lazy_flat_table_proxy<Key, Value>& other) = delete;
        lazy_flat_table_proxy<Key, Value>& operator=(lazy_flat_table_proxy& other) = delete;

        template<typename... Args>
        constexpr lazy_flat_table_proxy(Key self_key, Args&&... args) :
            key_(self_key),
            value_(std::forward<Args>(args)...)
        {
        }

        constexpr Value &operator->() { return value_.value(); }
        constexpr const Value &operator->() const { return value_.value(); }

        constexpr operator Value&() { return value_.value(); }
        constexpr operator const Value&() const { return value_.value(); }

        constexpr Value& value() { return value_.value(); }
        constexpr const Value& value() const { return value_.value(); }

        lazy_flat_table_proxy *operator&() = delete;
        const lazy_flat_table_proxy *operator&() const = delete;

        constexpr Key key() noexcept { return key_; }
        constexpr const Key key() const noexcept { return key_; }
        constexpr void remove() noexcept { value_.reset(); }
        constexpr bool is_removed() const noexcept { return !value_.has_value(); }
        template<std::size_t I>
        auto get() const
        {
            if constexpr (I == 0)
            {
                return key();
            }
            else if constexpr (I == 1)
            {
                static_assert(value_.has_value(), "Proxy has been removed");
                return value();
            }
            static_assert(false, "Index out of range");
        }

        template<std::size_t I>
        auto get()
        {
            if constexpr (I == 0)
            {
                return key();
            }
            else if constexpr (I == 1)
            {
                static_assert(value_.has_value(), "Proxy has been removed");
                return value();
            }
            else
            {
                static_assert(false, "Index out of range");
            }
        }
    };

    /*
    class lazy_flat_table, a.k.a hashed_vector
    not thread safe 
    */
    template<typename Key, typename Value>
    class lazy_flat_table
    {
    public:
        using proxy = lazy_flat_table_proxy<Key, Value>;

        lazy_flat_table() noexcept = default;
        ~lazy_flat_table() noexcept = default;

        lazy_flat_table(lazy_flat_table<Key, Value>&& other) noexcept = default;
        lazy_flat_table<Key, Value>& operator=(lazy_flat_table&& other) noexcept = default;

        template<typename... Args>
        /*
        need to check if return value is available.
        */
        std::optional<std::reference_wrapper<proxy>> emplace(Key key, Args&&... args)
        {
            if(index_table.count(key))
                return std::nullopt;
            if(removed_list.empty())
            {
                proxy &result = proxies.emplace_back(key, std::forward<Args>(args)...);
                index_table[key] = proxies.size() - 1;
                return result;
            }
            std::size_t begin = *removed_list.begin();
            removed_list.erase(begin);
            proxy &result = proxies[begin] = proxy(key, std::forward<Args>(args)...);
            index_table[key] = begin;
            return std::ref(result);
        }

        bool contains(const Key& key) const noexcept
        {
            return index_table.count(key);
        }

        /*
        this is a C-style method, which means it wouldn't create new object automatically.
        */
        proxy &operator[](const Key& key)
        {
            return at(key);
        }

        proxy &at(const Key& key)
        {
            return proxies.at(index_table.at(key));
        }

        const proxy &at(const Key& key) const
        {
            return proxies.at(index_table.at(key));
        }

        std::size_t size() const noexcept
        {
            return index_table.size();
        }

        std::size_t allocated_size() const noexcept
        {
            return proxies.size();
        }

        bool empty() const noexcept
        {
            return index_table.empty();
        }

        bool allocated_empty() const noexcept
        {
            return index_table.empty() && proxies.empty();
        }

        void force_compact() noexcept
        {
            std::vector<proxy> new_vec;
            /*
            rebuild map
            */
            index_table.clear();
            for(std::size_t i = 0; i < proxies.size(); ++i)
            {
                if(!proxies[i].is_removed())
                {
                    index_table[proxies[i].key()] = new_vec.size();
                    new_vec.push_back(std::move(proxies[i]));
                }
            }
            std::swap(new_vec, proxies);
            removed_list.clear();
        }

        void compact() noexcept
        {
            while(!removed_list.empty() && !proxies.empty() && *removed_list.rbegin() == proxies.size() - 1)
            {
                removed_list.erase(*removed_list.rbegin());
                proxies.pop_back();
            }
            if(removed_list.size() > proxies.size() / 2)
            {
                force_compact();
            }
        }

        void reserve(std::size_t size)
        {
            proxies.reserve(size);
            index_table.reserve(size);
        }

        /*
        returns true as the variable exists
        */
        bool lazy_remove(const Key& key)
        {
            auto iter = index_table.find(key);
            if(iter != index_table.end() && !proxies[iter->second].is_removed())
            {
                proxies[iter->second].remove();
                index_table.erase(key);
                removed_list.insert(iter->second);
                return true;
            }
            return false;
        }

        void clear()
        {
            removed_list.clear();
            index_table.clear();
            proxies.clear();
        }

        /*
        as this would compact the vector list,
        it's not a lazy method, :3
        */
        void remove(const Key& key)
        {
            if(lazy_remove(key))
                compact();
        }

        template<typename vec_iter>
        class iterator_impl
        {
            vec_iter current_;
            vec_iter end_;

            void skip_to_available()
            {
                while (current_ != end_ && (*current_).is_removed())
                {
                    ++current_;
                }
            }

        public:
            explicit iterator_impl(vec_iter it, vec_iter end) : current_(it), end_(end)
            {
                skip_to_available();
            }

            proxy &operator*() const { return *current_; }

            iterator_impl& operator++()
            {
                ++current_;
                skip_to_available();
                return *this;
            }

            bool operator!=(const iterator_impl& other) const
            {
                return current_ != other.current_;
            }
        };

        using iterator = iterator_impl<typename std::vector<proxy>::iterator>;
        using const_iterator = iterator_impl<typename std::vector<proxy>::const_iterator>;

        iterator begin()
        {
            return iterator(proxies.begin(), proxies.end());
        }

        iterator end()
        {
            return iterator(proxies.end(), proxies.end());
        }

        const_iterator cbegin() const {
            return const_iterator(proxies.cbegin(), proxies.cend());
        }

        const_iterator cend() const {
            return const_iterator(proxies.cend(), proxies.cend());
        }
        
    private:
        std::multiset<std::size_t> removed_list;
        std::vector<proxy> proxies;
        std::unordered_map<Key, std::size_t> index_table;
    };
};

namespace std
{
    template<typename Key, typename Value>
    struct tuple_size<libsugarx::lazy_flat_table_proxy<Key, Value>> : integral_constant<size_t, 2> {};

    template<typename Key, typename Value>
    struct tuple_element<0, libsugarx::lazy_flat_table_proxy<Key, Value>> { using type = Key; };

    template<typename Key, typename Value>
    struct tuple_element<1, libsugarx::lazy_flat_table_proxy<Key, Value>> { using type = Value; };
}

template<std::size_t I, typename Key, typename Value>
decltype(auto) get(const libsugarx::lazy_flat_table_proxy<Key, Value>& p)
{
    return p.template get<I>();
}

#endif // LIBSUGARX_LAZYTABLE_H