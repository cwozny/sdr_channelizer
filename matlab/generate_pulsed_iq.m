%% Clear everything out

clc

fprintf('%s - Clearing everything out\n', datetime)

clear all
close all

%% Constants

Fs = 56e6;
f = -(Fs/2) + Fs*rand;
T = 1e-3;
PW = 13e-6;
PRI = 100e-6;

fprintf('%s - Sample rate = %1.1f Msps, Center Freq = %1.1f MHz, Pulse Width = %1.1f us, PRI = %1.1f us\n', datetime, Fs*1e-6, f*1e-6, PW*1e6, PRI*1e6)

%% Generate pulse train

fprintf('%s - Generating pulse train\n', datetime)

mag = zeros(Fs*T,1);
phi = zeros(Fs*T,1);

numSamplesForPw = Fs*PW;
numSamplesForPri = Fs*PRI;

idx = 1;

for ii = 1:length(mag)
    
    numSamplesPerChip = numSamplesForPw/13;
    
    barker_phi = 90*ones(5*numSamplesPerChip,1);
    barker_phi = [barker_phi; -90*ones(2*numSamplesPerChip,1)];
    barker_phi = [barker_phi; 90*ones(2*numSamplesPerChip,1)];
    barker_phi = [barker_phi; -90*ones(1*numSamplesPerChip,1)];
    barker_phi = [barker_phi; 90*ones(1*numSamplesPerChip,1)];
    barker_phi = [barker_phi; -90*ones(1*numSamplesPerChip,1)];
    barker_phi = [barker_phi; 90*ones(1*numSamplesPerChip,1)];

    freq_phi = zeros(numSamplesForPw,1);

    for jj = 1:numSamplesForPw-1
        freq_phi(jj+1) = freq_phi(jj) + 2*pi*f/Fs;
    end
    
    my_phi = freq_phi+barker_phi;
    
    my_phi = angle(exp(1j*my_phi));

    if (idx+numSamplesForPw) < length(mag)
        mag(idx:idx+numSamplesForPw-1,1) = 1;
        phi(idx:idx+numSamplesForPw-1,1) = my_phi/pi;
    end

    idx = idx + numSamplesForPri;

    if idx >= length(mag)
        break
    end
end

%% Plot data

figure
plot(my_phi,'.')
grid on

fprintf('%s - Plotting data\n', datetime)

iq = mag .* (cosd(phi) + 1j*sind(phi));

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

%% Done

fprintf('%s - Done\n', datetime)