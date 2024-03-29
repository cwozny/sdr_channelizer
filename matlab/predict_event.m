%% Clear everything out

clc

fprintf('%s - Clearing everything out\n', datetime)

clear all
close all

%% Load data

listing = dir(uigetdir('/'));

firstFileSampleTime = nan;

%% Initialize data

event = [];

hFig=figure;
hNextEventPlt=plot(nan,nan,'gd');
hold on
hSamplePlt=plot(nan,nan,'b.');
hEventFitPlt=plot(nan,nan,'m');
hCurrEventPlot=plot(nan,nan,'r*');

grid on
ylabel('SNR (dB)')
xlabel('Sample Time (sec)')

for ii = 1:length(listing)
    if contains(listing(ii).name,'.mat')

        pdw.toa = [];
        pdw.freq = [];
        pdw.snr = [];
        pdw.pw = [];
        pdw.sat = [];

        fprintf('%s - Loading %s\n', datetime, listing(ii).name)

        load(fullfile(listing(ii).folder,listing(ii).name))

        if isnan(firstFileSampleTime)
            firstFileSampleTime = sampleStartTime;
        end

        maxVal = double(2^(bitWidth-1));
        iq = double(iq); % Convert from intX to double
        iq = iq/maxVal; % Normalize from -1 to 1
        iq = iq(1,:) + 1j*iq(2,:); % Convert to complex

        if max(abs(iq)) > 0.9

            fprintf('%s - Computing noise floor\n', datetime)

            % Convert complex I/Q to magnitude and phase data
            mag = abs(iq);
            phase = rad2deg(angle(iq));

            % Find the median magnitude value and set that as the noise floor.
            % I used median here instead of average because it is a "resistant
            % statistic".
            NOISE_FLOOR = median(mag);
            SNR_THRESHOLD = 20 % dB
            PULSE_THRESHOLD = NOISE_FLOOR*10^(SNR_THRESHOLD/10)

            fprintf('%s - Generating PDWs\n', datetime)

            pulseActive = false; % keeps track of whether pulse is active
            saturated = false; % keeps track of whether pulse was ever saturated

            for jj = 1:length(iq)
                % Look for a leading edge
                if ~pulseActive
                    if mag(jj) >= PULSE_THRESHOLD
                        pulseActive = true; % a pulse is now active
                        toa = jj; % initialize the time of arrival to current index
                        saturated = false; % initialize whether the pulse was ever saturated
                    end
                else % Look for a trailing edge now that pulse is active
                    if mag(jj) <= PULSE_THRESHOLD % Declare a trailing edge
                        pulseActive = false; % the pulse is no longer active

                        % compute the UTC time of the time of arrival of the pulse
                        thisToa = (toa/fs)+(sampleStartTime - firstFileSampleTime);

                        % compute the amplitude as the median magnitude over the entire pulse
                        thisAmp = median(mag(toa:jj));

                        % compute the SNR for this pulse given the
                        % amplitude and noise floor for this channelizer bin
                        thisSnr = 10*log10(thisAmp/NOISE_FLOOR);

                        % compute the pulse width as the number of samples
                        % this pulse was active for divided by the sampling
                        % rate for this channelizer bin
                        thisPw = (jj-toa)/fs;

                        % compute the median phase difference of this pulse
                        % in order to compute the frequency
                        phaseDiff = diff(phase(toa:jj));
                        phaseDiff(phaseDiff < -180) = phaseDiff(phaseDiff < -180) + 360;
                        phaseDiff(phaseDiff > 180) = phaseDiff(phaseDiff > 180) - 360;
                        medPhaseDiff = median(phaseDiff);

                        % compute the frequency by finding the period given
                        % the median phase difference and then offset it
                        % from the center frequency of this channelizer bin
                        thisFreq = fc+(fs/(360/medPhaseDiff));

                        pdw.toa = [pdw.toa; thisToa];
                        pdw.freq = [pdw.freq; thisFreq];
                        pdw.pw = [pdw.pw; thisPw];
                        pdw.snr = [pdw.snr; thisSnr];
                        pdw.sat = [pdw.sat; saturated];
                    else % Otherwise we're still measuring a pulse
                        if abs(real(iq(jj))) >= 0.9999 || abs(imag(iq(jj))) >= 0.9999
                            saturated = true;
                        end
                    end
                end
            end

            p = polyfit(pdw.toa,pdw.snr,2);
            t1 = min(pdw.toa):0.001:max(pdw.toa);
            y1 = polyval(p,t1);

            t_max = -p(2) / (2*p(1));
            y_max = p(1)*t_max^2 + p(2)*t_max + p(3);

            event = [event; t_max];

            if length(event) > 1
                nextEvent = median(diff(event)) + t_max;
            else
                nextEvent = t_max + 4.61962892466417;
            end

            hNextEventPlt.XData = [hNextEventPlt.XData nextEvent];
            hNextEventPlt.YData = [hNextEventPlt.YData y_max];

            hEventFitPlt.XData = [hEventFitPlt.XData t1 nan];
            hEventFitPlt.YData = [hEventFitPlt.YData y1 nan];

            hSamplePlt.XData = [hSamplePlt.XData pdw.toa'];
            hSamplePlt.YData = [hSamplePlt.YData pdw.snr'];

            hCurrEventPlot.XData = [hCurrEventPlot.XData t_max];
            hCurrEventPlot.YData = [hCurrEventPlot.YData y_max];
        end
    end
end
