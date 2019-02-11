clc
clear

b = 2;
m = 48 * b;

interleavedDSC = interleaver(b);

dscList = zeros(1, m);

for i = 1:m
    [dsc, bit] = find2d(interleavedDSC, i);
    
    dscList(i) = dsc;
end

dscList

csvwrite('b2Interleaver.h', dscList);