/* Save to a file, e.g. boilerplate.c, and then compile:
* $ gcc boilerplate.c -o libbladeRF_example_boilerplate -lbladeRF
*/
#include <libbladeRF.h>
#include <cstring>

#include <complex>
#include <vector>
#include <algorithm>
#include <iostream>

/* The RX and TX channels are configured independently for these parameters */
struct channel_config
{
	bladerf_channel channel;
	unsigned int frequency;
	unsigned int bandwidth;
	unsigned int samplerate;
	int gain;
};

static int init_sync(struct bladerf *dev)
{
	int status;
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

	status = bladerf_sync_config(dev, BLADERF_TX_X1, BLADERF_FORMAT_SC16_Q11, num_buffers, buffer_size, num_transfers, timeout_ms);

	if (status != 0)
	{
		std::cout << "Failed to configure TX sync interface: " << bladerf_strerror(status) << std::endl;
	}

	return status;
}

int configure_channel(struct bladerf *dev, struct channel_config *c)
{
	int status = bladerf_set_frequency(dev, c->channel, c->frequency);

	if (status != 0)
	{
		std::cout << "Failed to set frequency = " << c-> frequency << ": " << bladerf_strerror(status) << std::endl;
		return status;
	}

	status = bladerf_set_sample_rate(dev, c->channel, c->samplerate, NULL);

	if (status != 0)
	{
		std::cout <<  "Failed to set samplerate = " << c->samplerate << ": " << bladerf_strerror(status) << std::endl;
		return status;
	}

	status = bladerf_set_bandwidth(dev, c->channel, c->bandwidth, NULL);

	if (status != 0)
	{
		std::cout << "Failed to set bandwidth = " << c->bandwidth << ": " << bladerf_strerror(status) << std::endl;
		return status;
	}

	// Disable automatic gain control
	status = bladerf_set_gain_mode(dev, c->channel, BLADERF_GAIN_MGC);

	if (status != 0)
        {
                std::cout << "Failed to disable automatic gain control: " << bladerf_strerror(status) << std::endl;
                return status;
        }

	status = bladerf_set_gain(dev, c->channel, c->gain);

	if (status != 0)
	{
		std::cout << "Failed to set gain: " << bladerf_strerror(status) << std::endl;
		return status;
	}

	return status;
}

#define SAMPLE_RATE 50e6 // each sample represents 20 ns
#define SAMPLE_LENGTH 1048576 // each buffer represents 20.972 milliseconds
#define INT16_BUFFER_LENGTH 2097152

#define NOISE_FLOOR_WINDOW 2048 // every 40.96 us sample window will be evaluated for noise floor

int main(int argc, char *argv[])
{
	int status;
	struct channel_config config;
	struct bladerf *dev = NULL;
	struct bladerf_devinfo dev_info;

	/* Initialize the information used to identify the desired device
	* to all wildcard (i.e., "any device") values */
	bladerf_init_devinfo(&dev_info);

	status = bladerf_open_with_devinfo(&dev, &dev_info);

	if (status != 0)
	{
		std::cout << "Unable to open device: " << bladerf_strerror(status) << std::endl;
		return 1;
	}

	/* Set up RX channel 1 parameters */
	config.channel = BLADERF_CHANNEL_RX(0);
	config.frequency = 152.6e6;
	config.bandwidth = 1e6;
	config.samplerate = SAMPLE_RATE;
	config.gain = 60;

	status = configure_channel(dev, &config);

	if (status != 0)
	{
		std::cout << "Failed to configure RX channel 1. Exiting." << std::endl;
		bladerf_close(dev);
		return 1;
	}

	/* Initialize synch interface on RX and TX */
	status = init_sync(dev);

	if (status != 0)
	{
		bladerf_close(dev);
		return 1;
	}

	int16_t rx_samples[INT16_BUFFER_LENGTH]; // Receive sample buffer
	std::vector<std::complex<float>> iq;
	std::vector<float> mag;


	status = bladerf_enable_module(dev, BLADERF_RX, true);

	if (status != 0)
	{
		std::cout << "Failed to enable RX: " << bladerf_strerror(status) << std::endl;
		bladerf_close(dev);
	}

	for(bladerf_frequency freq = 70e6; freq <= 5999e6; freq += 1e6)
	{
		iq.clear();
		mag.clear();
		memset(&rx_samples, 0, sizeof(rx_samples));

		// Retune to new frequency
		status = bladerf_set_frequency(dev, BLADERF_CHANNEL_RX(0), freq);

		// What frequency did we actually get?
		bladerf_frequency actualFreq = 0;
		bladerf_get_frequency(dev, BLADERF_CHANNEL_RX(0), &actualFreq);

//		std::cout << "Asked for " << freq * 1e-6 << " MHz, got " << actualFreq * 1e-6 << " MHz" << std::endl;

		if (status != 0)
		{
			std::cout << "Failed to retune RX0 to " << freq*1e-6 << " MHz: " << bladerf_strerror(status) << std::endl;
		}
		else
		{
			status = bladerf_sync_rx(dev, rx_samples, SAMPLE_LENGTH, NULL, 5000);

			// Convert data from int16 I/Q to single precision I/Q
			for(int i = 0; i < INT16_BUFFER_LENGTH; i += 2)
			{
				std::complex<float> complex_sample(rx_samples[i], rx_samples[i+1]);

				complex_sample /= 2048.0; // normalize sample

				iq.push_back(complex_sample);
				mag.push_back(std::abs(complex_sample));
			}

			std::vector<float> maxNfeBins; 

			for(int n = 0; n < mag.size(); n += NOISE_FLOOR_WINDOW)
			{
				maxNfeBins.push_back(*std::max_element(mag.begin() + n, mag.begin() + n + NOISE_FLOOR_WINDOW - 1));
			}

			const float noiseFloor = *std::min_element(maxNfeBins.begin(), maxNfeBins.end());

			std::cout << "Noise floor at " << freq*1e-6 << " MHz is " << noiseFloor << std::endl;

			bool pulseActive = false;
			int pw = 0;
			int toa = 0;

			for(int n = 0; n < mag.size(); n++)
			{
				// Look for leading edge
				if (pulseActive == false)
				{
					if(mag[n] > noiseFloor)
					{
						pulseActive = true;
						pw = 1;
						toa = n;
					}
				}
				else // Now we're looking for a trailing edge since a pulse is active
				{
					if(mag[n] <= noiseFloor) // Declare a trailing edge
					{
						pulseActive = false;
					//	std::cout << toa << "," << freq*1e-6 << "," << pw << std::endl;
					}
					else // Otherwise we're still measuring a pulse
					{
						pw++;
					}
				}

			}
		}
	}

	status = bladerf_enable_module(dev, BLADERF_RX, false);

	if (status != 0)
	{
		std::cout << "Failed to disable RX: " << bladerf_strerror(status) << std::endl;
	}

	return status;
}
