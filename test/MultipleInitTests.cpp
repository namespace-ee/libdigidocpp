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

// Fixture sets up cwd + dataPath so optional offline TestConfig has somewhere
// to find TSL.xml; tests that don't installConf() use the lib's default
// ConfCurrent which fetches the live EU TSL on initialize().
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
        boost::unit_test::unit_test_monitor.register_exception_translator<Exception>(&translate);
    }

    void installOfflineConf() const
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

// Offline path: tiny static TSL.xml from test/data — currently passes on macOS.
BOOST_AUTO_TEST_CASE(InitTerminateInit_OfflineConf)
{
    installOfflineConf();
    BOOST_REQUIRE_NO_THROW(digidoc::initialize("multi-init-test"));
    BOOST_REQUIRE_NO_THROW(digidoc::terminate());

    installOfflineConf();
    BOOST_REQUIRE_NO_THROW(digidoc::initialize("multi-init-test"));
    BOOST_REQUIRE_NO_THROW(digidoc::terminate());
}

// Default ConfCurrent: live EU TSL (TSLAutoUpdate=true). Matches what
// pydigidoc's DigiDocConf does and what its CI crashes on.
BOOST_AUTO_TEST_CASE(InitTerminateInit_DefaultConf)
{
    BOOST_REQUIRE_NO_THROW(digidoc::initialize("multi-init-test"));
    BOOST_REQUIRE_NO_THROW(digidoc::terminate());

    BOOST_REQUIRE_NO_THROW(digidoc::initialize("multi-init-test"));
    BOOST_REQUIRE_NO_THROW(digidoc::terminate());
}

BOOST_AUTO_TEST_SUITE_END()
