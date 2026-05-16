#include <libbladeRF.h>

#include "IqPacket.h"
#include "Helper.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include <bit>
#include <iostream>
#include <fstream>
#include <chrono>
#include <complex>

int main(const int argc, const char *argv[])
{
  std::int32_t status;
  bladerf *dev = NULL;
  bladerf_devinfo dev_info;
  bladerf_metadata meta;
  bladerf_channel channel = BLADERF_CHANNEL_RX(0);
  struct bladerf_version version;
  bladerf_serial serNo;
  bladerf_timestamp startTimeTicks = 0;
  IqPacket packet;
  char filenameStr[FILENAME_LENGTH];
  std::uint32_t overrunCounter = 0;

  // AGC tunables. Any I or Q sample with |sample| >= SATURATION_THRESHOLD in the kept
  // portion of a buffer triggers a GAIN_STEP_DOWN_DB cut and resets the clean-buffer counter.
  // CLIMB_AFTER_N_CLEAN consecutive saturation-free buffers trigger a GAIN_STEP_UP_DB climb.
  // SC8_Q7 samples saturate at +/-127 — note this is unlike the USRP sc8 host format.
  const std::int8_t SATURATION_THRESHOLD = static_cast<std::int8_t>(0.98 * 127);
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
  const std::uint32_t requestedSampleRate = atof(argv[3])*1e6;
  std::uint32_t receivedSampleRate = 0;
  const float requestedStartGainDb = atof(argv[4]);
  const float dwellDuration = atof(argv[5]);
  const float collectionDuration = atof(argv[6]);
  const std::int32_t FILTER_DELAY = atoi(argv[7]); // Number of initial zero'd samples induced by filter delay

  /* Initialize the information used to identify the desired device
   * to all wildcard (i.e., "any device") values */
  bladerf_init_devinfo(&dev_info);

  status = bladerf_open_with_devinfo(&dev, &dev_info);

  if (status != 0)
  {
    std::cout << "Unable to open device: " << bladerf_strerror(status) << std::endl;
    return __LINE__;
  }

  packet.linkSpeed = bladerf_device_speed(dev);

  if ( packet.linkSpeed == BLADERF_DEVICE_SPEED_SUPER )
  {
    std::cout << "Negotiated USB 3 link speed" << std::endl;
  }
  else if ( packet.linkSpeed == BLADERF_DEVICE_SPEED_HIGH )
  {
    std::cout << "Negotiated USB 2 link speed" << std::endl;
  }
  else
  {
    std::cout << "Negotiated unknown link speed" << std::endl;
  }

  // Save off information about the device being used

  bladerf_get_serial_struct(dev, &serNo);

  const std::string boardName = bladerf_get_board_name(dev);
  strncpy(packet.boardName, boardName.c_str(), sizeof(packet.boardName) - 1);
  std::cout << "Board Name: " << packet.boardName << std::endl;

  const std::string serialNumber = serNo.serial;
  strncpy(packet.serialNumber, serialNumber.c_str(), sizeof(packet.serialNumber) - 1);
  std::cout << "Serial Number: " << packet.serialNumber << std::endl;

  bladerf_fpga_version(dev, &version);

  const std::string fpgaVersion = version.describe;
  strncpy(packet.fpgaVersion, fpgaVersion.c_str(), sizeof(packet.fpgaVersion) - 1);
  std::cout << "FPGA Version: " << packet.fpgaVersion << std::endl;

  bladerf_fw_version(dev, &version);

  const std::string fwVersion = version.describe;
  strncpy(packet.fwVersion, fwVersion.c_str(), sizeof(packet.fwVersion) - 1);
  std::cout << "FW Version: " << packet.fwVersion << std::endl;

  // Set relevant features of device

  status = bladerf_enable_feature(dev, BLADERF_FEATURE_DEFAULT, true);

  if (status == 0)
  {
    std::cout << "Feature = DEFAULT" << std::endl;
  }
  else
  {
    std::cout << "Failed to set feature = DEFAULT: " << bladerf_strerror(status) << std::endl;
    bladerf_close(dev);
    return __LINE__;
  }

  // Set center frequency of device

  status = bladerf_set_frequency(dev, channel, requestedFrequencyHz);
  status = bladerf_get_frequency(dev, channel, &receivedFrequencyHz);

  if (status == 0)
  {
    std::cout << "Frequency = " << receivedFrequencyHz*1e-6 << " MHz" << std::endl;
  }
  else
  {
    std::cout << "Failed to set frequency = " << requestedFrequencyHz << ": " << bladerf_strerror(status) << std::endl;
    bladerf_close(dev);
    return __LINE__;
  }

  // Set sample rate of device

  status = bladerf_set_sample_rate(dev, channel, requestedSampleRate, &receivedSampleRate);

  if (status == 0)
  {
    std::cout << "Sample Rate = " << receivedSampleRate*1e-6 << " Msps" << std::endl;
  }
  else
  {
    std::cout << "Failed to set sample rate = " << requestedSampleRate << ": " << bladerf_strerror(status) << std::endl;
    bladerf_close(dev);
    return __LINE__;
  }

  // Set analog bandwidth of device

  status = bladerf_set_bandwidth(dev, channel, requestedBandwidthHz, &receivedBandwidthHz);

  if (status == 0)
  {
    std::cout << "Bandwidth = " << receivedBandwidthHz*1e-6 << " MHz" << std::endl;
  }
  else
  {
    std::cout << "Failed to set bandwidth = " << requestedBandwidthHz << ": " << bladerf_strerror(status) << std::endl;
    bladerf_close(dev);
    return __LINE__;
  }

  // Disable automatic gain control

  status = bladerf_set_gain_mode(dev, channel, BLADERF_GAIN_MGC);

  if (status == 0)
  {
    std::cout << "Disabled automatic gain control" << std::endl;
  }
  else
  {
    std::cout << "Failed to disable automatic gain control: " << bladerf_strerror(status) << std::endl;
    bladerf_close(dev);
    return __LINE__;
  }

  // Query supported gain range, clamp the starting gain to it, and apply.
  // targetGainDb is the AGC accumulator; bladerf_set_gain takes integer dB, so the
  // accumulator is rounded at the API boundary — sub-dB targets accumulate until
  // they cross an integer threshold.

  const struct bladerf_range *gainRangeRaw = NULL;
  status = bladerf_get_gain_range(dev, channel, &gainRangeRaw);
  if (status != 0)
  {
    std::cout << "Failed to read gain range: " << bladerf_strerror(status) << std::endl;
    bladerf_close(dev);
    return __LINE__;
  }
  const float minGainDb = static_cast<float>(gainRangeRaw->min) * gainRangeRaw->scale;
  const float maxGainDb = static_cast<float>(gainRangeRaw->max) * gainRangeRaw->scale;
  float targetGainDb = std::min(maxGainDb, std::max(minGainDb, requestedStartGainDb));

  status = bladerf_set_gain(dev, channel, static_cast<bladerf_gain>(std::round(targetGainDb)));
  if (status != 0)
  {
    std::cout << "Failed to set starting gain: " << bladerf_strerror(status) << std::endl;
    bladerf_close(dev);
    return __LINE__;
  }

  bladerf_gain actualGain = 0;
  bladerf_get_gain(dev, channel, &actualGain);
  std::cout << "Gain range = [" << minGainDb << ", " << maxGainDb << "] dB, start = " << actualGain << " dB" << std::endl;

  // Compute the requested number of samples and buffer size

  const std::uint64_t requested_num_samples = dwellDuration*receivedSampleRate + FILTER_DELAY;

  // Set up the configuration parameters necessary to receive samples with the device

  /* These items configure the underlying asynch stream used by the sync
   * interface. The "buffer" here refers to those used internally by worker
   * threads, not the user's sample buffers.
   *
   * It is important to remember that TX buffers will not be submitted to
   * the hardware until `buffer_size` samples are provided via the
   * bladerf_sync_tx call. Similarly, samples will not be available to
   * RX via bladerf_sync_rx() until a block of `buffer_size` samples has been
   * received. */
  const std::uint32_t num_buffers = 4;
  const std::uint32_t buffer_size = 1024 * 1024; /* Must be a multiple of 1024 */
  const std::uint32_t num_transfers = 2;
  const std::uint32_t timeout_ms = 3500;

  // Configure both the device's x1 RX and TX channels for use with the synchronous interface.

  status = bladerf_sync_config(dev, BLADERF_RX_X1, BLADERF_FORMAT_SC8_Q7_META, num_buffers, buffer_size, num_transfers, timeout_ms);

  if (status == 0)
  {
    std::cout << "Configured RX sync interface" << std::endl;
  }
  else
  {
    std::cout << "Failed to configure RX sync interface: " << bladerf_strerror(status) << std::endl;
    bladerf_close(dev);
    return __LINE__;
  }

  status = bladerf_enable_module(dev, BLADERF_RX, true);

  if (status == 0)
  {
    std::cout << "Enabled RX" << std::endl;
  }
  else
  {
    std::cout << "Failed to enable RX: " << bladerf_strerror(status) << std::endl;
    bladerf_close(dev);
    return __LINE__;
  }

  // Specify the endianness of the recording

  if constexpr (std::endian::native == std::endian::big)
  {
    packet.endianness = 0x00000000;
  }
  else if constexpr (std::endian::native == std::endian::little)
  {
    packet.endianness = 0x02020202;
  }
  else
  {
    packet.endianness = 0xFFFFFFFF;
  }

  // Set information about the recording for data analysis purposes

  packet.frequencyHz = receivedFrequencyHz;
  packet.bandwidthHz = receivedBandwidthHz;
  packet.sampleRateSps = receivedSampleRate;
  packet.bitWidth = 8; // signed 8-bit integer

  // Precompute the filter delay in seconds
  const std::double_t filterDelaySecs = FILTER_DELAY*1.0/packet.sampleRateSps;

  // Allocate the host buffer the device will be streaming to

  std::complex<std::int8_t>* iq = new std::complex<std::int8_t>[requested_num_samples];

  std::uint32_t cleanBufferCount = 0;

  const std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();
  const std::double_t startTimeSecs = startTime.time_since_epoch() / std::chrono::nanoseconds(1) * 1e-9;
  std::chrono::system_clock::time_point currentTime = startTime;

  status = bladerf_get_timestamp(dev, BLADERF_RX, &startTimeTicks);

  if (status == 0)
  {
    std::cout << "Retrieved device timestamp (in clock ticks): " << startTimeTicks << std::endl;
  }
  else
  {
    std::cout << "Failed to get timestamp: " << bladerf_strerror(status) << std::endl;
    bladerf_close(dev);
    return __LINE__;
  }

  while(((currentTime - startTime) / std::chrono::milliseconds(1) * 1e-3) <= collectionDuration)
  {
    std::memset(&meta, 0, sizeof(meta));
    meta.flags = BLADERF_META_FLAG_RX_NOW;

    currentTime = std::chrono::system_clock::now();

    // Capture the device's actual gain for this dwell. The header carries this value
    // so MATLAB can attribute saturated/clean buffers to the gain in effect.
    bladerf_get_gain(dev, channel, &actualGain);
    packet.rxGainDb = static_cast<float>(actualGain);

    const std::double_t relativeSampleTimeSecs = (meta.timestamp - startTimeTicks) * 1.0 / packet.sampleRateSps;

    packet.sampleStartTime = startTimeSecs + relativeSampleTimeSecs + filterDelaySecs;

    status = bladerf_sync_rx(dev, iq, requested_num_samples, &meta, 5000);

    if (status != 0)
    {
      std::cout << "RX \"now\" failed: " << bladerf_strerror(status) << std::endl;
    }
    else if (meta.status & BLADERF_META_STATUS_OVERRUN)
    {
      std::cout << "Overrun detected. " << meta.actual_count << " valid samples were read." << std::endl;
      overrunCounter++;
    }
    else
    {
      std::cout << "Received " << meta.actual_count << " @ " << packet.rxGainDb << " dB" << std::endl;
    }

    packet.numSamples = meta.actual_count - FILTER_DELAY;

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
      fout.write((const char*)&iq[FILTER_DELAY], (requested_num_samples-FILTER_DELAY)*sizeof(std::complex<std::int8_t>));
      fout.close();

      // Adjust gain target for the NEXT dwell. The accumulator is float so sub-dB
      // steps accumulate; the bladeRF API only accepts integer dB so we round at the call.
      if (saturated)
      {
        targetGainDb = std::max(minGainDb, targetGainDb - GAIN_STEP_DOWN_DB);
        bladerf_set_gain(dev, channel, static_cast<bladerf_gain>(std::round(targetGainDb)));
        cleanBufferCount = 0;
        std::cout << "  saturated -> target " << targetGainDb << " dB" << std::endl;
      }
      else
      {
        cleanBufferCount++;
        if (cleanBufferCount >= CLIMB_AFTER_N_CLEAN)
        {
          targetGainDb = std::min(maxGainDb, targetGainDb + GAIN_STEP_UP_DB);
          bladerf_set_gain(dev, channel, static_cast<bladerf_gain>(std::round(targetGainDb)));
          cleanBufferCount = 0;
          std::cout << "  " << CLIMB_AFTER_N_CLEAN << " clean -> target " << targetGainDb << " dB" << std::endl;
        }
      }
    }
  }

  // Disable the device

  status = bladerf_enable_module(dev, BLADERF_RX, false);

  if (status == 0)
  {
    std::cout << "Disabled RX" << std::endl;
  }
  else
  {
    std::cout << "Failed to disable RX: " << bladerf_strerror(status) << std::endl;
  }

  std::cout << "There were " << overrunCounter << " overruns." << std::endl;

  // Free up the I/Q buffer we dynamically allocated
  delete [] iq;

  return status;
}
