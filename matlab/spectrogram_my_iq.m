%% Clear everything out

clc

fprintf('%s - Clearing everything out\n', datetime)

clearvars
close all

%% Load data

listing = dir(uigetdir('./'));

for ii = 1:length(listing)

    loaded = false;

    filename = listing(ii).name;

    if contains(filename,'.iq')

        fprintf('%s - Reading %s\n', datetime, listing(ii).name)

        fid = fopen(fullfile(listing(ii).folder,filename),"r");

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
        bitWidth = fread(fid,1,'uint32=>uint32');

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

        filename = strrep(filename,'.iq','');

        loaded = true;
    elseif contains(filename,'.mat')

        fprintf('%s - Loading %s\n', datetime, listing(ii).name)

        load(fullfile(listing(ii).folder,listing(ii).name))

        filename = strrep(filename,'.mat','');

        loaded = true;
    end

    if loaded
        maxVal = double(2^(bitWidth-1));
        iq = double(iq);
        iq = iq/maxVal;
        iq = iq(1,:) + 1j*iq(2,:);

        fprintf('%s - Plotting\n', datetime)

        hFig=figure;

        [s,f,t] = stft(iq,fs,'Window',hamming(768),'OverlapLength',0);
        mesh(t*1e3,(f+fc)*1e-6,abs(s).^2)
        view(2)

        fmin = (fc-fs/2)*1e-6;
        fmax = (fc+fs/2)*1e-6;

        xlabel("Time (ms)")
        ylabel("Frequency (MHz)")
        ylim([fmin fmax])
        colorbar
        %clim([0 1])

        title(filename,'Interpreter','none')

        saveas(hFig,filename,'png')

        close(hFig)
    end
end
