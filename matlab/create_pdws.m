%% Clear everything out

clc

fprintf('%s - Clearing everything out\n', datestr(now))

clear all
close all

%% Load data

listing = dir('.');

%% Initialize data

pdw.toa = [];
pdw.freq = [];
pdw.snr = [];
pdw.pw = [];
pdw.sat = [];

for ii = 1:length(listing)
    if contains(listing(ii).name,'.mat')

        fprintf('%s - Loading %s\n', datestr(now), listing(ii).name)

        load(fullfile(listing(ii).folder,listing(ii).name))

        iq = double(iq); % Convert from int16 to double
        iq = iq/32768; % Normalize from -1 to 1
        iq = iq(1,:) + 1j*iq(2,:); % Convert to complex

        fprintf('%s - Computing noise floor\n', datestr(now))

        % Convert complex I/Q to magnitude and phase data
        mag = abs(iq);
        phase = rad2deg(angle(iq));

        % Find the median magnitude value and set that as the noise floor.
        % I used median here instead of average because it is a "resistant
        % statistic".
        NOISE_FLOOR = median(mag);
        SNR_THRESHOLD = 15 % dB
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
                    thisToa = ((toa/fs)+sampleStartTime);

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
    end
end

pdw.sat = pdw.sat == 1;
pdw.d = datetime(1970,1,1,0,0,pdw.toa);

save('pdw.mat','pdw','-v7.3')