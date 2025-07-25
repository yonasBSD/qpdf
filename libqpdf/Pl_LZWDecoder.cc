#include <qpdf/Pl_LZWDecoder.hh>

#include <qpdf/QIntC.hh>
#include <qpdf/QTC.hh>
#include <qpdf/QUtil.hh>
#include <cstring>
#include <stdexcept>

Pl_LZWDecoder::Pl_LZWDecoder(char const* identifier, Pipeline* next, bool early_code_change) :
    Pipeline(identifier, next),
    code_change_delta(early_code_change)
{
    if (!next) {
        throw std::logic_error("Attempt to create Pl_LZWDecoder with nullptr as next");
    }
}

void
Pl_LZWDecoder::write(unsigned char const* bytes, size_t len)
{
    for (size_t i = 0; i < len; ++i) {
        buf[next_char_++] = bytes[i];
        if (next_char_ == 3) {
            next_char_ = 0;
        }
        bits_available += 8;
        if (bits_available >= code_size) {
            sendNextCode();
        }
    }
}

void
Pl_LZWDecoder::finish()
{
    next()->finish();
}

void
Pl_LZWDecoder::sendNextCode()
{
    unsigned int high = byte_pos;
    unsigned int med = (byte_pos + 1) % 3;
    unsigned int low = (byte_pos + 2) % 3;

    unsigned int bits_from_high = 8 - bit_pos;
    unsigned int bits_from_med = code_size - bits_from_high;
    unsigned int bits_from_low = 0;
    if (bits_from_med > 8) {
        bits_from_low = bits_from_med - 8;
        bits_from_med = 8;
    }
    unsigned int high_mask = (1U << bits_from_high) - 1U;
    unsigned int med_mask = 0xff - ((1U << (8 - bits_from_med)) - 1U);
    unsigned int low_mask = 0xff - ((1U << (8 - bits_from_low)) - 1U);
    unsigned int code = 0;
    code += (buf[high] & high_mask) << bits_from_med;
    code += ((buf[med] & med_mask) >> (8 - bits_from_med));
    if (bits_from_low) {
        code <<= bits_from_low;
        code += ((buf[low] & low_mask) >> (8 - bits_from_low));
        byte_pos = low;
        bit_pos = bits_from_low;
    } else {
        byte_pos = med;
        bit_pos = bits_from_med;
    }
    if (bit_pos == 8) {
        bit_pos = 0;
        ++byte_pos;
        byte_pos %= 3;
    }
    bits_available -= code_size;

    handleCode(code);
}

unsigned char
Pl_LZWDecoder::getFirstChar(unsigned int code)
{
    unsigned char result = '\0';
    if (code < 256) {
        result = static_cast<unsigned char>(code);
    } else if (code > 257) {
        unsigned int idx = code - 258;
        if (idx >= table.size()) {
            throw std::runtime_error("Pl_LZWDecoder::getFirstChar: table overflow");
        }
        Buffer& b = table.at(idx);
        result = b.getBuffer()[0];
    } else {
        throw std::runtime_error(
            "Pl_LZWDecoder::getFirstChar called with invalid code (" + std::to_string(code) + ")");
    }
    return result;
}

void
Pl_LZWDecoder::addToTable(unsigned char c)
{
    unsigned int last_size = 0;
    unsigned char const* last_data = nullptr;
    unsigned char tmp[1];

    if (last_code < 256) {
        tmp[0] = static_cast<unsigned char>(last_code);
        last_data = tmp;
        last_size = 1;
    } else if (last_code > 257) {
        unsigned int idx = last_code - 258;
        if (idx >= table.size()) {
            throw std::runtime_error("Pl_LZWDecoder::addToTable: table overflow");
        }
        Buffer& b = table.at(idx);
        last_data = b.getBuffer();
        last_size = QIntC::to_uint(b.getSize());
    } else {
        throw std::runtime_error(
            "Pl_LZWDecoder::addToTable called with invalid code (" + std::to_string(last_code) +
            ")");
    }

    Buffer entry(1 + last_size);
    unsigned char* new_data = entry.getBuffer();
    memcpy(new_data, last_data, last_size);
    new_data[last_size] = c;
    table.push_back(std::move(entry));
}

void
Pl_LZWDecoder::handleCode(unsigned int code)
{
    if (eod) {
        return;
    }

    if (code == 256) {
        if (!table.empty()) {
            QTC::TC("libtests", "Pl_LZWDecoder intermediate reset");
        }
        table.clear();
        code_size = 9;
    } else if (code == 257) {
        eod = true;
    } else {
        if (last_code != 256) {
            // Add to the table from last time.  New table entry would be what we read last plus the
            // first character of what we're reading now.
            unsigned char next_c = '\0';
            unsigned int table_size = QIntC::to_uint(table.size());
            if (code < 256) {
                // just read < 256; last time's next_c was code
                next_c = static_cast<unsigned char>(code);
            } else if (code > 257) {
                size_t idx = code - 258;
                if (idx > table_size) {
                    throw std::runtime_error("LZWDecoder: bad code received");
                } else if (idx == table_size) {
                    // The encoder would have just created this entry, so the first character of
                    // this entry would have been the same as the first character of the last entry.
                    QTC::TC("libtests", "Pl_LZWDecoder last was table size");
                    next_c = getFirstChar(last_code);
                } else {
                    next_c = getFirstChar(code);
                }
            }
            unsigned int new_idx = 258 + table_size;
            if (new_idx == 4096) {
                throw std::runtime_error("LZWDecoder: table full");
            }
            addToTable(next_c);
            unsigned int change_idx = new_idx + code_change_delta;
            if ((change_idx == 511) || (change_idx == 1023) || (change_idx == 2047)) {
                ++code_size;
            }
        }

        if (code < 256) {
            auto ch = static_cast<unsigned char>(code);
            next()->write(&ch, 1);
        } else {
            unsigned int idx = code - 258;
            if (idx >= table.size()) {
                throw std::runtime_error("Pl_LZWDecoder::handleCode: table overflow");
            }
            Buffer& b = table.at(idx);
            next()->write(b.getBuffer(), b.getSize());
        }
    }

    last_code = code;
}
