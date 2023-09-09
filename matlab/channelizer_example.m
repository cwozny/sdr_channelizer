%% Clear everything out

clc

fprintf('%s - Clearing everything out\n', datetime)

clear all
close all

%% Load data

fprintf('%s - Loading data\n', datetime)

load data.mat

iq = iq';

%% Channelize

fprintf('%s - Channelizing data\n', datetime)

numBands = fs*1e-6; % 1 MHz channelizer bins

channelizer = dsp.Channelizer(numBands);

duration = 5e-3;
samples = duration*fs;

vidObj = VideoWriter('blah');

open(vidObj)

hFig=figure('WindowState','maximized');
hSurf = surf(1:2,1:2,rand(2));
hSurf.EdgeColor = 'none';
hold on
xlabel('Frequency (MHz)')
ylabel('Time (sec)')
zlabel('Magnitude')

pause(1)

for ii = 1:100*numBands:length(iq)

    start = ii;
    stop = ii+samples-1;

    out = abs(channelizer(iq(start:stop)));

    zeroCenterOut = fftshift(out,2);

    f = (fc-centerFrequencies(channelizer,fs))*1e-6;
    t = ii/fs + (0:size(out,1)-1)*numBands/fs;

    hSurf.XData = f;
    hSurf.YData = t;
    hSurf.ZData = zeroCenterOut;
    
    axis tight

    zlim([0 1.5])

    writeVideo(vidObj,getframe(gcf))

end

close(vidObj)
