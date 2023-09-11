#ifndef IqPacket_H
#define IqPacket_H

#include <cstdint>

struct IqPacket
{
  std::uint32_t endianness;
  std::uint32_t linkSpeed;
  std::uint32_t frequencyHz;
  std::uint32_t bandwidthHz;
  std::uint32_t sampleRate;
  std::uint32_t rxGain;
  std::uint32_t numSamples;
  std::uint32_t bitWidth;
  char fpgaVersion[32];
  char fwVersion[32];
  double sampleStartTime;
};

#endif
