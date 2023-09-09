/* Save to a file, e.g. boilerplate.c, and then compile:
* $ gcc boilerplate.c -o libbladeRF_example_boilerplate -lbladeRF
*/
#include <libbladeRF.h>
#include <cstring>

#include <iostream>

int main(int argc, char *argv[])
{
	int status;
	struct bladerf *dev = NULL;
	struct bladerf_devinfo dev_info;
	bladerf_channel channel = BLADERF_CHANNEL_RX(0);

	unsigned int frequencyHz = atof(argv[1])*1e6;
	unsigned int bandwidthHz = atoi(argv[2])*1e6;
	unsigned int sampleRate = atoi(argv[3])*1e6;
	int rxGain = atoi(argv[4]);
	unsigned int sampleLength = 2500000;
	unsigned int bufferSize = 2*sampleLength;

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

	status = bladerf_set_sample_rate(dev, channel, sampleRate, NULL);

	if (status == 0)
	{
		std::cout <<  "Sample Rate = " << sampleRate*1e-6 << " Msps" << std::endl;
	}
	else
	{
		std::cout <<  "Failed to set samplerate = " << sampleRate << ": " << bladerf_strerror(status) << std::endl;
		bladerf_close(dev);
		return 1;
	}

	status = bladerf_set_bandwidth(dev, channel, bandwidthHz, NULL);

	if (status == 0)
	{
		std::cout << "Bandwidth = " << bandwidthHz*1e-6 << " MHz" << std::endl;
	}
	else
	{
		std::cout << "Failed to set bandwidth = " << bandwidthHz << ": " << bladerf_strerror(status) << std::endl;
		bladerf_close(dev);
		return 1;
	}

	// Disable automatic gain control
	status = bladerf_set_gain_mode(dev, channel, BLADERF_GAIN_MGC);

	if (status != 0)
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
	const unsigned int num_buffers = 16;
	const unsigned int buffer_size = 8192; /* Must be a multiple of 1024 */
	const unsigned int num_transfers = 8;
	const unsigned int timeout_ms = 3500;

	/* Configure both the device's x1 RX and TX channels for use with the
	* synchronous
	* interface. SC16 Q11 samples *without* metadata are used. */
	status = bladerf_sync_config(dev, BLADERF_RX_X1, BLADERF_FORMAT_SC16_Q11, num_buffers, buffer_size, num_transfers, timeout_ms);

	if (status != 0)
	{
		std::cout<< "Failed to configure RX sync interface: " << bladerf_strerror(status) << std::endl;
		return status;
	}

	if (status != 0)
	{
		bladerf_close(dev);
		return 1;
	}

	int16_t* rx_samples = new int16_t[bufferSize];

	memset(&rx_samples, 0, sizeof(rx_samples));

	status = bladerf_enable_module(dev, BLADERF_RX, true);

	if (status != 0)
	{
		std::cout << "Failed to enable RX: " << bladerf_strerror(status) << std::endl;
		bladerf_close(dev);
	}

	status = bladerf_sync_rx(dev, rx_samples, sampleLength, NULL, 5000);

	status = bladerf_enable_module(dev, BLADERF_RX, false);
	
	delete [] rx_samples;

	if (status != 0)
	{
		std::cout << "Failed to disable RX: " << bladerf_strerror(status) << std::endl;
	}

	return status;
}
