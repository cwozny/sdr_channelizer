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

for ii = 1:length(listing)
    if contains(listing(ii).name,'.mat')

        fprintf('%s - Loading %s\n', datestr(now), listing(ii).name)

        load(fullfile(listing(ii).folder,listing(ii).name))

        iq = double(iq);
        iq = iq/32768;
        iq = iq(1,:) + 1j*iq(2,:);

        mag = abs(iq);
        phase = rad2deg(angle(iq));

        fprintf('%s - Computing time\n', datestr(now))

        n = length(iq);

        fprintf('%s - Computing noise floor\n', datestr(now))

        nf = median(mag);

        fprintf('%s - Generating PDWs\n', datestr(now))

        SNR_THRESHOLD = 7 % dB
        PULSE_THRESHOLD = nf*10^(SNR_THRESHOLD/10)

        pulseActive = false;
        saturated = false;
        
        for jj = 1:n
            % Look for a leading edge
            if ~pulseActive
                if mag(jj) >= PULSE_THRESHOLD
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
                if mag(jj) <= PULSE_THRESHOLD % Declare a trailing edge
                    pulseActive = false;
                    pdw.t = [pdw.t; ((toa/fs)+sampleStartTime)];
                    pdw.snr = [pdw.snr; 10*log10((amp/pw)/nf)];
                    pdw.pw = [pdw.pw; pw/fs];
                    pdw.saturated = [pdw.saturated; saturated];

                    medPhaseDiff = median(diff(phase(toa:jj)));

                    pdw.freq = [pdw.freq; fc+(fs/(360/medPhaseDiff))];
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

pdw.saturated = pdw.saturated == 1;
pdw.d = datetime(1970,1,1,0,0,pdw.t);

save('pdw.mat','pdw','-v7.3')
