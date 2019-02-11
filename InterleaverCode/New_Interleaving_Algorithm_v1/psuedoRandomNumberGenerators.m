format compact
clc

%PRN = @(n) mod(floor(3.3 * n + 13), 48)
PRN = @(n) mod(floor(3.1 * n + 13), 48)

n = 14;

count = 0;

values = [];

while ((n > 0) && (count < 288))
    n = PRN(n);
    values = [values n];
    count = count + 1;
end

unsorted = values;
values = sort(unique(values))
unsorted(1:length(values))


diff = zeros(1, length(values)-1);

for i = 1:(length(values)-1)
    diff(i) = unsorted(i+1) - unsorted(i);
end

diff