/*
 * libdigidocpp
 *
 * Regression test for repeated digidoc::initialize() / digidoc::terminate()
 * cycles within a single process. The public init/terminate API has the
 * shape of a symmetric pair, but is not exercised in the existing test
 * suite — every other test only calls them once via DigiDocPPFixture.
 *
 * On macOS arm64 and Windows with current EU TSL data this crashes during
 * the second initialize() call (during xmlsec/libxml2 re-init after
 * xmlCleanupParser() in the prior terminate()). Linux currently passes.
 */

#define BOOST_TEST_MODULE "Repeated initialize/terminate tests"
#include "test.h"

namespace {

// Fixture: sets the working directory and resolves the test data path,
// but deliberately does NOT call digidoc::initialize() — each test drives
// the init/terminate cycle itself.
class MultipleInitFixture {
public:
    MultipleInitFixture()
    {
        int argc = boost::unit_test::framework::master_test_suite().argc;
        if(argc > 1)
        {
            fs::current_path(boost::unit_test::framework::master_test_suite().argv[argc - 1]);
            dataPath = boost::unit_test::framework::master_test_suite().argv[argc - 1];
        }
        if(!fs::is_regular_file(fs::path("TSL.xml")))
            throw runtime_error("TSL.xml not found in data path: " + dataPath);
        boost::unit_test::unit_test_monitor.register_exception_translator<Exception>(&translate);
    }

    void installConf() const
    {
        Conf::init(new TestConfig(string("TSL.xml"), string(dataPath)));
    }

    static void translate(const Exception &e)
    {
        stringstream s;
        s << '\n' << e.file() << '(' << e.line() << "): " << e.msg();
        BOOST_ERROR(s.str().c_str());
        for(const Exception &cause: e.causes())
            translate(cause);
    }

    string dataPath = ".";
};

} // namespace

BOOST_FIXTURE_TEST_SUITE(MultipleInitSuite, MultipleInitFixture)

BOOST_AUTO_TEST_CASE(InitTerminateInit)
{
    // Cycle 1
    installConf();
    BOOST_REQUIRE_NO_THROW(digidoc::initialize("multi-init-test"));
    BOOST_REQUIRE_NO_THROW(digidoc::terminate());

    // Cycle 2 — same calls; crashes on macOS arm64 / Windows.
    installConf();
    BOOST_REQUIRE_NO_THROW(digidoc::initialize("multi-init-test"));
    BOOST_REQUIRE_NO_THROW(digidoc::terminate());
}

BOOST_AUTO_TEST_SUITE_END()
