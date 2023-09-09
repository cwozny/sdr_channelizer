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
	std::int32_t status = EXIT_SUCCESS;
	uhd::rx_metadata_t meta;
	std::string device_args("");
	std::string subdev("A:A");
	std::string ant("RX2");
	std::string ref("internal");
	IqPacket packet;
	char filenameStr[80];
	const float SAMP_MAX = 0.9999;
	const float SAMP_MIN = -0.9999;
	bool saturated = false;
	bool badSamples = false;
	std::uint32_t overrunCounter = 0;

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

	// Set the time on the device

	const double timeInSecs = std::chrono::system_clock::now().time_since_epoch() / std::chrono::nanoseconds(1) * 1e-9;

	usrp->set_time_now(uhd::time_spec_t(timeInSecs));

	// Set up the configuration parameters necessary to receive samples with the device

	// create a receive streamer
	uhd::stream_args_t stream_args("fc32","sc12"); // 32-bit floats on host, 12-bit over-the-wire
	uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);
	const std::uint32_t maxSampsPerBuffer = rx_stream->get_max_num_samps();

	// Compute the requested number of samples and buffer size

	const std::uint32_t sampleLength = dwellDuration*receivedSampleRate;

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

	std::vector<std::complex<float>> iq_vec;
	iq_vec.resize(sampleLength, 0);
	std::complex<float>* iq = iq_vec.data();

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
		badSamples = false;

		memset(iq, 0, sampleLength*sizeof(std::complex<float>));

		memset(&meta, 0, sizeof(meta));

		size_t num_accum_samps = 0;

		stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS;
		rx_stream->issue_stream_cmd(stream_cmd);

		while(num_accum_samps < sampleLength)
		{
			const std::int32_t startIndex = num_accum_samps;
			const std::int32_t remainingSize = sampleLength-num_accum_samps;

			num_accum_samps += rx_stream->recv(&iq[startIndex], remainingSize, meta, 5.0, true);

			// Handle streaming error codes
			switch (meta.error_code)
			{
				// No errors
				case uhd::rx_metadata_t::ERROR_CODE_NONE:
					break;

				case uhd::rx_metadata_t::ERROR_CODE_TIMEOUT: // I get this error on the expected last iteration of the while loop
					std::cout << "ERROR_CODE_TIMEOUT: Got timeout before all samples received" << std::endl;
					badSamples = true;
					break;

				case uhd::rx_metadata_t::ERROR_CODE_OVERFLOW:
					overrunCounter++;
					std::cout << "ERROR_CODE_OVERFLOW: Overflowed" << std::endl;
					badSamples = true;
					break;

				default:
					std::cout << "Got error code: " << meta.strerror() << std::endl;
					badSamples = true;
					break;
			}
		}

		stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
		rx_stream->issue_stream_cmd(stream_cmd);

		std::cout << "Received " << num_accum_samps << std::endl;

		// If we received a full set of samples with no error
		if (badSamples == false)
		{
			// Look for instances of saturating to min or max value
			//const auto [minSamp, maxSamp] = std::minmax_element(std::execution::par_unseq, std::begin(iq_vec), std::end(iq_vec));

			//saturated = (((*minSamp) <= SAMP_MIN) || ((*maxSamp) >= SAMP_MAX));

			// Compute the noise floor as the median of the magnitude of the I/Q data
			float NOISE_FLOOR = 0.1; // TODO: median(mag)
			float SNR_THRESHOLD = 12; // dB

			// Compute the magnitude threshold to declare a pulse
			float PULSE_THRESHOLD = NOISE_FLOOR * pow(10,SNR_THRESHOLD/1);

			bool pulseActive = false; // keeps track of whether pulse is active
			bool pulseSaturated = false; // keeps track of whether pulse was ever saturated
			std::uint32_t toa = 0; // this is sufficient as long as we don't ever record more than ~35 seconds of samples at 60 Msps in one buffer

			// Loop through I/Q and generate PDWs
			for(int jj = 0; jj < num_accum_samps; jj++)
			{
				// Look for a leading edge
				if (pulseActive == false)
				{
					if (std::abs(iq[jj]) >= PULSE_THRESHOLD)
					{
						pulseActive = true; // a pulse is now active
						toa = jj; // initialize the time of arrival to current index
						pulseSaturated = false; // initialize whether the pulse was ever saturated
					}
				}
				else // Look for a trailing edge now that pulse is active
				{
					if (std::abs(iq[jj]) <= PULSE_THRESHOLD) // Declare a trailing edge
					{
						pulseActive = false; // the pulse is no longer active

						// compute the UTC time of the time of arrival of the pulse
						//thisToa = (toa/fs)+(sampleStartTime - firstFileSampleTime);

						// compute the amplitude as the median magnitude over the entire pulse
						//thisAmp = median(mag(toa:jj));

						// compute the SNR for this pulse given the
						// amplitude and noise floor for this channelizer bin
						//thisSnr = 10*log10(thisAmp/NOISE_FLOOR);

						// compute the pulse width as the number of samples
						// this pulse was active for divided by the sampling
						// rate for this channelizer bin
						//thisPw = (jj-toa)/fs;

						// compute the median phase difference of this pulse
						// in order to compute the frequency
						//phaseDiff = diff(phase(toa:jj));
						//phaseDiff(phaseDiff < -180) = phaseDiff(phaseDiff < -180) + 360;
						//phaseDiff(phaseDiff > 180) = phaseDiff(phaseDiff > 180) - 360;
						//medPhaseDiff = median(phaseDiff);

						// compute the frequency by finding the period given
						// the median phase difference and then offset it
						// from the center frequency of this channelizer bin
						//thisFreq = fc+(fs/(360/medPhaseDiff));

						//pdw.toa = [pdw.toa; thisToa];
						//pdw.freq = [pdw.freq; thisFreq];
						//pdw.pw = [pdw.pw; thisPw];
						//pdw.snr = [pdw.snr; thisSnr];
						//pdw.sat = [pdw.sat; pulseSaturated];
					}
					else // Otherwise we're still measuring a pulse
					{
						if (abs(iq[jj].real()) >= SAMP_MAX || abs(iq[jj].imag()) >= SAMP_MAX)
						{
							pulseSaturated = true;
						}
					}
				}
			}
			
			// Now that we've generated the PDWs, let's go through and see if a
			// main beam event occurred
			
			// p = polyfit(pdw.toa,pdw.snr,2);
			// t1 = min(pdw.toa):0.001:max(pdw.toa);
			// y1 = polyval(p,t1);

			// t_max = -p(2) / (2*p(1));
			//y_max = p(1)*t_max^2 + p(2)*t_max + p(3);

			// mbe = [mbe; t_max];

			// if length(mbe) > 1
				// nextMbe = median(diff(mbe)) + t_max;
			// else
				// nextMbe = t_max + 4.61962892466417;
			//end
		}

		packet.numSamples = num_accum_samps;
		packet.sampleStartTime = meta.time_spec.get_real_secs();

		getFilenameStr(filenameStr);

		//std::ofstream fout(filenameStr);
		//fout.write((char*)&packet, sizeof(packet));
		//fout.write((char*)iq, 2*num_accum_samps*sizeof(std::int16_t));
		//fout.close();

		currentTime = std::chrono::system_clock::now();
	}
	while(((currentTime - startTime) / std::chrono::milliseconds(1) * 1e-3) <= collectionDuration);

	// Disable the device

	stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
	rx_stream->issue_stream_cmd(stream_cmd);

	std::cout << "Disabled RX" << std::endl;

	std::cout << "There were " << overrunCounter << " overruns." << std::endl;

	return status;
}
