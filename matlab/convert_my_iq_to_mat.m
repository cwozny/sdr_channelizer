%% Clear everything out

clc

fprintf('%s - Clearing everything out\n', datetime)

clear all
close all

%% Load data

listing = dir(uigetdir('../cpp'));

for ii = 1:length(listing)
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
        bitWidth = fread(fid,1,'uint32=>uint32');
        fpgaVersion = string(fread(fid,32,'*char')');
        fwVersion = string(fread(fid,32,'*char')');
        sampleStartTime = fread(fid,1,'float64=>float64');
        iq = fread(fid,[2,inf],'int16=>int16');

        fclose(fid);

        assert(length(iq) == numSamples)

        %% Compute duration

        dur = length(iq)/fs;

        %% Saving data

        fprintf('%s - Saving I/Q\n', datetime)

        [filepath,name,ext] = fileparts(listing(ii).name);

        save(sprintf('%s.mat',name),'iq','fs','fc','dur','bw','gain','bitWidth','sampleStartTime','linkSpeed','fpgaVersion','fwVersion','-v7.3')

        clear iq
        clear fs
        clear fc
        clear dur

        %% Done

        fprintf('%s - Done\n', datetime)
    end

end
