#include "../include/InputStream.h"

namespace streams {


ArrayInputStream::ArrayInputStream( ValuePtr buf, const SizeType bufSize )
: m_buf( buf )
, m_bufsize( bufSize ) {}


ArrayInputStream::SizeType ArrayInputStream::read( ValuePtr data, const SizeType dataSize ) {
    SizeType sizeToRead = std::min<SizeType>( dataSize, size() );
    std::memcpy( data, m_buf + m_rpos, sizeToRead );
    m_rpos += sizeToRead;
    return sizeToRead;
}


ArrayInputStream::SizeType ArrayInputStream::size() const {
    return m_bufsize - m_rpos;
}


bool ArrayInputStream::reset( const SizeType offset ) {
    m_rpos = offset;
    return true;
}

} // namespace streams