#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "hash_drgb.h"

#define DSC_BITS 6
#define DSC_CHANNELS 48

#define NUM_RUNS 10000000
#define RUNS_PER_TENTH_PERCENT (NUM_RUNS / 1000)

void gen_interleaver_shift_array(BYTE* seed, 
                                 uint32_t seedSize,
                                 int dscBits,
                                 int* shiftArr);

int main()
{
    uint32_t i, j;
    int shiftArr[DSC_BITS];
    BYTE seed[32];
    int randFD = open("/dev/urandom", O_RDONLY);

    uint64_t dscAppearances[DSC_CHANNELS] = {0};
    uint64_t min;

    int percent = 0;

    read(randFD, seed, 32);

    for (i = 1; i < NUM_RUNS; i++)
    {
        /*read(randFD, seed, 32);*/
        _add_byte_arr_int(seed,
                          1, 
                          seed,
                          32);

        gen_interleaver_shift_array(seed, 32,
                                    DSC_BITS, 
                                    shiftArr);

        if (i % RUNS_PER_TENTH_PERCENT == 0)
        {
            percent++;
            printf("%d.%d percent compelete...\n", percent/10, percent%10);
            
        }

        for (j = 0; j < DSC_BITS; j++)
        {
            dscAppearances[shiftArr[j]]++;
        }
    }

    min = 0xFFFFFFFFFFFFFFFF;
    for (j = 0; j < DSC_CHANNELS; j++)
    {
        if (dscAppearances[j] < min)
        {
            min = dscAppearances[j];
        }
    }

    printf("DSC Appearances (Weighted (min=1)): \n");
    for (j = 0; j < DSC_CHANNELS; j++)
    {
        if (j % 16 == 0)
        {
            printf("\n");
        }
        printf("%f, ", ((double)dscAppearances[j] / (double)min));
    }
    printf("\n");

    return 0;
}

void gen_interleaver_shift_array(BYTE* seed, 
                                 uint32_t seedSize,
                                 int dscBits,
                                 int* shiftArr)
{
    Hash_DRGB drgb;
    int i, j;
    int temp;
    uint8_t contains;
    uint8_t drgbBuffer[8];
    uint32_t temp32;
    double tempDouble;

    hash_drgb_init(&drgb,
                   seed, seedSize,
                   NULL, 0,
                   NULL, 0);

    // Determine which dsc will be used w/ no repetitions
    i = 0;
    while (i < dscBits)
    {
        hash_drgb_get_rand(&drgb,
                           drgbBuffer, 8,
                           NULL, 0);

        temp32 = ((uint32_t) drgbBuffer[0])         | 
                 ((uint32_t) (drgbBuffer[1] << 8))  |
                 ((uint32_t) (drgbBuffer[2] << 16)) |
                 ((uint32_t) (drgbBuffer[3] << 24));
        tempDouble = (double)temp32 / (double)0xFFFFFFFF;
        temp = (int)(tempDouble * 48);

        contains = 0;
        for (j = 0; j < i; j++)
        {
            if (shiftArr[j] == temp)
            {
                contains = 1;
                break;
            }
        }

        if (!contains)
        {
            shiftArr[i] = temp;
            i++;
        } 
    }

    // Shuffle the DSC (fisher yates shuffle)
    for (i = 0; i < dscBits; i++)
    {
        hash_drgb_get_rand(&drgb,
                           drgbBuffer, 4,
                           NULL, 0);

        j = drgbBuffer[1] % (dscBits - i) + i; 

        temp = shiftArr[j];
        shiftArr[j] = shiftArr[i];
        shiftArr[i] = temp; 
    }

    hash_drgb_clean(&drgb);
}
