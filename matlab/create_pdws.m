%% Clear everything out

clc

fprintf('%s - Clearing everything out\n', datestr(now))

clear all
close all

%% Load data

listing = dir('.');

pdw.t = [];
pdw.freq = [];
pdw.snr = [];
pdw.pw = [];
pdw.saturated = [];

M = 60; % 1 MHz channelizer bins for an Fs of 60e6

channelizer = dsp.Channelizer(M);

for ii = 1:length(listing)
    if contains(listing(ii).name,'.mat')

        fprintf('%s - Loading %s\n', datestr(now), listing(ii).name)

        load(fullfile(listing(ii).folder,listing(ii).name))

        iq = double(iq); % Convert from int16 to double
        iq = iq/32768; % Normalize from -1 to 1
        iq = iq(1,:) + 1j*iq(2,:); % Convert to complex

        fprintf('%s - Channelizing data\n', datestr(now))

        if length(iq) ~= size(iq,1)
            % Convert from row vector to column vector. NOTE: Don't use the
            % notation iq' here because that tranposes and conjugates.
            iq = transpose(iq);
        end

        % Lop off excess samples to make length a multiple of the number of
        % channelizer bins
        lastSample = floor(length(iq)/M)*M;

        iq(lastSample+1:end) = [];

        iq_chan = channelizer(iq);

        iq_chan = fftshift(iq_chan,2);

        mag = abs(iq_chan);
        fs_chan = fs/M;

        binFreqs = centerFrequencies(channelizer,fs);

        fprintf('%s - Computing noise floor\n', datestr(now))

        nf = median(mag);

        SNR_THRESHOLD = 25 % dB
        PULSE_THRESHOLD = nf*10^(SNR_THRESHOLD/10)

        fprintf('%s - Generating PDWs\n', datestr(now))

        for bin = 1:M
            iq = iq_chan(:,bin);

            mag = abs(iq);
            phase = rad2deg(angle(iq));

            fc_chan = fc + binFreqs(bin);

            pulseActive = false;
            saturated = false;

            for jj = 1:length(iq)
                % Look for a leading edge
                if ~pulseActive
                    if mag(jj) >= PULSE_THRESHOLD(bin)
                        pulseActive = true;
                        pw = 1;
                        toa = jj;   
                        amp = mag(jj);
                        saturated = false;

                        if real(iq(jj)) >= 0.9999 || imag(iq(jj)) >= 0.9999
                            saturated = true;
                        end
                    end
                else % Look for a trailing edge now that pulse is active
                    if mag(jj) <= PULSE_THRESHOLD(bin) % Declare a trailing edge
                        pulseActive = false;
                        pdw.t = [pdw.t; ((toa/fs_chan)+sampleStartTime)];
                        pdw.snr = [pdw.snr; 10*log10((amp/pw)/nf(bin))];
                        pdw.pw = [pdw.pw; pw/fs_chan];
                        pdw.saturated = [pdw.saturated; saturated];

                        medPhaseDiff = median(diff(phase(toa:jj)));

                        pdw.freq = [pdw.freq; fc_chan+(fs_chan/(360/medPhaseDiff))];
                    else % Otherwise we're still measuring a pulse
                        pw = pw + 1;
                        amp = amp + mag(jj);

                        if real(iq(jj)) >= 0.9999 || imag(iq(jj)) >= 0.9999
                            saturated = true;
                        end
                    end
                end
            end
        end
    end
end

pdw.saturated = pdw.saturated == 1;
pdw.d = datetime(1970,1,1,0,0,pdw.t);

save('pdw.mat','pdw','-v7.3')