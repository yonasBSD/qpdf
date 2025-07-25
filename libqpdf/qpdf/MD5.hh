#ifndef MD5_HH
#define MD5_HH

#include <qpdf/QPDFCryptoImpl.hh>
#include <qpdf/Types.h>
#include <memory>
#include <string>

class MD5
{
  public:
    typedef unsigned char Digest[16];

    MD5();
    void reset();

    // encodes string and finalizes
    void encodeString(char const* input_string);

    // encodes file and finalizes; offset < 0 reads whole file
    void encodeFile(char const* filename, qpdf_offset_t up_to_offset = -1);

    // appends string to current md5 object
    void appendString(char const* input_string);

    // appends arbitrary data to current md5 object
    void
    encodeDataIncrementally(char const* input_data, size_t len)
    {
        crypto->MD5_update(reinterpret_cast<unsigned char*>(const_cast<char*>(input_data)), len);
    }

    // appends arbitrary data to current md5 object
    void
    encodeDataIncrementally(std::string_view input_data)
    {
        crypto->MD5_update(
            reinterpret_cast<unsigned char*>(const_cast<char*>(input_data.data())),
            input_data.size());
    }

    // computes a raw digest
    void digest(Digest);
    std::string digest();
    static std::string digest(std::string_view data);

    // prints the digest to stdout terminated with \r\n (primarily for testing)
    void print();

    // returns the digest as a hexadecimal string
    std::string unparse();

    // Convenience functions
    static std::string getDataChecksum(char const* buf, size_t len);
    static std::string getFileChecksum(char const* filename, qpdf_offset_t up_to_offset = -1);
    static bool checkDataChecksum(char const* const checksum, char const* buf, size_t len);
    static bool checkFileChecksum(
        char const* const checksum, char const* filename, qpdf_offset_t up_to_offset = -1);

  private:
    void init();
    void finalize();

    std::shared_ptr<QPDFCryptoImpl> crypto;
};

#endif // MD5_HH
