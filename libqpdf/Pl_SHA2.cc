#include <qpdf/Pl_SHA2.hh>

#include <qpdf/QPDFCryptoProvider.hh>
#include <qpdf/QUtil.hh>

#include <stdexcept>

Pl_SHA2::Pl_SHA2(int bits, Pipeline* next) :
    Pipeline("sha2", next)
{
    if (bits) {
        resetBits(bits);
    }
}

void
Pl_SHA2::write(unsigned char const* buf, size_t len)
{
    if (!in_progress) {
        in_progress = true;
    }

    // Write in chunks in case len is too big to fit in an int. Assume int is at least 32 bits.
    static size_t const max_bytes = 1 << 30;
    size_t bytes_left = len;
    unsigned char const* data = buf;
    while (bytes_left > 0) {
        size_t bytes = (bytes_left >= max_bytes ? max_bytes : bytes_left);
        crypto->SHA2_update(data, bytes);
        bytes_left -= bytes;
        data += bytes;
    }

    if (next()) {
        next()->write(buf, len);
    }
}

void
Pl_SHA2::finish()
{
    if (next()) {
        next()->finish();
    }
    crypto->SHA2_finalize();
    in_progress = false;
}

void
Pl_SHA2::resetBits(int bits)
{
    if (in_progress) {
        throw std::logic_error("bit reset requested for in-progress SHA2 Pipeline");
    }
    crypto = QPDFCryptoProvider::getImpl();
    crypto->SHA2_init(bits);
}

std::string
Pl_SHA2::getRawDigest()
{
    if (in_progress) {
        throw std::logic_error("digest requested for in-progress SHA2 Pipeline");
    }
    return crypto->SHA2_digest();
}

std::string
Pl_SHA2::getHexDigest()
{
    if (in_progress) {
        throw std::logic_error("digest requested for in-progress SHA2 Pipeline");
    }
    return QUtil::hex_encode(getRawDigest());
}
