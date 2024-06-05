/* Save to a file, e.g. boilerplate.c, and then compile:
 * $ gcc boilerplate.c -o libbladeRF_example_boilerplate -lbladeRF
 */
#include <libbladeRF.h>

#include <cstring>

#include <iostream>
#include <chrono>

int main(const int argc, const char *argv[])
{
  std::int32_t status;
  bladerf *dev = NULL;
  bladerf_devinfo dev_info;
  bladerf_metadata meta;
  bladerf_channel channel = BLADERF_CHANNEL_RX(0);
  struct bladerf_version version;
  bladerf_serial serNo;
  const std::int8_t SAMP_MAX = 127;
  const std::int8_t SAMP_MIN = -128;
  bool saturated = false;
  std::uint32_t overrunCounter = 0;

  if (argc != 7)
  {
    std::cout << std::endl << "\tUsage:" << std::endl;
    std::cout << "\t\t./blade_find_max_gain.out <freqMhz> <bwMhz> <sampleRateMsps> <gainDb> <dwellSec> <durationSec>" << std::endl;
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

  /* Initialize the information used to identify the desired device
   * to all wildcard (i.e., "any device") values */
  bladerf_init_devinfo(&dev_info);

  status = bladerf_open_with_devinfo(&dev, &dev_info);

  if (status != 0)
  {
    std::cout << "Unable to open device: " << bladerf_strerror(status) << std::endl;
    return 1;
  }

  switch (bladerf_device_speed(dev))
  {
    case BLADERF_DEVICE_SPEED_SUPER:
      std::cout << "Negotiated USB 3 link speed" << std::endl;
      break;
    case BLADERF_DEVICE_SPEED_HIGH:
      std::cout << "Negotiated USB 2 link speed" << std::endl;
      break;
    default:
      std::cout << "Negotiated unknown link speed" << std::endl;
      break;
  }

  bladerf_fpga_version(dev, &version);

  std::cout << "FPGA version: " << version.describe << std::endl;

  bladerf_fw_version(dev, &version);

  std::cout << "Firmware version: " << version.describe << std::endl;

  bladerf_get_serial_struct(dev, &serNo);

  std::cout << "Using " << bladerf_get_board_name(dev) << " serial number " << serNo.serial << std::endl;

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
    return 1;
  }

  // Set center frequency of device

  status = bladerf_set_frequency(dev, channel, frequencyHz);

  if (status == 0)
  {
    std::cout << "Frequency = " << frequencyHz*1e-6 << " MHz" << std::endl;
  }
  else
  {
    std::cout << "Failed to set frequency = " << frequencyHz << ": " << bladerf_strerror(status) << std::endl;
    bladerf_close(dev);
    return 1;
  }

  // Set sample rate of device

  status = bladerf_set_sample_rate(dev, channel, requestedSampleRate, &receivedSampleRate);

  if (status == 0)
  {
    std::cout <<  "Sample Rate = " << receivedSampleRate*1e-6 << " Msps" << std::endl;
  }
  else
  {
    std::cout <<  "Failed to set sample rate = " << requestedSampleRate << ": " << bladerf_strerror(status) << std::endl;
    bladerf_close(dev);
    return 1;
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
    return 1;
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
    return 1;
  }

  // Set gain of the device

  status = bladerf_set_gain(dev, channel, rxGain);

  if (status == 0)
  {
    std::cout << "Gain = " << rxGain << " dB" << std::endl;
  }
  else
  {
    std::cout << "Failed to set gain: " << bladerf_strerror(status) << std::endl;
    bladerf_close(dev);
    return 1;
  }

  // Compute the requested number of samples and buffer size

  const std::uint64_t requested_num_samples = dwellDuration*receivedSampleRate;
  const std::uint64_t bufferSize = 2*requested_num_samples;

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
    return 1;
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
  }

  // Allocate the host buffer the device will be streaming to

  std::int8_t* iq = new std::int8_t[bufferSize];

  const std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();
  std::chrono::system_clock::time_point currentTime;

  do
  {
    // If we're saturated, then drop the receive gain down by 1 dB
    if (saturated)
    {
      status = bladerf_set_gain(dev, channel, --rxGain);

      if (status == 0)
      {
        std::cout << "Gain = " << rxGain << " dB" << std::endl;
      }
      else
      {
        std::cout << "Failed to set gain: " << bladerf_strerror(status) << std::endl;
        bladerf_close(dev);
        return 1;
      }
    }

    saturated = false;

    std::memset(&meta, 0, sizeof(meta));
    meta.flags = BLADERF_META_FLAG_RX_NOW;

    currentTime = std::chrono::system_clock::now();

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
      std::cout << "Gain = " << rxGain << " dB" << std::endl;
      std::cout << "Received " << meta.actual_count << std::endl;

      for (std::uint64_t ii = 0; ii < bufferSize; ii++)
      {
          if (iq[ii] <= (0.98 * SAMP_MIN) || (0.98 * SAMP_MAX) <= iq[ii])
          {
              std::cout << "Saturated sample at " << iq[ii] << std::endl;
              saturated = true;
              break;
          }
      }
    }
  }
  while(((currentTime - startTime) / std::chrono::milliseconds(1) * 1e-3) <= collectionDuration);

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
