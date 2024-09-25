%% Clear everything out

clc

fprintf('%s - Clearing everything out\n', datetime)

clear all
close all

%% Constants

fs = 56e6;
f = -(fs/2) + fs*rand;
T = 100e-3;

MIN_PW = 10e-6;
MAX_PW = 1000e-6;
PW = MIN_PW + (MAX_PW - MIN_PW) * rand;
MIN_PRI = 10e-6;
MIN_PRI = max(MIN_PRI,PW); % make sure our PRI is at least as long as our PW
MAX_PRI = 10000e-6;
PRI = MIN_PRI + (MAX_PRI - MIN_PRI) * rand;

startIdx = randi(round(PRI * fs)); % pick a random sample index
t0 = startIdx / fs;
startIdx = round(t0 * fs);

fprintf('%s - Sample rate = %1.1f Msps, Center Freq = %1.1f MHz, Pulse Width = %1.1f us, PRI = %1.1f us\n', datetime, fs*1e-6, f*1e-6, PW*1e6, PRI*1e6)

%% Generate pulse train

fprintf('%s - Generating pulse train\n', datetime)

mag = zeros(fs*T,1);
phi = zeros(fs*T,1);

numSamplesForPw = round(fs*PW);
numSamplesForPri = round(fs*PRI);

idx = startIdx;

for ii = 1:length(mag)

    freq_phi = zeros(numSamplesForPw,1);

    for jj = 1:numSamplesForPw-1
        freq_phi(jj+1) = freq_phi(jj) + 2*pi*f/fs;
    end

    my_phi = angle(exp(1j*freq_phi));

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

t = 0:1/fs:(T-(1/fs));

figure
subplot(2,1,1)
plot(t, abs(iq), '.')
hold on
hAx=gca;
grid on

ylabel('Magnitude')

subplot(2,1,2)
plot(t, rad2deg(angle(iq)), '.')
hold on
hAx(2)=gca;
grid on
ylim([-180 180])

ylabel('Phase (degs)')
xlabel('Time (sec)')

linkaxes(hAx,'x')

%% Cast data to 16-bit I & Q

binWidth = 0.1e6;
numBands = round(fs/binWidth); % channelizer bins

channelizer = dsp.Channelizer(numBands);

chan_iq = channelizer(iq);
chan_iq = fftshift(chan_iq,2);

f = centerFrequencies(channelizer,fs)*1e-6;
t = (0:size(chan_iq,1)-1)*numBands/fs;

hFig=figure;

hSurf = surf(f,t,abs(chan_iq));
hSurf.EdgeColor = 'none';

xlabel('Frequency (MHz)')
ylabel('Time (sec)')
zlabel('Magnitude')

binfile = fopen(sprintf("%1.1f_MHz_%1.1f_us_%1.1f_us.iq",f*1e-6,PW*1e6,PRI*1e6),"w","ieee-le");

fwrite(binfile,hex2dec("01010101"),"uint32"); % LE
fwrite(binfile,1,"uint32");
fwrite(binfile,0,"uint32");
fwrite(binfile,fs,"uint32"); % bandwidth
fwrite(binfile,fs,"uint32"); % sample rate
fwrite(binfile,0,"uint32");
fwrite(binfile,length(iq),"uint32"); % number of samples
fwrite(binfile,16,"uint32"); % bit width

str = "simulated";

fwrite(binfile,char(str),"char");
fwrite(binfile,zeros(4*16 - length(char(str)),1),"uint8");

fwrite(binfile,seconds(datetime - datetime(1970,1,1,0,0,0)),"double"); % sample start time

fwrite(binfile,[iq_real_16 iq_imag_16]',"int16");

fclose(binfile);

%% Done

fprintf('%s - Done\n', datetime)