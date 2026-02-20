#ifndef IqPacket_H
#define IqPacket_H

#include <cstdint>
#include <cmath>

#define IQ_FILE_FORMAT 3

struct IqPacket
{
  std::uint32_t endianness;
  std::uint32_t linkSpeed;
  std::uint64_t frequencyHz;
  std::uint32_t bandwidthHz;
  std::uint32_t sampleRateSps;
  std::float_t rxGainDb;
  std::uint32_t numSamples;
  std::uint32_t bitWidth;
  std::uint32_t spare0;
  char boardName[16];
  char serialNumber[16];
  char fpgaVersion[16];
  char fwVersion[16];
  std::double_t sampleStartTime;
};

#endif
