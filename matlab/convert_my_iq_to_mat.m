%% Clear everything out

clc

fprintf('%s - Clearing everything out\n', datetime)

clearvars
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

    waitbar(ii/length(files),hWait,sprintf('%1.1f%% - %d/%d - Reading %s', 100*ii/length(files), ii, length(files), strrep(files(ii),'_','\_')));

    fid = fopen(fullfile(indir,files(ii)),"r");

    endianness = fread(fid,1,'uint32=>uint32');

    switch endianness
        case 0x00000000
            fprintf('%s - Reading in big endian file\n', datetime)
            fileFormat = 2; % assume latest file format (this is a gap since you can't specify a file format & big endian)
        case 0x01010101
            fprintf('%s - Reading in little endian file\n', datetime)
            fileFormat = 1;
        case 0x02020202
            fprintf('%s - Reading in little endian file\n', datetime)
            fileFormat = 2;
        otherwise
            error('%s - Unsupported endianness (0x%X)\n', datetime, endianness)
    end

    fprintf('%s - File format is %d\n', datetime, fileFormat)

    linkSpeed = fread(fid,1,'uint32=>uint32');

    if fileFormat == 1
        warning('File format 1 doesn''t interpret frequencies above 2^32 Hz correctly');
        fc = fread(fid,1,'uint32=>float64');
    elseif fileFormat == 2
        fc = fread(fid,1,'uint64=>float64');
    end

    bw = fread(fid,1,'uint32=>float64');
    fs = fread(fid,1,'uint32=>float64');
    gain = fread(fid,1,'uint32=>float64');
    numSamples = fread(fid,1,'uint32=>float64');
    bitWidth = fread(fid,1,'uint32=>float64');

    if fileFormat == 2
        spare0 = fread(fid,1,'uint32=>float64');
    end

    boardName = strip(string(fread(fid,16,'*char')'),char(0));
    serialNo = strip(string(fread(fid,16,'*char')'),char(0));
    fpgaVersion = strip(string(fread(fid,16,'*char')'),char(0));
    fwVersion = strip(string(fread(fid,16,'*char')'),char(0));
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

    waitbar(ii/length(files),hWait,sprintf('%1.1f%% - %d/%d - Saving %s', 100*ii/length(files), ii, length(files), strrep(filename,'_','\_')));

    save(fullfile(outdir,filename),'iq','fs','fc','dur','bw','gain','bitWidth','sampleStartTime','linkSpeed','boardName','serialNo','fpgaVersion','fwVersion','-v7.3')

    clear iq
    clear fs
    clear fc
    clear dur

    %% Done

    fprintf('%s - Done\n', datetime)

end

close(hWait);

