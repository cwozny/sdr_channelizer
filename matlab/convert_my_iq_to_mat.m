%% Clear everything out

clc

fprintf('%s - Clearing everything out\n', datetime)

clear all
close all

%% Create list of files to read in

indir = uigetdir('../cpp');

listing = dir(indir);

files = string([]);
idx = 1;

for ii = 1:length(listing)
    if contains(listing(ii).name,'.iq')
        files(idx) = listing(ii).name;
        idx = idx + 1;
    end
end

outdir = uigetdir(indir);

%% Load data

hWait = waitbar(0,'Please wait...','Name','I/Q Parser');

for ii = 1:length(files)

    fprintf('%s - Reading %s\n', datetime, files(ii))

    waitbar(ii/length(files),hWait,sprintf('%1.3f%% - %d/%d - Reading %s', 100*ii/length(files), ii, length(files), strrep(files(ii),'_','\_')));

    fid = fopen(fullfile(indir,files(ii)),"r");

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

    if 0 < bitWidth && bitWidth <= 8
        iq = fread(fid,[2,inf],'int8=>int8');
    elseif 8 < bitWidth && bitWidth <= 16
        iq = fread(fid,[2,inf],'int16=>int16');
    else
        error('Unsupported bit width');
    end

    fclose(fid);

    assert(length(iq) == numSamples)

    %% Compute duration

    dur = length(iq)/fs;

    %% Saving data

    fprintf('%s - Saving I/Q\n', datetime)

    [filepath,name,ext] = fileparts(files(ii));

    filename = sprintf('%s.mat',name);

    waitbar(ii/length(files),hWait,sprintf('%1.3f%% - %d/%d - Saving %s', 100*ii/length(files), ii, length(files), strrep(filename,'_','\_')));

    save(fullfile(outdir,filename),'iq','fs','fc','dur','bw','gain','bitWidth','sampleStartTime','linkSpeed','fpgaVersion','fwVersion','-v7.3')

    clear iq
    clear fs
    clear fc
    clear dur

    %% Done

    fprintf('%s - Done\n', datetime)

end

close(hWait);

