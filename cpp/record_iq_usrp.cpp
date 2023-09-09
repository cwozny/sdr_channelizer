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

	std::string device_args("");
	std::string subdev("A:A");
	std::string ant("RX2");
	std::string ref("internal");
	IqPacket packet;
	char filenameStr[80];
	const std::int16_t INT12_MAX = 2047;
	const std::int16_t INT12_MIN = -2048;
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

	return EXIT_SUCCESS;
}
