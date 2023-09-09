%% Clear everything out

clc

fprintf('%s - Clearing everything out\n', datestr(now))

clear all
close all

%% Load data

listing = dir(uigetdir('/'));

firstFileSampleTime = nan;

%% Initialize data

mbe = [];

hFig=figure;
hNextMbePlt=plot(nan,nan,'gd');
hold on
hSamplePlt=plot(nan,nan,'b.');
hMbeFitPlt=plot(nan,nan,'m');
hCurrMbePlot=plot(nan,nan,'r*');

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

        fprintf('%s - Loading %s\n', datestr(now), listing(ii).name)

        load(fullfile(listing(ii).folder,listing(ii).name))

        if isnan(firstFileSampleTime)
            firstFileSampleTime = sampleStartTime;
        end

        iq = double(iq); % Convert from int16 to double
        iq = iq/32768; % Normalize from -1 to 1
        iq = iq(1,:) + 1j*iq(2,:); % Convert to complex

        if max(abs(iq)) > 0.9

            fprintf('%s - Computing noise floor\n', datestr(now))

            % Convert complex I/Q to magnitude and phase data
            mag = abs(iq);
            phase = rad2deg(angle(iq));

            % Find the median magnitude value and set that as the noise floor.
            % I used median here instead of average because it is a "resistant
            % statistic".
            NOISE_FLOOR = median(mag);
            SNR_THRESHOLD = 20 % dB
            PULSE_THRESHOLD = NOISE_FLOOR*10^(SNR_THRESHOLD/10)

            fprintf('%s - Generating PDWs\n', datestr(now))

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
                        if real(iq(jj)) >= 0.9999 || imag(iq(jj)) >= 0.9999
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

            mbe = [mbe; t_max];

            if length(mbe) > 1
                nextMbe = median(diff(mbe)) + t_max;
            else
                nextMbe = t_max + 4.61962892466417;
            end

            hNextMbePlt.XData = [hNextMbePlt.XData nextMbe];
            hNextMbePlt.YData = [hNextMbePlt.YData y_max];

            hMbeFitPlt.XData = [hMbeFitPlt.XData t1 nan];
            hMbeFitPlt.YData = [hMbeFitPlt.YData y1 nan];

            hSamplePlt.XData = [hSamplePlt.XData pdw.toa'];
            hSamplePlt.YData = [hSamplePlt.YData pdw.snr'];

            hCurrMbePlot.XData = [hCurrMbePlot.XData t_max];
            hCurrMbePlot.YData = [hCurrMbePlot.YData y_max];
        end
    end
end