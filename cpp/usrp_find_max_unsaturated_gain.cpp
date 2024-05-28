#include <uhd/utils/thread.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/exception.hpp>
#include <uhd/types/tune_request.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>

#include "Helper.h"

#include <cstring>

#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>

int UHD_SAFE_MAIN(int argc, char *argv[])
{
  std::int32_t status = EXIT_SUCCESS;
  uhd::rx_metadata_t meta;
  std::string device_args("");
  std::string subdev("A:A");
  std::string ant("RX2");
  std::string ref("internal");
  const std::int16_t SAMP_MAX = 32767;
  bool saturated = false;

  if (argc != 7)
  {
    std::cout << std::endl << "\tUsage:" << std::endl;
    std::cout << "\t\t" << argv[0] << " <freqMhz> <bwMhz> <sampleRateMsps> <gainDb> <dwellSec> <durationSec>" << std::endl;
    std::cout << std::endl;
    return 1;
  }

  const std::uint64_t frequencyHz = atof(argv[1])*1e6;
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

  std::cout << "Board name: " << usrp->get_mboard_name() << std::endl;

  uhd::dict<std::string, std::string> rx_info = usrp->get_usrp_rx_info();

  std::cout << "Serial number: " << rx_info.get("mboard_serial") << std::endl;

  // Set center frequency of device

  uhd::tune_request_t tune_request(frequencyHz);
  usrp->set_rx_freq(tune_request);

  std::cout << "Frequency = " << frequencyHz*1e-6 << " MHz" << std::endl;

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
  uhd::stream_args_t stream_args("sc16","sc12"); // 16-bit integers on host, 12-bit integers over-the-wire
  uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);

  // Compute the requested number of samples and buffer size

  const std::uint64_t requested_num_samples = dwellDuration*receivedSampleRate;
  const std::uint64_t bufferSize = 2*requested_num_samples;

  // setup streaming
  uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);

  stream_cmd.num_samps = requested_num_samples;
  stream_cmd.stream_now = true;
  stream_cmd.time_spec  = usrp->get_time_now();

  // Allocate the host buffer the device will be streaming to

  std::vector<std::int16_t> iq_vec;
  iq_vec.resize(bufferSize, 0);
  std::int16_t* iq = iq_vec.data();

  const std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();
  std::chrono::system_clock::time_point currentTime;

  do
  {
    // If we're saturated, then drop the receive gain down by 1 dB
    if (saturated == true)
    {
      usrp->set_rx_gain(--rxGain);
      rxGain = usrp->get_rx_gain();
    }

    saturated = false;

    stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE;

    rx_stream->issue_stream_cmd(stream_cmd);

    const std::uint64_t received_num_samples = rx_stream->recv(&iq[0], requested_num_samples, meta, 0.5, false);

    stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;

    rx_stream->issue_stream_cmd(stream_cmd);

    if (meta.error_code == uhd::rx_metadata_t::ERROR_CODE_NONE && requested_num_samples == received_num_samples)
    {
        std::cout << "Gain = " << rxGain << " dB" << std::endl;
        std::cout << "Received " << received_num_samples << std::endl;

        for (std::uint64_t ii = 0; ii < bufferSize; ii++)
        {
            if (iq[ii] >= (0.95 * SAMP_MAX))
            {
                std::cout << iq[ii] << std::endl;
                saturated = true;
                break;
            }

            if(iq[ii] <= (-0.95 * SAMP_MAX))
            {
                std::cout << iq[ii] << std::endl;
                saturated = true;
                break;
            }
        }
    }

    currentTime = std::chrono::system_clock::now();
  }
  while(((currentTime - startTime) / std::chrono::milliseconds(1) * 1e-3) <= collectionDuration);

  return status;
}
