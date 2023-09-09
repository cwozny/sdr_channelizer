%% Clear everything out

clc

fprintf('%s - Clearing everything out\n', datestr(now))

clear all
close all

%% Load data

listing = dir(uigetdir('/'));

for ii = 1:length(listing)

    if contains(listing(ii).name,'.bin')

        fprintf('%s - Reading %s\n', datestr(now), listing(ii).name)

        fileinfo = sscanf(listing(ii).name,"%dM_%d_MHz_%d.bin");

        fs = fileinfo(1)*1e6;
        fc = fileinfo(2);
        
        fid = fopen(listing(ii).name,"r");
        
        iq = fread(fid,[2,inf],'float32=>float32');
%         iq = fread(fid,[2,10],'float32=>float32');
        
        fclose(fid);
        
        %% Convert
        
        fprintf('%s - Convert to I/Q\n', datestr(now))
        
        iq = iq(1,:) + 1j*iq(2,:);

        %% Compute duration

        dur = length(iq)/fs;
        
        %% Saving data
        
        fprintf('%s - Saving I/Q\n', datestr(now))
        
        [filepath,name,ext] = fileparts(listing(ii).name);
        
        save(sprintf('%s.mat',name),'iq','fs','fc','dur','-v7.3')
        
        clear iq
        clear fs
        clear fc
        clear dur
        
        %% Done
        
        fprintf('%s - Done\n', datestr(now))

    end

end
