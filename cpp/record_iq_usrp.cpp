#include <uhd/utils/thread.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/exception.hpp>
#include <uhd/types/tune_request.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>

#include "IqPacket.h"

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

void getFilenameStr(char* filenameStr)
{
	// Get current time
	const std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	// Convert current time from chrono to time_t which goes down to second precision
	const std::time_t tt = std::chrono::system_clock::to_time_t(now);
	// Convert back to chrono so that we have a current time rounded to seconds
	const std::chrono::system_clock::time_point nowSec = std::chrono::system_clock::from_time_t(tt);
	tm utc_tm = *gmtime(&tt);

	std::uint16_t year  = utc_tm.tm_year + 1900;
	std::uint8_t month  = utc_tm.tm_mon + 1;
	std::uint8_t day    = utc_tm.tm_mday;
	std::uint8_t hour   = utc_tm.tm_hour;
	std::uint8_t minute = utc_tm.tm_min;
	std::uint8_t second = utc_tm.tm_sec;
	std::uint16_t millisecond = (now-nowSec)/std::chrono::milliseconds(1);

	snprintf(filenameStr, 80, "%04d_%02d_%02d_%02d_%02d_%02d_%03d.iq", year, month, day, hour, minute, second, millisecond);
}

int UHD_SAFE_MAIN(int argc, char *argv[])
{
	uhd::set_thread_priority_safe();

	std::int32_t status = EXIT_SUCCESS;
	uhd::rx_metadata_t meta;
	std::string device_args("");
	std::string subdev("A:A");
	std::string ant("RX2");
	std::string ref("internal");
	IqPacket packet;
	char filenameStr[80];
	bool saturated = false;
	std::uint32_t overrunCounter = 0;

	const std::uint32_t frequencyHz = atof(argv[1])*1e6;
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

	std::cout << "FPGA version: " << packet.fpgaVersion << std::endl;

	std::cout << "Firmware version: " << packet.fwVersion << std::endl;

	uhd::dict<std::string, std::string> rx_info = usrp->get_usrp_rx_info();

	std::cout << "Using " << usrp->get_mboard_name() << " serial number " << rx_info.get("rx_serial") << std::endl;

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

	// Compute the requested number of samples and buffer size

	const std::uint32_t sampleLength = dwellDuration*receivedSampleRate;
	const std::uint32_t bufferSize = 2*sampleLength;

	// Set up the configuration parameters necessary to receive samples with the device

	// create a receive streamer
	uhd::stream_args_t stream_args("sc16","sc16"); // 16-bit integers
	uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);

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
	packet.baseTimeMs = std::chrono::system_clock::now().time_since_epoch() / std::chrono::milliseconds(1);

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
			size_t num_rx_samps = rx_stream->recv(&iq_vec.front()+(2*num_accum_samps), iq_vec.size()-(2*num_accum_samps), meta, 5.0, true);

			num_accum_samps += num_rx_samps;

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
			}
		}

		stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
		rx_stream->issue_stream_cmd(stream_cmd);

		std::cout << "Received " << num_accum_samps << std::endl;

		// Look for instances of saturating to max positive value
		std::vector<std::int16_t>::iterator maxVal = std::find(std::execution::par_unseq, std::begin(iq_vec), std::end(iq_vec), INT16_MAX);
		// Look for instances of saturating to max negative value
		std::vector<std::int16_t>::iterator minVal = std::find(std::execution::par_unseq, std::begin(iq_vec), std::end(iq_vec), INT16_MIN);

		saturated = (maxVal != std::end(iq_vec)) || (minVal != std::end(iq_vec));

		packet.numSamples = num_accum_samps;
		packet.sampleStartTime = 0;

		getFilenameStr(filenameStr);

		std::ofstream fout(filenameStr);
		fout.write((char*)&packet, sizeof(packet));
		fout.write((char*)iq, 2*num_accum_samps*sizeof(std::int16_t));
		fout.close();

		currentTime = std::chrono::system_clock::now();
	}
	while((currentTime - startTime) / std::chrono::seconds(1) <= collectionDuration);

	// Disable the device

	stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
	rx_stream->issue_stream_cmd(stream_cmd);

	std::cout << "Disabled RX" << std::endl;

	std::cout << "There were " << overrunCounter << " overruns." << std::endl;

	return status;
}
