#ifndef IqPacket_H
#define IqPacket_H

#include <cstdint>

#define IQ_FILE_FORMAT 2

struct IqPacket
{
  std::uint32_t endianness;
  std::uint32_t linkSpeed;
  std::uint64_t frequencyHz;
  std::uint32_t bandwidthHz;
  std::uint32_t sampleRate;
  std::uint32_t rxGain;
  std::uint32_t numSamples;
  std::uint32_t bitWidth;
  std::uint32_t spare0;
  char boardName[16];
  char serialNumber[16];
  char fpgaVersion[16];
  char fwVersion[16];
  double sampleStartTime;
};

#endif
