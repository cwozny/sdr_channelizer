%% Clear everything out

clc

fprintf('%s - Clearing everything out\n', datestr(now))

clear all
close all

%% Load data

fprintf('%s - Loading data\n', datestr(now))

load data.mat

mag = abs(iq);
phase = rad2deg(angle(iq));

%% Compute time

fprintf('%s - Computing time\n', datestr(now))

n = length(iq);

t = 0 : (1/fs) : ((n-1)/fs);

%% Crudely compute noise floor

fprintf('%s - Computing noise floor\n', datestr(now))

nf = median(mag);

%% Generate PDWs

fprintf('%s - Generating PDWs\n', datestr(now))

SNR_THRESHOLD = 20
PULSE_THRESHOLD = nf*10^(SNR_THRESHOLD/10)

pulseActive = false;
saturated = false;
pulse = false(size(mag));

pdw.toa = [];
pdw.freq = [];
pdw.snr = [];
pdw.pw = [];
pdw.saturated = [];

iqPdwIdx = uint32(zeros(size(mag)));
pdwIdx = uint32(1);

for ii = 1:n
    % Look for a leading edge
    if ~pulseActive
        if mag(ii) >= PULSE_THRESHOLD
            pulseActive = true;
            pw = 1;
            toa = ii;   
            amp = mag(ii);
            saturated = false;
            pulse(ii) = true; % used for debug purposes
            iqPdwIdx(ii) = pdwIdx; % used to link PDWs to I/Q data

            if real(iq(ii)) >= 0.9999 || imag(iq(ii)) >= 0.9999
                saturated = true;
            end
        end
    else % Look for a trailing edge now that pulse is active
        if mag(ii) <= PULSE_THRESHOLD % Declare a trailing edge
            pulseActive = false;
            pdw.toa = [pdw.toa; toa];
            pdw.snr = [pdw.snr; 10*log10((amp/pw)/nf)];
            pdw.pw = [pdw.pw; pw];
            pdw.saturated = [pdw.saturated; saturated];

            subPhase = double(phase(toa:ii));
            subReal = double(real(iq(toa:ii)));
            subImag = double(imag(iq(toa:ii)));
            subT = t(toa:ii);

            [f,~] = computeFrequency(subT,subPhase,fs);
            [f(2),~] = computeFrequency(subT,subReal,fs);
            [f(3),~] = computeFrequency(subT,subImag,fs);

            freq = f*1e-6;

            freq = freq(~isnan(freq));

            if ~isempty(freq)
                pdw.freq = [pdw.freq; fc+freq(1)];
            else
               pdw.freq = [pdw.freq; nan];
            end

            pdwIdx = pdwIdx + 1;

        else % Otherwise we're still measuring a pulse
            pw = pw + 1;
            amp = amp + mag(ii);
            pulse(ii) = true; % used for debug purposes
            iqPdwIdx(ii) = pdwIdx; % used to link PDWs to I/Q data

            if real(iq(ii)) >= 0.9999 || imag(iq(ii)) >= 0.9999
                saturated = true;
            end
        end
    end
end

pdw.saturated = pdw.saturated == 1;
pdw.t = pdw.toa/fs;
pdw.pw = pdw.pw/fs;

save('pdw.mat','pdw','-v7.3')

%% Plot some stuff

figure

startT = 2.4815036;
stopT = 2.4815047;

startT = 0;
stopT = 60;

timeIdx = startT <= t & t <= stopT;

subplot(4,1,1)
plot(t(pulse & timeIdx), mag(pulse & timeIdx), '.')
hAx=gca;
ylim([0 sqrt(2)])
grid on

subplot(4,1,2)
plot(t(pulse & timeIdx), phase(pulse & timeIdx), '.')
hAx(2)=gca;
hold on
ylim([-180 180])
grid on

subplot(4,1,3)
plot(t(pulse & timeIdx), real(iq(pulse & timeIdx)), '.')
hAx(3)=gca;
grid on
ylim([-1 1])

subplot(4,1,4)
plot(t(pulse & timeIdx), imag(iq(pulse & timeIdx)), '.')
hAx(4)=gca;
grid on
ylim([-1 1])

linkaxes(hAx,'x')
