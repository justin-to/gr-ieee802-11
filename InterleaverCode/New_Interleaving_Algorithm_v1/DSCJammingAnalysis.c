#include <stdio.h>
#include <stdlib.h>

#include "b6Interleaver.h"

int numbUnique(int* arrs[], int numArrs);

int main()
{
    int i;
    int pos[b];    
    int* arrs[b];
    int prevP0 = -1;
    int nUnique;
    int min = 49;

#if b > 4
    int prevP1 = -1;
#endif

    /* Initialize the positions */
    for (i = 0; i < b; i++)
    {
        pos[i] = i * 16;
    }

    /* Look for the minimum number of unique values */
    while ( pos[0] < (m - (16*b) ))
    {
        /* Print statement for monitoring progress */
        if (pos[0] != prevP0)
        {
            prevP0 = pos[0];
            printf("P0 = %d\n", prevP0);
        }

#if b > 4
        if (pos[1] != prevP1)
        {
            prevP1 = pos[1];
            printf("P1 = %d\n", prevP1);
        }
#endif

        for (i = 0; i < b; i++)
        {
            arrs[i] = dscList + pos[i];
        }

        nUnique = numbUnique(arrs, b);

        if (nUnique < min)
        {
            min = nUnique;
        }

        /* Try the next set */
        pos[b-1]++;
        for (i = b-1; i > 0; i--)
        {
            if(pos[i] == m - 16*(b - i))
            {
                pos[i-1]++;
            }
        }
        for (i = 1; i < b; i++)
        {
            if(pos[i] >= m - 16*(b - i))
            {
                pos[i] = pos[i-1] + 16;
            }
        }
    }

    printf("Minimum: %d\n", min);

    return 0;
}

int numbUnique(int* arrs[], int numArrs)
{
    int used[49] = {0};
    int unique;
    int i, j;

    for (i = 0; i < numArrs; i++)
    {
        for (j = 0; j < 16; j++)
        {
            used[arrs[i][j]] = 1;
        }
    }

    unique = 0;

    for (i = 1; i <= 48; i++)
    {
        unique += used[i];
    }

    return unique;
}