#include "../include/OutputStream.h"

namespace streams {


ArrayOutputStream::ArrayOutputStream( ValueType *buf, SizeType bufSize )
: m_buf( buf )
, m_bufsize( bufSize ) {}


ArrayOutputStream::SizeType ArrayOutputStream::write( const ValueType *data, const SizeType dataSize ) {
    SizeType sizeToWrite = std::min<SizeType>( dataSize, freeSpace() );
    std::memcpy( m_buf + m_wpos, data, sizeToWrite );
    m_wpos += sizeToWrite;
    return sizeToWrite;
}


bool ArrayOutputStream::flush() {
    m_wpos = 0;
    return true;
}


ArrayOutputStream::SizeType ArrayOutputStream::dataSize() const {
    return m_wpos;
}


ArrayOutputStream::SizeType ArrayOutputStream::freeSpace() const {
    return m_bufsize - dataSize();
}


} // namespace streams