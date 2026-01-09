#ifndef LIBSUGARX_LAZYTABLE_H
#define LIBSUGARX_LAZYTABLE_H

#include <algorithm>
#include <optional>
#include <set>
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

        lazy_flat_table_proxy() noexcept = default;
        ~lazy_flat_table_proxy() noexcept = default;

        lazy_flat_table_proxy(lazy_flat_table_proxy<Key, Value>&& other) noexcept = default;
        lazy_flat_table_proxy<Key, Value>& operator=(lazy_flat_table_proxy&& other) noexcept = default;

        lazy_flat_table_proxy(const lazy_flat_table_proxy<Key, Value>& other) = delete;
        lazy_flat_table_proxy<Key, Value>& operator=(lazy_flat_table_proxy& other) = delete;

        template<typename... Args>
        lazy_flat_table_proxy(Key self_key, Args... args) :
            key_(self_key),
            value_(std::in_place, std::forward<Args>(args)...),
        {
        }

        Value &operator->() { return value_.value(); }
        const Value &operator->() const { return value_.value(); }

        operator Value&() { return value_.value(); }
        operator const Value&() const { return value_.value(); }

        Value& value() { return value_.value(); }
        const Value& value() const { return value_.value(); }

        lazy_flat_table_proxy *operator&() = delete;
        const lazy_flat_table_proxy *operator&() const = delete;

        Key key() { return key_; }
        const Key key() const { return key_; }
        void remove() { value_.reset(); }
        bool is_removed() const { return !value_.has_value(); }
        template<std::size_t I>
        decltype(auto) get() const
        {
            if constexpr (I == 0)
            {
                return key();
            }
            else if constexpr (I == 1)
            {
                if(!value_.has_value())
                {
                    throw std::runtime_error("proxy is removed");
                }
                return value();
            }
            else
            {
                static_assert(I != I, "Index out of range");
            }
        }

        template<std::size_t I>
        decltype(auto) get()
        {
            if constexpr (I == 0)
            {
                return key();
            }
            else if constexpr (I == 1)
            {
                if(!value_.has_value())
                {
                    throw std::runtime_error("proxy is removed");
                }
                return value();
            }
            else
            {
                static_assert(I != I, "Index out of range");
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
        proxy &insert(Key key, Args... args)
        {
            if(index_table.find(key) != index_table.end())
                throw std::runtime_error("Key already exists");
            std::size_t index = removed_list.empty() ? static_cast<std::size_t>(-1) : *removed_list.begin();
            if(index == static_cast<std::size_t>(-1))
            {
                proxies.emplace_back(key, std::forward<Args>(args)...);
                index = proxies.size() - 1;
            }
            else
            {
                proxies[index] = std::move(proxy(key, std::forward<Args>(args)...));
            }
            index_table[key] = index;
            return proxies[index];
        }

        bool contains(const Key& key) const noexcept
        {
            auto iter = index_table.find(key);
            return iter != index_table.end();
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

        class iterator
        {
            using vec_iter = std::vector<proxy>::iterator;
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
            explicit iterator(vec_iter it, vec_iter end) : current_(it), end_(end)
            {
                skip_to_available();
            }

            proxy &operator*() const { return *current_; }

            iterator& operator++()
            {
                ++current_;
                skip_to_available();
                return *this;
            }

            bool operator!=(const iterator& other) const
            {
                return current_ != other.current_;
            }
        };

        iterator begin() {
            return iterator(proxies.begin(), proxies.end());
        }

        iterator end() {
            return iterator(proxies.end(), proxies.end());
        }

        class const_iterator
        {
            using vec_iter = std::vector<proxy>::const_iterator;
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
            explicit const_iterator(vec_iter it, vec_iter end) : current_(it), end_(end)
            {
                skip_to_available();
            }

            proxy &operator*() const { return *current_; }

            const_iterator& operator++()
            {
                ++current_;
                skip_to_available();
                return *this;
            }

            bool operator!=(const const_iterator& other) const
            {
                return current_ != other.current_;
            }
        };

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