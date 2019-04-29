cnl = @(m, a, x) mod(floor(m * x + a), 48);

mValues = 3:0.1:7.9;
aValues = 1:47;
seedValues = 0:47;

% Declaration of variables used to store analysis data
numbGreater8 = 0;
index = 0;
movingAvg = 0;

for m = mValues
    for a = aValues
        for seed = seedValues
            i = 1;
            values = [];
            values(i) = cnl(m, a, seed);
            while 1
                val = cnl(m, a, values(i));
                
                if (any(values == val))
                    break;    
                end
                
                i = i + 1;
                values(i) = val;
            end
            
            fprintf("m = %d, a = %d, seed = %d, Number Unique Values = %d\n", ...
                    m, a, seed, length(values));
            
            if (length(values) >= 8)
                numbGreater8 = numbGreater8 + 1;
            end
            
            movingAvg = (movingAvg * index + length(values)) / (index + 1);
            index = index + 1;
        end
    end
end

numbRuns = length(mValues) * length(aValues) * length(seedValues);
disp(numbGreater8/numbRuns);