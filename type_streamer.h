#pragma once

#include <algorithm>
#include <iomanip>
#include <map>
#include <ostream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <experimental/tuple>

namespace type_streamer
{

namespace details
{
template <typename...>
using void_t = void;

template <bool B>
using bool_constant = std::integral_constant<bool, B>;

template <typename B>
struct negation : bool_constant<not bool(B::value)>
{
};

template <typename T, typename S = std::ostream>
struct is_out_streamable
{
private:
    template <typename S_, typename T_, typename = void>
    struct is_out_streamable_impl : std::false_type
    {
    };

    template <typename S_, typename T_>
    struct is_out_streamable_impl<S_, T_, void_t<decltype(std::declval<S_&>() << std::declval<T_>())>> : std::true_type
    {
    };

public:
    using value_type = is_out_streamable_impl<S, T>;
    static constexpr bool value = value_type{}.value;
};

template <typename T>
using is_non_out_streamable = negation<is_out_streamable<T>>;

template <typename T>
inline std::ostream& serialize_enum(std::ostream& out, T value,
                                    typename std::enable_if<details::is_out_streamable<T>::value>::type* = nullptr)
{
    return out << value;
}

template <typename T>
inline std::ostream& serialize_enum(std::ostream& out, T value,
                                    typename std::enable_if<details::is_non_out_streamable<T>::value>::type* = nullptr)
{
    return out << static_cast<int>(value);
}

} // namespace details

template <typename T, typename = void>
struct Serializable
{
    explicit Serializable(const T& wrapped) : wrapped{wrapped}
    {
    }
    const T& wrapped;
};

template <typename T>
struct Enum
{
    using value_type = T;
    T value;
};

template <typename T>
struct Serializable<T, typename std::enable_if<std::is_enum<T>::value>::type>
{
    explicit Serializable(const T& value) : wrapped{value}
    {
    }
    Enum<T> wrapped;
};

template <typename T>
inline Serializable<T> make_serializable(const T& value)
{
    return Serializable<T>{value};
}

template <typename K, typename V>
struct KeyValue
{
    KeyValue(const K& key, const V& value) : key{key}, value{value}
    {
    }
    const K& key;
    const V& value;
};

template <typename K, typename V>
inline KeyValue<K, V> kv(const K& key, const V& value)
{
    return KeyValue<K, V>{key, value};
}

struct Struct
{
    explicit Struct(std::ostream& out) : m_out{out}
    {
        m_out << "{";
    }

    Struct(std::ostream& out, const char* name) : m_out{out}
    {
        m_out << name << " {";
    }

    template <typename T>
    Struct&& var(const char* name, const T& value) &&
    {
        if (not m_empty)
        {
            m_out << ", ";
        }
        else
        {
            m_empty = false;
        }
        m_out << kv(name, value);
        return std::move(*this);
    }

    template <typename Iterator>
    Struct&& vars(Iterator begin, Iterator end) &&
    {
        if (begin != end)
        {
            m_out << kv(begin->first, begin->second);
            std::for_each(++begin, end, [this](const typename Iterator::value_type& item) {
                m_out << ", ";
                m_out << kv(item.first, item.second);
            });
        }
        return std::move(*this);
    }

    Struct& operator=(const Struct&) = delete;
    Struct& operator=(Struct&&) = delete;
    Struct(const Struct&) = delete;

private:
    std::ostream& m_out;
    bool m_empty = true;
};

inline std::ostream& operator<<(std::ostream& out, Struct&& s)
{
    return out << "}";
}

template <typename T>
std::ostream& serialize(std::ostream& out, const T& value);

inline std::ostream& serialize(std::ostream& out, const char* value)
{
    return out << std::quoted(value);
}

inline std::ostream& serialize(std::ostream& out, const std::string& value)
{
    return out << std::quoted(value);
}

template <typename T>
inline std::ostream& serialize(std::ostream& out, const Enum<T>& value)
{
    return details::serialize_enum(out, value.value);
}

template<typename T>
std::ostream& serialize_tuple_impl(std::ostream& out, const T& value)
{
    return serialize(out, value);
}

template<typename T, typename ... Ts>
std::ostream& serialize_tuple_impl(std::ostream& out, const T& head, const Ts&... tail)
{
    serialize(out, head) << ", ";
    return serialize_tuple_impl(out, tail...);
}

template<typename ... Ts>
std::ostream& serialize(std::ostream& out, const std::tuple<Ts...>& value)
{
    out << '{';
    std::experimental::apply([&out](const auto& ... elem){
        serialize_tuple_impl(out, elem...);
    }, value);
    return out << '}';
}

template <typename T, typename Allocator>
std::ostream& serialize(std::ostream& out, const std::vector<T, Allocator>& seq);

template <typename Key, typename T, typename Compare, typename Allocator>
std::ostream& serialize(std::ostream& out, const std::map<Key, T, Compare, Allocator>& seq);

template <typename T, typename U>
std::ostream& serialize(std::ostream& out, const std::pair<T, U>& pair);

template <typename Seq>
std::ostream& serialize_sequence(std::ostream& out, const Seq& seq, char begin_marker, char end_marker);

template <typename T>
inline std::ostream& operator<<(std::ostream& out, const Serializable<T>& value)
{
    return serialize(out, value.wrapped);
}

template <typename K, typename V>
inline std::ostream& operator<<(std::ostream& out, const KeyValue<K, V>& nv)
{
    return out << make_serializable(nv.key) << ": " << make_serializable(nv.value);
}

template <typename T>
inline std::ostream& serialize(std::ostream& out, const T& value)
{
    return out << value;
}

template <typename T, typename Allocator>
inline std::ostream& serialize(std::ostream& out, const std::vector<T, Allocator>& seq)
{
    return serialize_sequence(out, seq, '[', ']');
}

template <typename Key, typename T, typename Compare, typename Allocator>
std::ostream& serialize(std::ostream& out, const std::map<Key, T, Compare, Allocator>& mapping)
{
    out << '{';
    auto it = mapping.begin();
    if (it != mapping.end())
    {
        serialize(out, it->first) << ": ";
        serialize(out, it->second);

        std::for_each(++it, mapping.end(),
                      [&out](const typename std::map<Key, T, Compare, Allocator>::value_type& item) {
                          out << ", ";
                          serialize(out, item.first) << ": ";
                          serialize(out, item.second);
                      });
    }
    out << '}';
    return out;
}

template <typename T, typename U>
std::ostream& serialize(std::ostream& out, const std::pair<T, U>& pair)
{
    out << '(';
    serialize(out, pair.first) << ", ";
    return serialize(out, pair.second) << ')';
}

template <typename Seq>
inline std::ostream& serialize_sequence(std::ostream& out, const Seq& seq, char begin_marker, char end_marker)
{
    out << begin_marker;
    auto it = seq.begin();
    if (it != seq.end())
    {
        serialize(out, *it);
        std::for_each(++it, seq.end(), [&out](const typename Seq::value_type& item) {
            out << ", ";
            serialize(out, item);
        });
    }
    out << end_marker;
    return out;
}

} // namespace type_streamer
