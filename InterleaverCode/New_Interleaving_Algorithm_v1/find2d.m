function [row,col] = find2d(arr, val)
%FIND2D Finds the row and col indices of the value within the array
%   Returns for bot values if not in the array

row = 1;
col = 1;

[height, width] = size(arr);

found = false;

while ((row - 1) * width + col) <= height * width
    if (arr(row, col) == val)
        found = true;
        break;
    end
    
    if (col == width)
        row = row + 1;
        col = 1;
    else
        col = col + 1;
    end
end

if (~found)
    row = NaN;
    col = NaN;
end

end