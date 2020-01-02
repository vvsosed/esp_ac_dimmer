#pragma once

#include <cstdint>
#include <iterator>
#include <cstring>
#include <string>
#include <memory>
#include <list>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>

namespace streams {

class InputBase {
    enum class SerializationMarker : std::uint8_t { Item, End };

public:
    using UPtr = std::unique_ptr<InputBase>;
    using SizeType = std::uint32_t;
    using ValueType = char;
    using ValuePtr = ValueType *;

    virtual ~InputBase() = default;

    virtual SizeType read( ValuePtr data, const SizeType dataSize ) = 0;

    virtual SizeType size() const {
        return 0;
    }

    virtual bool reset( const SizeType offset ) {
        return false;
    }

    template <typename ValueType, typename std::enable_if<std::is_integral<ValueType>::value || std::is_floating_point<ValueType>::value || std::is_enum<ValueType>::value, int>::type = 0>
    inline bool read( ValueType& value ) {
        if( sizeof( ValueType ) > size() ) {
            return false;
        }
        return sizeof( ValueType ) == read( reinterpret_cast<char*>( &value ), sizeof( ValueType ) );
    }

    bool read( std::string& value ) {
        SizeType itemsCount;
        if( !read( itemsCount ) ) {
            return false;
        }
        value.reserve( itemsCount );
        auto beginItr = this->begin<std::string::traits_type::char_type>( itemsCount );
        auto endItr = this->end<std::string::traits_type::char_type>();
        value = std::move( std::string( beginItr, endItr ) );
        return value.size() == itemsCount;
    }

    template <typename T>
    bool read( std::list<T>& list ) {
        return readListLikeContainer( list );
    }

    template <typename ValueType>
    bool read( std::vector<ValueType>& vec ) {
        SizeType itemsCount;
        if( !read( itemsCount ) ) {
            return false;
        }
        auto begin = this->begin<ValueType>( itemsCount );
        auto end = this->end<ValueType>();
        vec = std::move( std::vector<ValueType>( begin, end ) );
        return vec.size() == itemsCount;
    }

    template <typename T>
    bool read( std::unique_ptr<T>& uptr ) {
        auto instanceUPtr = T::create( *this );
        if( instanceUPtr ) {
            uptr = std::move( instanceUPtr );
            return true;
        }
        else {
            uptr = nullptr;
            return false;
        }
    }

    template <typename Key, typename Value>
    bool read( std::unordered_map<Key, Value>& map ) {
        return readMapLikeContainer( map );
    }

    template <typename Key, typename Value>
    bool read( std::map<Key, Value>& map ) {
        return readMapLikeContainer( map );
    }

    template <typename Value>
    bool read( std::set<Value>& set ) {
        SerializationMarker marker;
        while( read( marker ) ) {
            if( SerializationMarker::End == marker ) {
                return true;
            }
            else if( SerializationMarker::Item != marker ) {
                return false;
            }

            Value val;
            if( !read( val ) ) {
                return false;
            }
            set.insert( std::move( val ) );
        }
        return false;
    }

    template <typename T, typename std::enable_if<std::is_class<T>::value, int>::type = 0>
    inline bool read( T& object ) {
        return object.read( *this );
    }

    template <typename ValueType>
    class Iterator : public std::iterator<std::input_iterator_tag, ValueType> {
        static const int END_OF_ITERATIONS = -1;

        InputBase& m_rstream;
        int m_itemsLeft;
        ValueType m_value;

    public:
        explicit Iterator( InputBase& rstream, int itemsCount = END_OF_ITERATIONS )
        : m_rstream( rstream )
        , m_itemsLeft( itemsCount ) {
            ++( *this );
        }

        Iterator& operator++() {
            if( m_rstream.size() < sizeof( ValueType ) || m_itemsLeft <= 0 ) {
                m_itemsLeft = END_OF_ITERATIONS;
                return *this;
            }

            if( m_rstream.read( m_value ) ) {
                --m_itemsLeft;
            }
            else {
                // If we can't read from stream simply terminate iterations
                m_itemsLeft = END_OF_ITERATIONS;
            }
            return *this;
        }

        Iterator operator++( int ) {
            Iterator retval = *this;
            ++( *this );
            return retval;
        }

        bool operator==( const Iterator& other ) const {
            return m_itemsLeft == other.m_itemsLeft;
        }

        bool operator!=( const Iterator& other ) const {
            return !( *this == other );
        }

        const ValueType& operator*() const {
            return m_value;
        }
    };

    template <typename ValueType>
    Iterator<ValueType> begin( std::size_t itemsCount ) {
        if( size() < itemsCount ) {
            return end<ValueType>();
        }
        return Iterator<ValueType>( *this, itemsCount );
    }

    template <typename ValueType>
    Iterator<ValueType> end() {
        return Iterator<ValueType>( *this );
    }

private:
    template <typename Container>
    bool readListLikeContainer( Container& list ) {
        SerializationMarker marker;
        while( read( marker ) ) {
            if( SerializationMarker::End == marker ) {
                return true;
            }
            else if( SerializationMarker::Item != marker ) {
                return false;
            }

            auto itr = list.emplace( list.end() );
            if( !read( *itr ) ) {
                return false;
            }
        }
        return false;
    }

    template <typename Container>
    bool readMapLikeContainer( Container& map ) {
        SerializationMarker marker;
        while( read( marker ) ) {
            if( SerializationMarker::End == marker ) {
                return true;
            }
            else if( SerializationMarker::Item != marker ) {
                return false;
            }

            typename Container::key_type key;
            typename Container::mapped_type value;
            if( !read( key ) || !read( value ) ) {
                return false;
            }

            map.emplace( key, value );
        }
        return false;
    }
}; // class InputBase


class ArrayInputStream final : public InputBase {
    ValuePtr m_buf;
    SizeType m_bufsize;
    SizeType m_rpos = 0;

public:
    ArrayInputStream( ValuePtr buf, const SizeType bufSize );

    SizeType read( ValuePtr data, const SizeType dataSize ) override;

    using InputBase::read;

    SizeType size() const override;

    bool reset( const SizeType offset ) override;
};


} // namespace streams