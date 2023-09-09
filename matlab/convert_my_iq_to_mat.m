%% Clear everything out

clc

fprintf('%s - Clearing everything out\n', datestr(now))

clear all
close all

%% Load data

listing = dir('../cpp/');

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

        fc = fread(fid,1,'uint32=>float64');
        bw = fread(fid,1,'uint32=>float64');
        fs = fread(fid,1,'uint32=>float64');
        gain = fread(fid,1,'uint32=>float64');
        numSamples = fread(fid,1,'uint32=>float64');
        timestamp = fread(fid,1,'uint64=>uint64');

        iq = fread(fid,[2,inf],'int16=>int16');

        fclose(fid);

        assert(length(iq) == numSamples)

        %% Compute duration

        dur = length(iq)/fs;

        %% Saving data

        fprintf('%s - Saving I/Q\n', datestr(now))

        [filepath,name,ext] = fileparts(listing(ii).name);

        save(sprintf('%s.mat',name),'iq','fs','fc','dur','bw','gain','timestamp','-v7.3')

        clear iq
        clear fs
        clear fc
        clear dur

        %% Done

        fprintf('%s - Done\n', datestr(now))
    end

end
