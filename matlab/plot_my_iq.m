%% Clear everything out

clc

fprintf('%s - Clearing everything out\n', datetime)

clear all
close all

%% Load data

listing = dir(uigetdir('./'));

for ii = 1:length(listing)

    loaded = false;

    if contains(listing(ii).name,'.iq')

        fprintf('%s - Reading %s\n', datetime, listing(ii).name)

        fid = fopen(fullfile(listing(ii).folder,listing(ii).name),"r");

        endianness = fread(fid,1,'uint32=>uint32');

        if endianness == 0x00000000
            fprintf('%s - Reading in big endian file\n', datetime)
        elseif endianness == 0x01010101
            fprintf('%s - Reading in little endian file\n', datetime)
        else
            warning('%s - Reading in file with unknown endianness\n', datetime)
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
        iq = fread(fid,[2,inf],'int8=>int8');

        fclose(fid);

        assert(length(iq) == numSamples)

        loaded = true;

    elseif contains(listing(ii).name,'.mat')

        fprintf('%s - Loading %s\n', datetime, listing(ii).name)

        load(fullfile(listing(ii).folder,listing(ii).name))

        loaded = true;

    end

    if loaded
        iq = double(iq);
        %iq = iq/32768; % This is the max for the USRP b200mini
        iq = iq/2048; % This is the max for the bladeRF 2.0 micro 16-bit
        %iq = iq/256; % This is the max for the bladeRF 2.0 micro 8-bit
        iq = iq(1,:) + 1j*iq(2,:);

        mag = abs(iq);
        phase = rad2deg(angle(iq));

        fprintf('%s - Computing time\n', datetime)

        t = 0:1/fs:(length(iq)-1)/fs;

        fprintf('%s - Plotting\n', datetime)

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
