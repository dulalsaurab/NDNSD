#include <nac-abe/attribute-authority.hpp>
#include <nac-abe/cache-producer.hpp>

#include <ndn-cxx/util/dummy-client-face.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/util/scheduler.hpp>

#include <chrono>
#include <unistd.h>

class ABEHelper
{
public:
  ABEHelper(const ndn::Name& aaPrefix)
  : m_aaCert(m_keyChain.getPib().getIdentity(aaPrefix).getDefaultKey().getDefaultCertificate())
  , m_attrAuthority(m_aaCert, m_face, m_keyChain)
  {
  }
  
  ndn::nacabe::KpAttributeAuthority&
  getAttrAuthority()
  {
    return m_attrAuthority;
  }

  void
  run()
  {
    m_face.processEvents();
  }

private:
  ndn::Face m_face;
  ndn::security::KeyChain m_keyChain;
  ndn::Name aaPrefix;
  ndn::security::Certificate m_aaCert;
  ndn::nacabe::KpAttributeAuthority m_attrAuthority;
};


int main ()
{
 ABEHelper abe("/ndnsd/aa");
 abe.run();
}
