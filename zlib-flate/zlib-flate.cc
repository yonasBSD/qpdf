#include <qpdf/Pl_Flate.hh>
#include <qpdf/Pl_StdioFile.hh>
#include <qpdf/QPDF.hh>
#include <qpdf/QUtil.hh>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

static char const* whoami = nullptr;

void
usage()
{
    std::cerr << "Usage: " << whoami << " { -uncompress | -compress[=n] }" << '\n'
              << "If n is specified with -compress, it is a zlib compression level from" << '\n'
              << "1 to 9 where lower numbers are faster and less compressed and higher" << '\n'
              << "numbers are slower and more compressed" << '\n';
    exit(2);
}

int
main(int argc, char* argv[])
{
    if ((whoami = strrchr(argv[0], '/')) == nullptr) {
        whoami = argv[0];
    } else {
        ++whoami;
    }

    if ((argc == 2) && (strcmp(argv[1], "--version") == 0)) {
        std::cout << whoami << " from qpdf version " << QPDF::QPDFVersion() << '\n';
        exit(0);
    }

    if (argc != 2) {
        usage();
    }

    Pl_Flate::action_e action = Pl_Flate::a_inflate;

    if ((strcmp(argv[1], "-uncompress") == 0)) {
        // okay
    } else if ((strcmp(argv[1], "-compress") == 0)) {
        action = Pl_Flate::a_deflate;
    } else if ((strncmp(argv[1], "-compress=", 10) == 0)) {
        action = Pl_Flate::a_deflate;
        int level = QUtil::string_to_int(argv[1] + 10);
        Pl_Flate::setCompressionLevel(level);
    } else if (strcmp(argv[1], "--_zopfli") == 0) {
        // Undocumented option, but that doesn't mean someone doesn't use it...
        // This is primarily here to support the test suite.
        std::cout << (Pl_Flate::zopfli_supported() ? "1" : "0")
                  << (Pl_Flate::zopfli_enabled() ? "1" : "0") << '\n';
        return 0;
    } else {
        usage();
    }

    bool warn = false;
    try {
        QUtil::binary_stdout();
        QUtil::binary_stdin();
        Pl_Flate::zopfli_check_env();
        auto out = std::make_shared<Pl_StdioFile>("stdout", stdout);
        auto flate = std::make_shared<Pl_Flate>("flate", out.get(), action);
        flate->setWarnCallback([&warn](char const* msg, int code) {
            warn = true;
            std::cerr << whoami << ": WARNING: zlib code " << code << ", msg = " << msg << '\n';
        });

        unsigned char buf[10000];
        bool done = false;
        while (!done) {
            size_t len = fread(buf, 1, sizeof(buf), stdin);
            if (len <= 0) {
                done = true;
            } else {
                flate->write(buf, len);
            }
        }
        flate->finish();
    } catch (std::exception& e) {
        std::cerr << whoami << ": " << e.what() << '\n';
        exit(2);
    }

    if (warn) {
        exit(3);
    }
    return 0;
}
