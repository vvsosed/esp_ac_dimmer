#pragma once

#include <cstdint>
#include <type_traits>
#include <string>
#include <cstring>
#include <list>
#include <map>
#include <unordered_map>
#include <set>
#include <memory>

namespace streams {

class OutputBase {
    enum class SerializationMarker : std::uint8_t { Item, End };

public:
    using UPtr = std::unique_ptr<OutputBase>;
    using SizeType = std::uint32_t;
    using ValueType = char;

    virtual ~OutputBase() = default;

    virtual SizeType write( const ValueType *data, const SizeType size ) = 0;

    virtual bool flush() = 0;

    template <typename ValType, typename std::enable_if<std::is_integral<ValType>::value || std::is_floating_point<ValType>::value || std::is_enum<ValType>::value, int>::type = 0>
    inline bool write( const ValType& value ) {
        return sizeof( ValType ) == write( reinterpret_cast<const ValueType *>( &value ), sizeof( ValType ) );
    }

    inline bool write( const std::string& str ) {
        const SizeType lenght = str.length();
        return write( lenght ) && lenght == write( str.c_str(), lenght );
    }

    template <typename T>
    bool write( const std::list<T>& list ) {
        return writeListLikeContainer( list );
    }

    template <typename Key, typename Value>
    bool write( const std::unordered_map<Key, Value>& map ) {
        return writeMapLikeContainer( map );
    }

    template <typename Key, typename Value>
    bool write( const std::map<Key, Value>& map ) {
        return writeMapLikeContainer( map );
    }

    template <typename Value>
    bool write( const std::set<Value>& set ) {
        return writeListLikeContainer( set );
    }

    template <typename T, typename std::enable_if<std::is_class<T>::value, int>::type = 0>
    inline bool write( const T& object ) {
        return object.write( *this );
    }

private:
    template <typename Container>
    bool writeListLikeContainer( const Container& list ) {
        auto end = list.cend();
        for( auto itr = list.cbegin(); itr != end; ++itr ) {
            if( !write( SerializationMarker::Item ) ) {
                return false;
            }
            if( !write( *itr ) ) {
                return false;
            }
        }
        return write( SerializationMarker::End );
    }

    template <typename Container>
    bool writeMapLikeContainer( const Container& map ) {
        auto end = map.cend();
        for( auto itr = map.cbegin(); itr != end; ++itr ) {
            if( !write( SerializationMarker::Item ) ) {
                return false;
            }
            if( !write( itr->first ) || !write( itr->second ) ) {
                return false;
            }
        }
        return write( SerializationMarker::End );
    }
};

class ArrayOutputStream final : public OutputBase {
    ValueType *m_buf;
    SizeType m_bufsize;
    SizeType m_wpos = 0;

public:
    ArrayOutputStream( ValueType *buf, SizeType bufSize );

    SizeType write( const ValueType *data, const SizeType dataSize ) override;

    using OutputBase::write;

    bool flush() override;

    SizeType dataSize() const;

    SizeType freeSpace() const;
};

} // namespace streams