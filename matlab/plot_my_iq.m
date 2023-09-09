%% Clear everything out

clc

fprintf('%s - Clearing everything out\n', datestr(now))

clear all
close all

%% Load data

listing = dir(uigetdir('/'));

for ii = 1:length(listing)
    if contains(listing(ii).name,'.iq')

        fprintf('%s - Reading %s\n', datestr(now), listing(ii).name)

        fid = fopen(fullfile(listing(ii).folder,listing(ii).name),"r");

        endianness = fread(fid,1,'uint32=>uint32');

        if endianness == 0x00000000
            fprintf('%s - Reading in big endian file\n', datestr(now))
        elseif endianness == 0x01010101
            fprintf('%s - Reading in little endian file\n', datestr(now))
        else
            warning('%s - Reading in file with unknown endianness\n', datestr(now))
        end

        linkSpeed = fread(fid,1,'uint32=>uint32');
        fc = fread(fid,1,'uint32=>float64');
        bw = fread(fid,1,'uint32=>float64');
        fs = fread(fid,1,'uint32=>float64');
        gain = fread(fid,1,'uint32=>float64');
        numSamples = fread(fid,1,'uint32=>float64');
        spare1 = fread(fid,1,'uint32=>uint32');
        fpgaVersion = string(fread(fid,32,'*char')');
        fwVersion = string(fread(fid,32,'*char')');
        sampleStartTime = fread(fid,1,'float64=>float64');
        iq = fread(fid,[2,inf],'int16=>int16');

        fclose(fid);

        assert(length(iq) == numSamples)

        iq = double(iq);
        iq = iq/32768;
        iq = iq(1,:) + 1j*iq(2,:);

        mag = abs(iq);
        phase = rad2deg(angle(iq));

        fprintf('%s - Computing time\n', datestr(now))

        t = 0:1/fs:(length(iq)-1)/fs;

        fprintf('%s - Plotting\n', datestr(now))

        figure
        
        subplot(2,1,1)
        plot(t,mag,'.')
        grid on
        hAx=gca;
        ylabel('Magnitude')

        subplot(2,1,2)
        plot(t,phase,'.')
        grid on
        hAx(2)=gca;
        ylabel('Phase (degs)')
        xlabel('Time (secs)')
        ylim([-180 180])

        linkaxes(hAx,'x')
    end
end
