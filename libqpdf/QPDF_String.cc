#include <qpdf/QPDFObject_private.hh>

#include <qpdf/QPDFObjectHandle_private.hh>
#include <qpdf/QUtil.hh>

// DO NOT USE ctype -- it is locale dependent for some things, and it's not worth the risk of
// including it in case it may accidentally be used.

static bool
is_iso_latin1_printable(char ch)
{
    return (((ch >= 32) && (ch <= 126)) || (static_cast<unsigned char>(ch) >= 160));
}

std::shared_ptr<QPDFObject>
QPDF_String::create_utf16(std::string const& utf8_val)
{
    std::string result;
    if (!QUtil::utf8_to_pdf_doc(utf8_val, result, '?')) {
        result = QUtil::utf8_to_utf16(utf8_val);
    }
    return QPDFObject::create<QPDF_String>(result);
}

void
QPDF_String::writeJSON(int json_version, JSON::Writer& p)
{
    auto candidate = getUTF8Val();
    if (json_version == 1) {
        p << "\"" << JSON::Writer::encode_string(candidate) << "\"";
    } else {
        // See if we can unambiguously represent as Unicode.
        if (QUtil::is_utf16(val) || QUtil::is_explicit_utf8(val)) {
            p << "\"u:" << JSON::Writer::encode_string(candidate) << "\"";
            return;
        } else if (!useHexString()) {
            std::string test;
            if (QUtil::utf8_to_pdf_doc(candidate, test, '?') && (test == val)) {
                // This is a PDF-doc string that can be losslessly encoded as Unicode.
                p << "\"u:" << JSON::Writer::encode_string(candidate) << "\"";
                return;
            }
        }
        p << "\"b:" << QUtil::hex_encode(val) << "\"";
    }
}

bool
QPDF_String::useHexString() const
{
    // Heuristic: use the hexadecimal representation of a string if there are any non-printable (in
    // PDF Doc encoding) characters or if too large of a proportion of the string consists of
    // non-ASCII characters.
    unsigned int non_ascii = 0;
    for (auto const ch: val) {
        if (ch > 126) {
            ++non_ascii;
        } else if (ch >= 32) {
            continue;
        } else if (ch < 0 || ch >= 24) {
            ++non_ascii;
        } else if (!(ch == '\n' || ch == '\r' || ch == '\t' || ch == '\b' || ch == '\f')) {
            return true;
        }
    }
    return 5 * non_ascii > val.length();
}

std::string
QPDF_String::unparse(bool force_binary)
{
    bool use_hexstring = force_binary || useHexString();
    std::string result;
    if (use_hexstring) {
        static auto constexpr hexchars = "0123456789abcdef";
        result.reserve(2 * val.length() + 2);
        result += '<';
        for (const char c: val) {
            result += hexchars[static_cast<unsigned char>(c) >> 4];
            result += hexchars[c & 0x0f];
        }
        result += '>';
    } else {
        result += "(";
        for (unsigned int i = 0; i < val.length(); ++i) {
            char ch = val.at(i);
            switch (ch) {
            case '\n':
                result += "\\n";
                break;

            case '\r':
                result += "\\r";
                break;

            case '\t':
                result += "\\t";
                break;

            case '\b':
                result += "\\b";
                break;

            case '\f':
                result += "\\f";
                break;

            case '(':
                result += "\\(";
                break;

            case ')':
                result += "\\)";
                break;

            case '\\':
                result += "\\\\";
                break;

            default:
                if (is_iso_latin1_printable(ch)) {
                    result += val.at(i);
                } else {
                    result += "\\" +
                        QUtil::int_to_string_base(
                                  static_cast<int>(static_cast<unsigned char>(ch)), 8, 3);
                }
                break;
            }
        }
        result += ")";
    }

    return result;
}

std::string
QPDF_String::getUTF8Val() const
{
    if (QUtil::is_utf16(val)) {
        return QUtil::utf16_to_utf8(val);
    } else if (QUtil::is_explicit_utf8(val)) {
        // PDF 2.0 allows UTF-8 strings when explicitly prefixed with the three-byte representation
        // of U+FEFF.
        return val.substr(3);
    } else {
        return QUtil::pdf_doc_to_utf8(val);
    }
}
