%% Clear everything out

clc

fprintf('%s - Clearing everything out\n', datestr(now))

clear all
close all

%% Load data

fprintf('%s - Loading data\n', datestr(now))

load data.mat

iq = iq';

%% Channelize

fprintf('%s - Channelizing data\n', datestr(now))

numBands = 50;

channelizer = dsp.Channelizer(numBands);

extra = mod(length(iq),numBands);

iq(end-(extra-1):end) = [];

output = channelizer(iq);

%% Compute intermediate data

fprintf('%s - Computing intermediate data\n', datestr(now))

mag = abs(iq);
% phase = rad2deg(angle(iq));

%% Compute time

fprintf('%s - Computing time\n', datestr(now))

n = length(iq);
decN = length(output(:,1));

t = 0 : (1/fs) : ((n-1)/fs);
decT = 0 : (numBands/fs) : (numBands*(decN-1)/fs);

%% Plot some stuff

fprintf('%s - Plotting data\n', datestr(now))

startT = 8;
stopT = 9;

% startT = 0;
% stopT = 60;

chanDelay = 1.2e-6;

timeIdx = startT <= t & t <= stopT;
decTimeIdx = startT <= decT & decT <= stopT;

figure
plot(t(timeIdx), mag(timeIdx))
hold on
plot(decT(decTimeIdx)-chanDelay, abs(output(decTimeIdx,1)))
plot(decT(decTimeIdx)-chanDelay, abs(output(decTimeIdx,2)))
plot(decT(decTimeIdx)-chanDelay, abs(output(decTimeIdx,3)))
plot(decT(decTimeIdx)-chanDelay, abs(output(decTimeIdx,4)))
plot(decT(decTimeIdx)-chanDelay, abs(output(decTimeIdx,5)))
plot(decT(decTimeIdx)-chanDelay, abs(output(decTimeIdx,6)))
plot(decT(decTimeIdx)-chanDelay, abs(output(decTimeIdx,7)))
plot(decT(decTimeIdx)-chanDelay, abs(output(decTimeIdx,8)))
plot(decT(decTimeIdx)-chanDelay, abs(output(decTimeIdx,9)))
plot(decT(decTimeIdx)-chanDelay, abs(output(decTimeIdx,10)))
ylim([0 1.5])
grid on
legend('Original','0','+1','+2','+3','+4','+-5','-4','-3','-2','-1')
