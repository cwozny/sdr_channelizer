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

        [s,f,t] = stft(iq,fs);
        mesh(t*1e3,(f+fc)*1e-6,abs(s).^2)
        view(2)

        xlabel("Time (ms)")
        ylabel("Frequency (MHz)")

        title(filename,'Interpreter','none')

        saveas(hFig,filename,'png')

        close(hFig)
    end
end
