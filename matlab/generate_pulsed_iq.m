%% Clear everything out

clc

fprintf('%s - Clearing everything out\n', datetime)

clear all
close all

%% Constants

Fs = 56e6;
f_start = -(Fs/2) + Fs*rand;
T = 10e-3;
PW = 200e-6;
PRI = 1000e-6;
BARKER_13 = true;
LFM_EXTENT = 1e6;
f_stop = f_start + LFM_EXTENT;

fprintf('%s - Sample rate = %1.1f Msps, Center Freq = %1.1f MHz, Pulse Width = %1.1f us, PRI = %1.1f us\n', datetime, Fs*1e-6, f_start*1e-6, PW*1e6, PRI*1e6)

%% Generate pulse train

fprintf('%s - Generating pulse train\n', datetime)

mag = zeros(Fs*T,1);
phi = zeros(Fs*T,1);

numSamplesForPw = Fs*PW;
numSamplesForPri = Fs*PRI;

if BARKER_13
    numSamplesPerChip = round(numSamplesForPw/13);

    if numSamplesForPw ~= numSamplesPerChip*13
        numSamplesForPw = numSamplesPerChip*13;
        PW = numSamplesForPw/Fs;
        fprintf('%s - Pulse width adjusted to %f us\n', datetime, PW*1e6)
    end
end

f_lfm = linspace(f_start,f_stop,numSamplesForPw)';

freq_phi = cumsum(2*pi*f_lfm/Fs);

my_phi = freq_phi;

if BARKER_13
    barker_phi = 90*ones(5*numSamplesPerChip,1);
    barker_phi = [barker_phi; -90*ones(2*numSamplesPerChip,1)];
    barker_phi = [barker_phi; 90*ones(2*numSamplesPerChip,1)];
    barker_phi = [barker_phi; -90*ones(1*numSamplesPerChip,1)];
    barker_phi = [barker_phi; 90*ones(1*numSamplesPerChip,1)];
    barker_phi = [barker_phi; -90*ones(1*numSamplesPerChip,1)];
    barker_phi = [barker_phi; 90*ones(1*numSamplesPerChip,1)];

    my_phi = my_phi + barker_phi;
end

my_phi = angle(exp(1j*my_phi));

idx = 1;

for ii = 1:length(mag)

    if (idx+numSamplesForPw) < length(mag)
        mag(idx:idx+numSamplesForPw-1,1) = 1;
        phi(idx:idx+numSamplesForPw-1,1) = my_phi;
    end

    idx = idx + numSamplesForPri;

    if idx >= length(mag)
        break
    end
end

%% Plot data

fprintf('%s - Plotting data\n', datetime)

iq = mag .* (cos(phi) + 1j*sin(phi));

t = 0:1/Fs:(T-(1/Fs));

figure
subplot(2,1,1)
plot(t, abs(iq), '.')
hAx=gca;
grid on
subplot(2,1,2)
plot(t, rad2deg(angle(iq)), '.')
hAx(2)=gca;
grid on
xlabel('Time (sec)')

linkaxes(hAx,'x')

figure
spectrogram(iq,1024,0,1024,Fs,'centered','yaxis')

%% Done

fprintf('%s - Done\n', datetime)