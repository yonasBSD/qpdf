#include <qpdf/QPDFJob.hh>
#include <qpdf/QPDFUsage.hh>
#include <qpdf/QUtil.hh>

#include <cstdio>
#include <cstdlib>
#include <iostream>

static char const* whoami = nullptr;

static void
usageExit(std::string const& msg)
{
    std::cerr << '\n'
              << whoami << ": " << msg << '\n'
              << '\n'
              << "For help:\n"
              << "  " << whoami << " --help=usage       usage information\n"
              << "  " << whoami << " --help=topic       help on a topic\n"
              << "  " << whoami << " --help=--option    help on an option\n"
              << "  " << whoami << " --help             general help and a topic list\n"
              << '\n';
    exit(QPDFJob::EXIT_ERROR);
}

int
realmain(int argc, char* argv[])
{
    whoami = QUtil::getWhoami(argv[0]);
    QUtil::setLineBuf(stdout);

    QPDFJob j;
    try {
        // See "HOW TO ADD A COMMAND-LINE ARGUMENT" in README-maintainer.
        j.initializeFromArgv(argv);
        j.run();
    } catch (QPDFUsage& e) {
        usageExit(e.what());
    } catch (std::exception& e) {
        std::cerr << whoami << ": " << e.what() << '\n';
        return QPDFJob::EXIT_ERROR;
    }
    return j.getExitCode();
}

#ifdef WINDOWS_WMAIN

extern "C" int
wmain(int argc, wchar_t* argv[])
{
    return QUtil::call_main_from_wmain(argc, argv, realmain);
}

#else

int
main(int argc, char* argv[])
{
    return realmain(argc, argv);
}

#endif
