/** NDN-Atmos: Cataloging Service for distributed data originally developed
 *  for atmospheric science data
 *  Copyright (C) 2015 Colorado State University
 *
 *  NDN-Atmos is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  NDN-Atmos is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with NDN-Atmos.  If not, see <http://www.gnu.org/licenses/>.
 **/

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <json/value.h>
#include <json/writer.h>
#include <json/reader.h>
#include <iostream>
#include <fstream>
#include <getopt.h>

void
usage(const char *fileName)
{
  std::cout << "\n Usage:\n " << fileName <<
    "[-h] [-f name list file] \n"
    "   [-c catalogPrefix]  - set the catalog prefix\n"
    "   [-f name list file]  - set the file that contains name list\n"
    "   [-n namespace]       - set the publisher namespace\n"
    "   [-h]                 - print help and exit\n"
    "\n";
}

namespace ndn {
namespace atmos {

class Producer : noncopyable
{
public:
  void
  run()
  {
    if (m_jsonFile.empty()) {
      std::cout << "jsonFile is empty! exiting ..." << std::endl;
      return;
    }
    m_face.setInterestFilter(m_namespace,
                             bind(&Producer::onInterest, this, _1, _2),
                             bind(&Producer::onRegisterSucceed, this, _1),
                             bind(&Producer::onRegisterFailed, this, _1, _2));
    m_face.processEvents();
  }

private:
  void
  onInterest(const InterestFilter& filter, const Interest& interest)
  {
    std::cout << "<< I: " << interest << std::endl;

    // Create new name, based on Interest's name
    Name dataName(interest.getName());

    Json::Value publishValue;   // will contains the root value after parsing.
    Json::Reader reader;
    std::ifstream test(m_jsonFile, std::ifstream::binary);
    bool parsingSuccessful = reader.parse(test, publishValue, false);
    if (!parsingSuccessful) {
        // report to the user the failure and their locations in the document.
        std::cout << reader.getFormattedErrorMessages() << std::endl;
    }

    Json::FastWriter fastWriter;
    const std::string jsonMessage = fastWriter.write(publishValue);
    const char* payload = jsonMessage.c_str();
    size_t payLoadLength = jsonMessage.size() + 1;

    // Create Data packet
    shared_ptr<Data> data = make_shared<Data>();
    data->setName(dataName);
    data->setFreshnessPeriod(time::seconds(10));
    // todo: set the correct segment number
    // assume that the last component it the segment number
    data->setFinalBlockId(interest.getName()[-1]);
    data->setContent(reinterpret_cast<const uint8_t*>(payload), payLoadLength);

    // Sign Data packet with default identity
    m_keyChain.sign(*data);

    // Return Data packet to the requester
    std::cout << ">> D: " << *data << std::endl;
    m_face.put(*data);
    m_face.shutdown();
  }


  void
  onRegisterFailed(const Name& prefix, const std::string& reason)
  {
    std::cerr << "ERROR: Failed to register prefix \""
              << prefix << "\" in local hub's daemon (" << reason << ")"
              << std::endl;
    m_face.shutdown();
  }

  void
  onRegisterSucceed(const Name& prefix)
  {
    std::cout << "register succeed" << std::endl;
    Interest interest(Name(m_catalogPrefix).append("publish").append(m_namespace));
    interest.setInterestLifetime(time::milliseconds(1000));
    interest.setMustBeFresh(true);

    m_face.expressInterest(interest,
                           bind(&Producer::onData, this,  _1, _2),
                           bind(&Producer::onTimeout, this, _1));

    std::cout << "Sending " << interest << std::endl;
  }

  void
  onData(const Interest& interest, const Data& data)
  {
    std::cout << data << std::endl;
  }

  void
  onTimeout(const Interest& interest)
  {
    std::cout << "Timeout " << interest << std::endl;
  }

public:
  std::string m_jsonFile;
  std::string m_namespace;
  std::string m_catalogPrefix;

private:
  Face m_face;
  KeyChain m_keyChain;
};

}
}

int
main(int argc, char** argv)
{
  ndn::atmos::Producer producer;
  int option;
  if (argc < 7) {
    usage(argv[0]);
    return 0;
  }

  while ((option = getopt(argc, argv, "c:f:n:h")) != -1) {
    switch (option) {
      case 'c':
        producer.m_catalogPrefix.assign(optarg);
        break;
      case 'f':
        producer.m_jsonFile.assign(optarg);
        break;
      case 'n':
        producer.m_namespace.assign(optarg);
        break;
      case 'h':
      default:
        usage(argv[0]);
        return 0;
    }
  }

  argc -= optind;
  argv += optind;
  if (argc != 0) {
    usage(argv[0]);
    return 1;
  }

  try {
    producer.run();
  }
  catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
  }
  return 0;
}
