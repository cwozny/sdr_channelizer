#include <uhd/utils/thread.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/exception.hpp>
#include <uhd/types/tune_request.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>

#include <cstring>
#include <ctime>

#include <bit>
#include <iostream>
#include <fstream>
#include <chrono>
#include <algorithm>
#include <vector>
#include <iterator>
#include <execution>

int UHD_SAFE_MAIN(int argc, char *argv[])
{
	std::int32_t status = EXIT_SUCCESS;
	uhd::rx_metadata_t meta;
	std::string device_args("");
	std::string subdev("");
	std::string ref("internal");
	char filenameStr[80];
	const std::int16_t SAMP_MAX = 32736;
	const std::int16_t SAMP_MIN = -32736;
	bool saturated = false;
	std::uint32_t overrunCounter = 0;

	const std::uint32_t frequencyHz = atof(argv[1])*1e6;
	const std::uint32_t requestedBandwidthHz = atof(argv[2])*1e6;
	std::uint32_t receivedBandwidthHz = 0;
	const std::uint32_t requestedSampleRate = atof(argv[3])*1e6;
	std::uint32_t receivedSampleRate = 0;
	std::int32_t rxGain = atoi(argv[4]);
	std::int32_t txGain = atoi(argv[4]);
	const float dwellDuration = atof(argv[5]);
	const float collectionDuration = atof(argv[6]);

	//create a usrp device

	uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(device_args);

	// Lock mboard clocks
	usrp->set_clock_source(ref);

	//always select the subdevice first, the channel mapping affects the other settings
	usrp->set_rx_subdev_spec(subdev);
	usrp->set_tx_subdev_spec(subdev);

	uhd::dict<std::string, std::string> rx_info = usrp->get_usrp_rx_info();

	std::cout << "Using " << usrp->get_mboard_name() << " serial number " << rx_info.get("rx_serial") << std::endl;

	// Set center frequency of device

	uhd::tune_request_t tune_request(frequencyHz);
	usrp->set_rx_freq(tune_request);
	usrp->set_tx_freq(tune_request);

	std::cout << "Frequency = " << frequencyHz*1e-6 << " MHz" << std::endl;

	// Set sample rate of device

	usrp->set_rx_rate(requestedSampleRate);
	receivedSampleRate = usrp->get_rx_rate();
	std::cout <<  "Rx Sample Rate = " << receivedSampleRate*1e-6 << " Msps" << std::endl;

	usrp->set_tx_rate(requestedSampleRate);
	receivedSampleRate = usrp->get_tx_rate();
	std::cout <<  "Tx Sample Rate = " << receivedSampleRate*1e-6 << " Msps" << std::endl;

	// Set analog bandwidth of device

	usrp->set_rx_bandwidth(requestedBandwidthHz);
	receivedBandwidthHz = usrp->get_rx_bandwidth();
	std::cout << "Rx Bandwidth = " << receivedBandwidthHz*1e-6 << " MHz" << std::endl;
	
	usrp->set_tx_bandwidth(requestedBandwidthHz);
	receivedBandwidthHz = usrp->get_tx_bandwidth();
	std::cout << "Tx Bandwidth = " << receivedBandwidthHz*1e-6 << " MHz" << std::endl;

	// Disable automatic gain control

	usrp->set_rx_agc(false);

	std::cout << "Disabled automatic gain control" << std::endl;

	// Set gain of the device

	usrp->set_rx_gain(rxGain);
	rxGain = usrp->get_rx_gain();
	std::cout << "Rx Gain = " << rxGain << " dB" << std::endl;

	usrp->set_tx_gain(txGain);
	txGain = usrp->get_tx_gain();
	std::cout << "Tx Gain = " << txGain << " dB" << std::endl;

	usrp->set_rx_antenna("RX2");
	usrp->set_tx_antenna("TRX");

	// Set the time on the device

	const double timeInSecs = std::chrono::system_clock::now().time_since_epoch() / std::chrono::microseconds(1) * 1e-6;

	usrp->set_time_now(uhd::time_spec_t(timeInSecs));
/*
	// Set up the configuration parameters necessary to receive samples with the device

	// create a receive streamer
	uhd::stream_args_t stream_args("sc16","sc12"); // 16-bit integers on host, 12-bit over-the-wire
	uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);
	const std::uint32_t maxSampsPerBuffer = rx_stream->get_max_num_samps();

	// Compute the requested number of samples and buffer size

	const std::uint32_t sampleLength = dwellDuration*receivedSampleRate;
	const std::uint32_t bufferSize = 2*(sampleLength+maxSampsPerBuffer);

	// setup streaming
	uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
	stream_cmd.num_samps  = sampleLength;
	stream_cmd.stream_now = true;
	stream_cmd.time_spec  = uhd::time_spec_t();

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

	packet.frequencyHz = frequencyHz;
	packet.bandwidthHz = receivedBandwidthHz;
	packet.sampleRate = receivedSampleRate;
	packet.numSamples = sampleLength;

	// Allocate the host buffer the device will be streaming to

	std::vector<std::int16_t> iq_vec;
	iq_vec.resize(bufferSize, 0);
	std::int16_t* iq = iq_vec.data();

	const std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();
	std::chrono::system_clock::time_point currentTime;

	do
	{
		// If we're saturated, then drop the receive gain down by 1 dB
		if (saturated)
		{
			usrp->set_rx_gain(--rxGain);
			rxGain = usrp->get_rx_gain();

			std::cout << "Gain = " << rxGain << " dB" << std::endl;
		}

		packet.rxGain = rxGain;
		saturated = false;

		memset(iq, 0, bufferSize*sizeof(std::int16_t));

		memset(&meta, 0, sizeof(meta));

		size_t num_accum_samps = 0;

		stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS;
		rx_stream->issue_stream_cmd(stream_cmd);

		while(num_accum_samps < sampleLength)
		{
			const std::int32_t startIndex = 2*num_accum_samps;
			const std::int32_t remainingSize = 2*sampleLength-(2*num_accum_samps);

			num_accum_samps += rx_stream->recv(&iq[startIndex], remainingSize, meta, 5.0, true);

			// Handle streaming error codes
			switch (meta.error_code)
			{
				// No errors
				case uhd::rx_metadata_t::ERROR_CODE_NONE:
					break;

				case uhd::rx_metadata_t::ERROR_CODE_TIMEOUT: // I get this error on the expected last iteration of the while loop
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
		}

		stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
		rx_stream->issue_stream_cmd(stream_cmd);

		std::cout << "Received " << num_accum_samps << std::endl;

		// Look for instances of saturating to min or max value
		const auto [minSamp, maxSamp] = std::minmax_element(std::execution::par_unseq, std::begin(iq_vec), std::end(iq_vec));

		saturated = (((*minSamp) <= SAMP_MIN) || ((*maxSamp) >= SAMP_MAX));

		packet.numSamples = num_accum_samps;
		packet.sampleStartTime = meta.time_spec.get_real_secs();

		getFilenameStr(filenameStr);

		std::ofstream fout(filenameStr);
		fout.write((char*)&packet, sizeof(packet));
		fout.write((char*)iq, 2*num_accum_samps*sizeof(std::int16_t));
		fout.close();

		currentTime = std::chrono::system_clock::now();
	}
	while(((currentTime - startTime) / std::chrono::milliseconds(1) * 1e-3) <= collectionDuration);

	// Disable the device

	stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
	rx_stream->issue_stream_cmd(stream_cmd);

	std::cout << "Disabled RX" << std::endl;

	std::cout << "There were " << overrunCounter << " overruns." << std::endl;
*/
	return status;
}
