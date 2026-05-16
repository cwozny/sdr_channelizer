#include <uhd/utils/safe_main.hpp>
#include <uhd/usrp/multi_usrp.hpp>

#include "IqPacket.h"
#include "Helper.h"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <chrono>
#include <complex>

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

  // AGC tunables. Any I or Q sample with |sample| >= SATURATION_THRESHOLD in the kept
  // portion of a buffer triggers a GAIN_STEP_DOWN_DB cut and resets the clean-buffer counter.
  // CLIMB_AFTER_N_CLEAN consecutive saturation-free buffers trigger a GAIN_STEP_UP_DB climb.
  const std::int16_t SATURATION_THRESHOLD = static_cast<std::int16_t>(0.98 * 32767);
  const float GAIN_STEP_DOWN_DB = 0.5;
  const float GAIN_STEP_UP_DB = 0.25;
  const std::uint32_t CLIMB_AFTER_N_CLEAN = 10;

  if (argc != 8)
  {
    std::cout << std::endl << "\tUsage:" << std::endl;
    std::cout << "\t\t" << argv[0] << " <freqMhz> <bwMhz> <sampleRateMsps> <startGainDb> <dwellSec> <durationSec> <filter delay>" << std::endl;
    std::cout << std::endl;
    return __LINE__;
  }

  const std::uint64_t requestedFrequencyHz = atof(argv[1])*1e6;
  std::uint64_t receivedFrequencyHz = 0;
  const std::uint32_t requestedBandwidthHz = atof(argv[2])*1e6;
  std::uint32_t receivedBandwidthHz = 0;
  const std::uint32_t requestedSampleRateSps = atof(argv[3])*1e6;
  std::uint32_t receivedSampleRateSps = 0;
  const float requestedStartGainDb = atof(argv[4]);
  const float dwellDurationSec = atof(argv[5]);
  const float collectionDurationSec = atof(argv[6]);
  const std::int32_t FILTER_DELAY = atoi(argv[7]);

  //create a usrp device

  uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(device_args);

  // Save off information about the device being used

  const std::string boardName = usrp->get_mboard_name();
  strncpy(packet.boardName, boardName.c_str(), sizeof(packet.boardName) - 1);
  std::cout << "Board Name: " << packet.boardName << std::endl;

  uhd::dict<std::string, std::string> rx_info = usrp->get_usrp_rx_info();

  const std::string serialNumber = rx_info.get("mboard_serial");
  strncpy(packet.serialNumber, serialNumber.c_str(), sizeof(packet.serialNumber) - 1);
  std::cout << "Serial Number: " << packet.serialNumber << std::endl;

  uhd::device::sptr dev = usrp->get_device();
  uhd::property_tree::sptr tree = dev->get_tree();
  const uhd::fs_path& path = "/mboards/0/";

  const std::string fpgaVersion = tree->access<std::string>(path / "fpga_version").get();
  strncpy(packet.fpgaVersion, fpgaVersion.c_str(), sizeof(packet.fpgaVersion) - 1);
  std::cout << "FPGA Version: " << packet.fpgaVersion << std::endl;

  const std::string fwVersion = tree->access<std::string>(path / "fw_version").get();
  strncpy(packet.fwVersion, fwVersion.c_str(), sizeof(packet.fwVersion) - 1);
  std::cout << "FW Version: " << packet.fwVersion << std::endl;

  // Lock mboard clocks
  usrp->set_clock_source(ref);

  //always select the subdevice first, the channel mapping affects the other settings
  usrp->set_rx_subdev_spec(subdev);

  std::cout << "Number of RX channels: " << usrp->get_rx_num_channels() << std::endl;

  // Set the time on the device

  const double timeInSecs = std::chrono::system_clock::now().time_since_epoch() / std::chrono::nanoseconds(1) * 1e-9;

  usrp->set_time_now(uhd::time_spec_t(timeInSecs));

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Set up the configuration parameters necessary to receive samples with the device

  // create a receive streamer
  uhd::stream_args_t stream_args("sc16","sc12"); // 16-bit integers on host, 12-bit integers over-the-wire
  uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);

  // Set sample rate of device

  usrp->set_rx_rate(requestedSampleRateSps);
  receivedSampleRateSps = usrp->get_rx_rate();
  std::cout << "Sample Rate = " << receivedSampleRateSps*1e-6 << " Msps\n";

  // Set analog bandwidth of device

  usrp->set_rx_bandwidth(requestedBandwidthHz);
  receivedBandwidthHz = usrp->get_rx_bandwidth();
  std::cout << "Bandwidth = " << receivedBandwidthHz*1e-6 << " MHz\n";

  // Disable automatic gain control

  usrp->set_rx_agc(false);

  std::cout << "Disabled automatic gain control" << std::endl;

  // Query the device's supported gain range and clamp the starting gain into it.
  // targetGainDb is the AGC accumulator; the file header carries the gain the device
  // actually applied (read back via get_rx_gain) which may differ due to quantization.

  const uhd::gain_range_t gainRange = usrp->get_rx_gain_range();
  const float minGainDb = gainRange.start();
  const float maxGainDb = gainRange.stop();
  float targetGainDb = std::min(maxGainDb, std::max(minGainDb, requestedStartGainDb));

  usrp->set_rx_gain(targetGainDb);
  std::cout << "Gain range = [" << minGainDb << ", " << maxGainDb << "] dB, start = " << usrp->get_rx_gain() << " dB" << std::endl;

  usrp->set_rx_antenna(ant);
  std::cout << "Antenna = " << usrp->get_rx_antenna() << "\n";

  std::cout << std::endl;

  usrp->clear_command_time();

  usrp->set_command_time(usrp->get_time_now() + uhd::time_spec_t(0.1)); //set cmd time for .1s in the future

  // Set center frequency of device
  uhd::tune_request_t tune_request(requestedFrequencyHz);

   usrp->set_rx_freq(tune_request);
  std::this_thread::sleep_for(std::chrono::milliseconds(110)); //sleep 110ms (~10ms after retune occurs) to allow LO to lock

  usrp->clear_command_time();

  // Get the frequency we're tuned to in case it differs from the one we requested
  receivedFrequencyHz = usrp->get_rx_freq();

  std::cout << "Frequency = " << receivedFrequencyHz*1e-6 << " MHz" << std::endl;

  // Compute the requested number of samples and buffer size

  const std::uint64_t requested_num_samples = dwellDurationSec*receivedSampleRateSps + FILTER_DELAY;

  // setup streaming
  uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE); // Give us the number of samples we want and then finish

  stream_cmd.num_samps  = requested_num_samples;
  stream_cmd.stream_now = false;
  stream_cmd.time_spec  = uhd::time_spec_t(0.0);

  // Specify the endianness of the recording

  if constexpr (std::endian::native == std::endian::big)
  {
    packet.endianness = 0x00000000;
  }
  else if constexpr (std::endian::native == std::endian::little)
  {
    packet.endianness = 0x03030303;
  }
  else
  {
    packet.endianness = 0xFFFFFFFF;
  }

  // Set information about the recording for data analysis purposes

  packet.frequencyHz = receivedFrequencyHz;
  packet.bandwidthHz = receivedBandwidthHz;
  packet.sampleRateSps = receivedSampleRateSps;
  packet.bitWidth = 16; // signed 16-bit integer

  // Precompute the filter delay in seconds
  const std::double_t filterDelaySecs = FILTER_DELAY*1.0/packet.sampleRateSps;

  // Allocate the host buffer the device will be streaming to

  std::complex<std::int16_t>* iq = new std::complex<std::int16_t>[requested_num_samples];

  std::uint32_t cleanBufferCount = 0;

  const std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();
  std::chrono::system_clock::time_point currentTime = startTime;

  while(((currentTime - startTime) / std::chrono::milliseconds(1) * 1e-3) <= collectionDurationSec)
  {
    meta.reset();

    stream_cmd.time_spec = uhd::time_spec_t(usrp->get_time_now().get_real_secs() + 100e-3);

    // Capture the device's actual gain for this dwell. The header carries this value
    // so MATLAB can attribute saturated/clean buffers to the gain in effect.
    packet.rxGainDb = usrp->get_rx_gain();

    // Issue the command to get the samples we requested
    rx_stream->issue_stream_cmd(stream_cmd);

    // Block until all of the samples are received
    packet.numSamples = rx_stream->recv(iq, requested_num_samples, meta, dwellDurationSec + 500e-3);
    packet.numSamples -= FILTER_DELAY;
    packet.sampleStartTime = meta.time_spec.get_real_secs() + filterDelaySecs;

    std::cout << "Received " << packet.numSamples << " @ " << packet.rxGainDb << " dB" << std::endl;

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

    if (packet.numSamples == (requested_num_samples - FILTER_DELAY))
    {
      // Saturation check over the same range we're about to write to disk
      bool saturated = false;
      for (std::uint64_t ii = FILTER_DELAY; ii < requested_num_samples; ii++)
      {
        if (iq[ii].real() >=  SATURATION_THRESHOLD || iq[ii].real() <= -SATURATION_THRESHOLD ||
            iq[ii].imag() >=  SATURATION_THRESHOLD || iq[ii].imag() <= -SATURATION_THRESHOLD)
        {
          saturated = true;
          break;
        }
      }

      getFilenameStr(currentTime, filenameStr, FILENAME_LENGTH);

      std::ofstream fout(filenameStr, std::ofstream::binary);
      fout.write((const char*)&packet, sizeof(packet));
      fout.write((const char*)&iq[FILTER_DELAY], (requested_num_samples-FILTER_DELAY)*sizeof(std::complex<std::int16_t>));
      fout.close();

      // Adjust gain target for the NEXT dwell. Track an internal target separate from
      // the device readback so that sub-quantum step accumulate when the hardware
      // doesn't resolve the requested step (e.g. ask for +0.25 dB on a 1-dB-step device).
      if (saturated)
      {
        targetGainDb = std::max(minGainDb, targetGainDb - GAIN_STEP_DOWN_DB);
        usrp->set_rx_gain(targetGainDb);
        cleanBufferCount = 0;
        std::cout << "  saturated -> target " << targetGainDb << " dB" << std::endl;
      }
      else
      {
        cleanBufferCount++;
        if (cleanBufferCount >= CLIMB_AFTER_N_CLEAN)
        {
          targetGainDb = std::min(maxGainDb, targetGainDb + GAIN_STEP_UP_DB);
          usrp->set_rx_gain(targetGainDb);
          cleanBufferCount = 0;
          std::cout << "  " << CLIMB_AFTER_N_CLEAN << " clean -> target " << targetGainDb << " dB" << std::endl;
        }
      }
    }

    currentTime = std::chrono::system_clock::now();
  }

  std::cout << "There were " << overrunCounter << " overruns." << std::endl;

  // Free up the I/Q buffer we dynamically allocated
  delete [] iq;

  return status;
}
