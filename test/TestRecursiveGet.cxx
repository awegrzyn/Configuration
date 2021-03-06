/// \file TestConfiguration.cxx
/// \brief Unit tests for the Configuration.
///
/// \author Sylvain Chapeland, CERN
/// \author Pascal Boeschoten, CERN
///
/// \todo Clean up
/// \todo Test all backends in uniform way

#include <fstream>
#include <iostream>
#include <unordered_map>
#include "Configuration/ConfigurationFactory.h"
#include "Configuration/ConfigurationInterface.h"
#include "Configuration/Visitor.h"
#include "Configuration/Tree.h"

#define BOOST_TEST_MODULE hello test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <assert.h>

using namespace std::literals::string_literals;
using namespace o2::configuration;

namespace {

inline std::string getReferenceFileName()
{
  return "/tmp/aliceo2_configuration_recursive_test.json";
}

inline std::string getConfigurationUri()
{
  return "json:/" + getReferenceFileName();
}

inline std::string getReferenceJson()
{
  return R"({
  "equipment_1":
  {
    "enabled": true,
    "type"   : "rorc",
    "serial" : 33333,
    "channel": 0
  },
  "equipment_2":
  {
    "enabled": true,
    "type"   : "dummy",
    "serial" : -1,
    "channel": 0
  }
})";
}

inline std::unordered_map<std::string, std::string> getReferenceMap()
{
  return {
    {"/equipment_1/enabled", "1"},
    {"/equipment_1/type", "rorc"},
    {"/equipment_1/serial", "33333"},
    {"/equipment_1/channel", "0"},
    {"/equipment_2/enabled", "1"},
    {"/equipment_2/type", "dummy"},
    {"/equipment_2/serial", "-1"},
    {"/equipment_2/channel", "0"},
  };
}

inline tree::Node getEquipment1()
{
  return tree::Branch {
    {"enabled", true},
    {"type", "rorc"s},
    {"serial", 33333},
    {"channel", 0}};
}

inline tree::Node getEquipment2()
{
  return tree::Branch {
    {"enabled", true},
    {"type", "dummy"s},
    {"serial", -1},
    {"channel", 0}};
}

inline tree::Node getBadEquipment2()
{
  return tree::Branch {
    {"enabled", false},
    {"type", "dummy"s},
    {"serial", -1},
    {"channel", 0}};
}

// Data structure equivalent to the reference JSON above
// This is what should be generated by the backend
inline tree::Node getReferenceTree()
{
  return tree::Branch {
    {"equipment_1", getEquipment1()},
    {"equipment_2", getEquipment2()}};
}

// Data structure NOT equivalent to the reference JSON, it contains an error
inline tree::Node getBadReferenceTree()
{
  return tree::Branch {
    {"equipment_1", getEquipment1()},
    {"equipment_2", getBadEquipment2()}};
}

void writeReferenceFile()
{
  std::ofstream stream(getReferenceFileName());
  stream << getReferenceJson();
}

BOOST_AUTO_TEST_CASE(RecursiveTest)
{
  writeReferenceFile();

  std::unique_ptr<ConfigurationInterface> conf;
  try {
    conf = ConfigurationFactory::getConfiguration(getConfigurationUri());
  }
  catch (const std::exception& e) {
    BOOST_WARN_MESSAGE(false,
        std::string("Exception thrown, you may be missing the required infrastructure: ") + e.what());
    return;
  }

  tree::Node tree = conf->getRecursive("/");
  BOOST_CHECK(tree == getReferenceTree());
  BOOST_CHECK(tree != getBadReferenceTree());
}

BOOST_AUTO_TEST_CASE(RecursiveTest2)
{
  writeReferenceFile();

  std::unique_ptr<ConfigurationInterface> conf;
  try {
    conf = ConfigurationFactory::getConfiguration(getConfigurationUri());
  }
  catch (const std::exception& e) {
    BOOST_WARN_MESSAGE(false,
        std::string("Exception thrown, you may be missing the required infrastructure: ") + e.what());
    return;
  }

  BOOST_CHECK(getReferenceTree() == conf->getRecursive("/"));
  BOOST_CHECK(getEquipment1() == conf->getRecursive("/equipment_1"));
  BOOST_CHECK(getEquipment2() == conf->getRecursive("/equipment_2"));
  BOOST_CHECK(getBadEquipment2() != conf->getRecursive("/equipment_2"));
  BOOST_CHECK(tree::get<int>(getEquipment1(), "channel") == tree::get<int>(conf->getRecursive("/equipment_1/channel")));

  conf->setPrefix("/equipment_1");
  BOOST_CHECK(getEquipment1() == conf->getRecursive("/"));

  conf->setPrefix("/equipment_2");
  BOOST_CHECK(getEquipment2() == conf->getRecursive("/"));

  conf->setPrefix("/");
  BOOST_CHECK(getReferenceTree() == conf->getRecursive("/"));
}

BOOST_AUTO_TEST_CASE(RecursiveTest3)
{
  writeReferenceFile();

  std::unique_ptr<ConfigurationInterface> conf;
  try {
    conf = ConfigurationFactory::getConfiguration(getConfigurationUri());
  }
  catch (const std::exception& e) {
    BOOST_WARN_MESSAGE(false,
        std::string("Exception thrown, you may be missing the required infrastructure: ") + e.what());
    return;
  }

  tree::Node tree = conf->getRecursive("/");
  std::ofstream stream;

  for (const auto& keyValue : tree::getBranch(tree)) {
    const auto& equipment = tree::getBranch(keyValue.second);
    stream << "Equipment '" << keyValue.first << "' \n"
      << "serial  " << tree::getRequired<int>(equipment, "serial") << '\n'
      << "channel " << tree::getRequired<int>(equipment, "channel") << '\n'
      << "enabled " << tree::getRequired<bool>(equipment, "enabled") << '\n'
      << "type    " << tree::getRequired<std::string>(equipment, "type") << '\n';

    BOOST_CHECK(!tree::get<int>(equipment, "nope").is_initialized());
  }
}

BOOST_AUTO_TEST_CASE(RecursiveMapTest)
{
  writeReferenceFile();

  std::unique_ptr<ConfigurationInterface> conf;
  try {
    conf = ConfigurationFactory::getConfiguration(getConfigurationUri());
  }
  catch (const std::exception& e) {
    BOOST_WARN_MESSAGE(false,
      std::string("Exception thrown, you may be missing the required infrastructure: ") + e.what());
    return;
  }

  ConfigurationInterface::KeyValueMap map = conf->getRecursiveMap("/");
  BOOST_CHECK(map == getReferenceMap());
}

BOOST_AUTO_TEST_CASE(TreeConversionTest)
{
  using namespace tree;

  //! [Key-value pair conversion]
  std::vector<std::pair<std::string, Leaf>> pairs {
      {"/dir/bool", false},
         {"/dir/double", 45.6},
         {"/dir/subdir/int", 123},
         {"/dir/subdir/subsubdir/string", "string"s}};

  Node convertedTree = keyValuesToTree(pairs);
  BOOST_CHECK(treeToKeyValues(convertedTree) == pairs);
}
} // Anonymous namespace
