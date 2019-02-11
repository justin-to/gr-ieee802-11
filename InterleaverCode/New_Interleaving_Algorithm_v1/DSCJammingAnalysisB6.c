#include <stdio.h>
#include <stdlib.h>

#include "bitwiseDefinitions.h"
#include "b6Interleaver.h"

#define P2_OFFSET m-32
#define P3_OFFSET m-16
#define P4_OFFSET m
#define P5_OFFSET 1

int numbUnique(int pos[], int* arr);

int main()
{
    int i;
    int pos[b];
    int prevP0 = -1;
    int nUnique;
    int min = 49;
    int prevP1 = -1;


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

        if (pos[1] != prevP1)
        {
            prevP1 = pos[1];
            printf("P1 = %d\n", prevP1);
        }

        nUnique = numbUnique(pos, dscList);

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

int numbUnique(int pos[], int* arr)
{
    static uint64_t* numbUniqueArr;
    uint64_t used;
    int unique;
    int i, j;
    int* arrs[2] = {arr + pos[0], arr + pos[1]};

    if (numbUniqueArr == NULL)
        numbUniqueArr = calloc(sizeof(uint64_t), m * (m-16) * (m-32) * (m-48));

    used = *(numbUniqueArr + pos[2] * P2_OFFSET 
                           + pos[3] * P3_OFFSET
                           + pos[4] * P4_OFFSET
                           + pos[5] * P5_OFFSET);

    /* If this is the first time this combination of positions 2,3,4, and 5 
       are being accessed create the usedBitString */
    if (!used)
    {
        /* Set this bits 'set' bit */
        used |= Bit[63];
        
        /* Parse over each of the positions 2-5 */
        for (i = 2; i < b; i++)
        {
            /* For each position set the bit which corresponds to the DSC */
            for (j = 0; j < 16; j++)
            {
                used |= Bit[*(arr + pos[i] + j)];
            }
        }

        /* Set this value in the tracking array */
        *(numbUniqueArr + pos[2] * P2_OFFSET 
                        + pos[3] * P3_OFFSET
                        + pos[4] * P4_OFFSET
                        + pos[5] * P5_OFFSET) = used;
    }

    /* Add the DSCs used by the pos1 and pos2 sets */
    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < 16; j++)
        {
            used |= Bit[ arrs[i][j] ];
        }
    }

    unique = 0;

    for (i = 1; i <= 48; i++)
    {
        if (Bit[i] & used)
        {
            unique++;
        }
    }

    return unique;
}