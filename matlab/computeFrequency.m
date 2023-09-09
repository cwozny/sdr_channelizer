function [freq1,freq2] = computeFrequency(t,x,fs)

    zeroCrossing = [];
    zeroCrossingInt = [];

    for ii = 2:length(x)
        if x(ii) > 0 && x(ii-1) < 0
            zeroCrossing = [zeroCrossing; interp1(x(ii-1:ii),t(ii-1:ii),0,'linear')];
            zeroCrossingInt = [zeroCrossingInt; ii];
        end

        if x(ii) < 0 && x(ii-1) > 0
            zeroCrossing = [zeroCrossing; interp1(x(ii-1:ii),t(ii-1:ii),0,'linear')];
            zeroCrossingInt = [zeroCrossingInt; ii];
        end
    end

    if length(zeroCrossing) == 2
        freq1 = 0.5/diff(zeroCrossing);
        freq2 = 0.5/(diff(zeroCrossingInt)/fs);
    else
        freq1 = nan;
        freq2 = nan;
    end

end