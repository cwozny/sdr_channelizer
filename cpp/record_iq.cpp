/* Save to a file, e.g. boilerplate.c, and then compile:
* $ gcc boilerplate.c -o libbladeRF_example_boilerplate -lbladeRF
*/
#include <libbladeRF.h>
#include <cstring>
#include <ctime>

#include <bit>
#include <iostream>
#include <fstream>

const unsigned int sampleLength = 250000;
const unsigned int bufferSize = 2*sampleLength;

struct IqPacket
{
	std::uint32_t endianness;
	std::uint32_t frequencyHz;
	std::uint32_t bandwidthHz;
	std::uint32_t sampleRate;
	std::uint32_t rxGain;
	std::int16_t iq[bufferSize];
};

int main(int argc, char *argv[])
{
	int status;
	bladerf *dev = NULL;
	bladerf_devinfo dev_info;
	bladerf_metadata meta;
	bladerf_channel channel = BLADERF_CHANNEL_RX(0);
	IqPacket packet;
	char datetimeStr[80];
	char filenameStr[80];

	const unsigned int frequencyHz = atof(argv[1])*1e6;
	const unsigned int requestedBandwidthHz = atoi(argv[2])*1e6;
	unsigned int receivedBandwidthHz = 0;
	const unsigned int requestedSampleRate = atoi(argv[3])*1e6;
	unsigned int receivedSampleRate = 0;
	const int rxGain = atoi(argv[4]);

	/* Initialize the information used to identify the desired device
	* to all wildcard (i.e., "any device") values */
	bladerf_init_devinfo(&dev_info);

	status = bladerf_open_with_devinfo(&dev, &dev_info);

	if (status != 0)
	{
		std::cout << "Unable to open device: " << bladerf_strerror(status) << std::endl;
		return 1;
	}

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

	status = bladerf_set_sample_rate(dev, channel, requestedSampleRate, &receivedSampleRate);

	if (status == 0)
	{
		std::cout <<  "Sample Rate = " << receivedSampleRate*1e-6 << " Msps" << std::endl;
	}
	else
	{
		std::cout <<  "Failed to set samplerate = " << requestedSampleRate << ": " << bladerf_strerror(status) << std::endl;
		bladerf_close(dev);
		return 1;
	}

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

	/* These items configure the underlying asynch stream used by the sync
	* interface. The "buffer" here refers to those used internally by worker
	* threads, not the user's sample buffers.
	*
	* It is important to remember that TX buffers will not be submitted to
	* the hardware until `buffer_size` samples are provided via the
	* bladerf_sync_tx call. Similarly, samples will not be available to
	* RX via bladerf_sync_rx() until a block of `buffer_size` samples has been
	* received. */
	const std::uint32_t num_buffers = 16;
	const std::uint32_t buffer_size = 8192; /* Must be a multiple of 1024 */
	const std::uint32_t num_transfers = 8;
	const std::uint32_t timeout_ms = 3500;

	/* Configure both the device's x1 RX and TX channels for use with the
	* synchronous
	* interface. SC16 Q11 samples *without* metadata are used. */
	status = bladerf_sync_config(dev, BLADERF_RX_X1, BLADERF_FORMAT_SC16_Q11, num_buffers, buffer_size, num_transfers, timeout_ms);

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
	
	for(int ii = 0; ii < 10; ii++)
	{
		memset(&packet.iq, 0, bufferSize*sizeof(std::int16_t));
		
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

		packet.frequencyHz = frequencyHz;
		packet.bandwidthHz = receivedBandwidthHz;
		packet.sampleRate = receivedSampleRate;
		packet.rxGain = rxGain;

		status = bladerf_sync_rx(dev, &packet.iq, sampleLength, &meta, 5000);

		if (status == 0)
		{
			std::cout << "Sampling..." << std::endl;
		}
		else
		{
			std::cout << "Failed to sample: " << bladerf_strerror(status) << std::endl;
		}

		// current date/time based on current system
		time_t now = time(0);

		// convert now to UTC time
		tm *utc = gmtime(&now);

		strftime(datetimeStr, sizeof(datetimeStr), "%Y_%m_%d_%H_%M_%S", utc);
		snprintf(filenameStr, sizeof(filenameStr), "%s_%04d.iq", datetimeStr, ii);

		std::ofstream fout(filenameStr);
	    	fout.write((char*)&packet, sizeof(packet));
	    	fout.close();
    	}

	status = bladerf_enable_module(dev, BLADERF_RX, false);

	if (status == 0)
	{
		std::cout << "Disabled RX" << std::endl;
	}
	else
	{
		std::cout << "Failed to disable RX: " << bladerf_strerror(status) << std::endl;
	}

	return status;
}
