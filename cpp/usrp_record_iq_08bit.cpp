#include <uhd/utils/safe_main.hpp>
#include <uhd/usrp/multi_usrp.hpp>

#include "IqPacket.h"
#include "Helper.h"

#include <iostream>
#include <fstream>
#include <chrono>

int UHD_SAFE_MAIN(int argc, char *argv[])
{
  std::int32_t status = EXIT_SUCCESS;
  uhd::rx_metadata_t meta;
  std::string device_args("");
  std::string subdev("A:A");
  std::string ant("RX2");
  std::string ref("internal");
  IqPacket packet;
  char filenameStr[FILENAME_LENGTH];
  std::uint32_t overrunCounter = 0;

  if (argc != 7)
  {
    std::cout << std::endl << "\tUsage:" << std::endl;
    std::cout << "\t\t" << argv[0] << " <freqMhz> <bwMhz> <sampleRateMsps> <gainDb> <dwellSec> <durationSec>" << std::endl;
    std::cout << std::endl;
    return 1;
  }

  const std::uint64_t requestedFrequencyHz = atof(argv[1])*1e6;
  std::uint64_t receivedFrequencyHz = 0;
  const std::uint32_t requestedBandwidthHz = atof(argv[2])*1e6;
  std::uint32_t receivedBandwidthHz = 0;
  const std::uint32_t requestedSampleRate = atof(argv[3])*1e6;
  std::uint32_t receivedSampleRate = 0;
  std::int32_t rxGain = atoi(argv[4]);
  const float dwellDuration = atof(argv[5]);
  const float collectionDuration = atof(argv[6]);

  //create a usrp device

  uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(device_args);

  // Lock mboard clocks
  usrp->set_clock_source(ref);

  //always select the subdevice first, the channel mapping affects the other settings
  usrp->set_rx_subdev_spec(subdev);

  // Save off information about the device being used

  strncpy(packet.userDefined[0], usrp->get_mboard_name().c_str(), sizeof(packet.userDefined[0]));

  std::cout << "Board name: " << packet.userDefined[0] << std::endl;

  uhd::dict<std::string, std::string> rx_info = usrp->get_usrp_rx_info();

  strncpy(packet.userDefined[1], rx_info.get("mboard_serial").c_str(), sizeof(packet.userDefined[1]));

  std::cout << "Serial number: " << packet.userDefined[1] << std::endl;

  // Set center frequency of device

  uhd::tune_request_t tune_request(requestedFrequencyHz);
  usrp->set_rx_freq(tune_request);
  receivedFrequencyHz = usrp->get_rx_freq();

  std::cout << "Frequency = " << receivedFrequencyHz*1e-6 << " MHz" << std::endl;

  // Set sample rate of device

  usrp->set_rx_rate(requestedSampleRate);
  receivedSampleRate = usrp->get_rx_rate();
  std::cout <<  "Sample Rate = " << receivedSampleRate*1e-6 << " Msps" << std::endl;

  // Set analog bandwidth of device

  usrp->set_rx_bandwidth(requestedBandwidthHz);
  receivedBandwidthHz = usrp->get_rx_bandwidth();
  std::cout << "Bandwidth = " << receivedBandwidthHz*1e-6 << " MHz" << std::endl;

  // Disable automatic gain control

  usrp->set_rx_agc(false);

  std::cout << "Disabled automatic gain control" << std::endl;

  // Set gain of the device

  usrp->set_rx_gain(rxGain);
  rxGain = usrp->get_rx_gain();
  std::cout << "Gain = " << rxGain << " dB" << std::endl;

  usrp->set_rx_antenna(ant);

  // Set the time on the device

  const double timeInSecs = std::chrono::system_clock::now().time_since_epoch() / std::chrono::nanoseconds(1) * 1e-9;

  usrp->set_time_now(uhd::time_spec_t(timeInSecs));

  // Set up the configuration parameters necessary to receive samples with the device

  // create a receive streamer
  uhd::stream_args_t stream_args("sc8","sc8"); // 8-bit integers on host, 8-bit integers over-the-wire
  uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);

  // Compute the requested number of samples and buffer size

  const std::uint64_t requested_num_samples = dwellDuration*receivedSampleRate;
  const std::uint64_t bufferSize = 2*requested_num_samples;

  // setup streaming
  uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);

  stream_cmd.num_samps = requested_num_samples;
  stream_cmd.stream_now = true;
  stream_cmd.time_spec  = usrp->get_time_now();

  // Specify the endianness of the recording

  if constexpr (std::endian::native == std::endian::big)
  {
    packet.endianness = 0x00000000;
  }
  else if constexpr (std::endian::native == std::endian::little)
  {
    packet.endianness = 0x01010101;
  }
  else
  {
    packet.endianness = 0xFFFFFFFF;
  }

  // Set information about the recording for data analysis purposes

  packet.frequencyHz = receivedFrequencyHz;
  packet.bandwidthHz = receivedBandwidthHz;
  packet.sampleRate = receivedSampleRate;
  packet.numSamples = requested_num_samples;
  packet.rxGain = rxGain;
  packet.bitWidth = 8; // signed 8-bit integer

  // Allocate the host buffer the device will be streaming to

  std::int8_t* iq = new std::int8_t[bufferSize];

  const std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();
  std::chrono::system_clock::time_point currentTime = startTime;

  while(((currentTime - startTime) / std::chrono::milliseconds(1) * 1e-3) <= collectionDuration)
  {
    meta.reset();

    stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE;

    rx_stream->issue_stream_cmd(stream_cmd);

    packet.numSamples = rx_stream->recv(&iq[0], requested_num_samples, meta, 0.5, false);
    packet.sampleStartTime = meta.time_spec.get_real_secs();

    stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;

    rx_stream->issue_stream_cmd(stream_cmd);

    std::cout << "Received " << packet.numSamples << std::endl;

    // Handle streaming error codes
    switch (meta.error_code)
    {
      case uhd::rx_metadata_t::ERROR_CODE_NONE:
        break;

      case uhd::rx_metadata_t::ERROR_CODE_TIMEOUT:
        std::cout << "ERROR_CODE_TIMEOUT: Got timeout before all samples received" << std::endl;
        break;

      case uhd::rx_metadata_t::ERROR_CODE_OVERFLOW:
        overrunCounter++;
        std::cout << "ERROR_CODE_OVERFLOW: Overflowed" << std::endl;
        break;

      default:
        std::cout << "Got error code: " << meta.strerror() << std::endl;
        break;
    }

    if (packet.numSamples == requested_num_samples)
    {
      getFilenameStr(currentTime, filenameStr, FILENAME_LENGTH);

      std::ofstream fout(filenameStr);
      fout.write((char*)&packet, sizeof(packet));
      fout.write((char*)iq, 2*packet.numSamples*sizeof(std::int8_t));
      fout.close();
    }

    currentTime = std::chrono::system_clock::now();
  }

  std::cout << "There were " << overrunCounter << " overruns." << std::endl;

  // Free up the I/Q buffer we dynamically allocated
  delete [] iq;

  return status;
}
