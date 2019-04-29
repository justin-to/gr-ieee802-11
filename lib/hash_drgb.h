#ifndef _hash_drgb_h_
#define _hash_drgb_h_

/**
 * This is the header which defines the API for hash_drgb
 */

#include <stdint.h>

#define SEED_LENGTH     55     /** Byte length of max allowable seed 
                                    *  Note that this is the max seedlen for
                                    * SHA-256 which is being used */
#define RESEED_INTERVAL  0x800000000000 /* 2^48 */

typedef uint8_t BYTE;

typedef enum _status_output
{
    SUCCESS = 0,
    ERROR = 1
} Status;

/* hash_drgb data structure */
typedef struct _hash_drgb_data {
    uint64_t reseedCounter;
    BYTE* currentValue;
    BYTE* constValue;
} Hash_DRGB;

/** hash_df
 * The function which performs the hash of the input with an output of
 *      the given number of bytes
 * Inputs:
 *      inputStr - The input byte string to hash
 *      inputStrLen - The byte length of inputStr
 *      bytesToReturn - The number of bytes to return
 *      outputString - The buffer to write the bit string to
 *                      Must have at least bytesToReturn bytes of space
 *      frontAppendByte - The byte to append to front of inputStr, NULL if NSB
 * Output:
 *      SUCCESS - If the value is properly set
 *      ERROR - If one of inputs was improper
 */
Status hash_df(BYTE* inputStr, 
               uint32_t inputStrLen,
               uint32_t bytesToReturn,
               BYTE* outputString,
               BYTE* frontAppendByte);

/** hash_drgb_init
 * Initializes a new instance of a Hash_DRGB with the specified values
 * Inputs:
 *      drgb - The Hash_DRGB struct to store the working condition in
 *      seedInput - The entropy source input 
 *      seedInputLen - Byte length of seedInput
 *      nonce - The nonce applied for the initial seed
 *      nonceInputLen - Byte length of the nonce
 *      personalizationString - The persionalization string for initial seed
 *      perStrLen - Byte length of personalization string
 * Allocates internal space:
 *      drgb->currentValue
 *      drgb->constValue
 */
void hash_drgb_init(Hash_DRGB* drgb, 
                    const BYTE* seedInput, 
                    uint32_t seedInputLen,
                    BYTE* nonce, 
                    uint32_t nonceInputLen,
                    BYTE* personalizationString,
                    uint32_t perStrLen);

/** hash_drgb_reseed
 * Reseeds the instance of a Hash_DRGB with the specified values
 * Inputs:
 *      drgb - The Hash_DRGB struct to store the working condition in
 *      seedInput - The entropy source input 
 *      seedInputLen - Byte length of seedInput
 *      additionalStr - The additional string for reseed
 *      additionalStrLen - Byte length of additional string
 */
void hash_drgb_reseed(Hash_DRGB* drgb,
                      BYTE* seedInput,
                      uint32_t seedInputLen,
                      BYTE* additionalStr,
                      uint32_t additionalStrLen);

/** hash_drgb_reseed
 * Gets the next random set from the instance of the drgb
 * Inputs:
 *      drgb - The Hash_DRGB struct to store the working condition in
 *      buffer - The buffer to store the random bytes in 
 *      bytes - The number of random bytes to get
 *      additionalStr - The additional string for random generator
 *      additionalStrLen - Byte length of additional string
 */
Status hash_drgb_get_rand(Hash_DRGB* drgb, 
                          BYTE* buffer,
                          uint32_t bytes, 
                          BYTE* additionalStr,
                          uint32_t additionalStrLen);

/** _hash_gen
 * Helper function which does the actual generation of the random values
 */
void _hash_gen(BYTE* buffer,
               uint32_t bytes,
               BYTE* value);

/** _add_byte_arrs
 * Helper function which add two byte arrays
 */
void _add_byte_arrs(BYTE* op1, 
                    BYTE* op2, 
                    BYTE* res,
                    uint32_t size);

/** _add_byte_arr_int
 * Helper function which adds a 64 bit int to the byte array
 */
void _add_byte_arr_int(BYTE* op1, 
                       uint64_t op2, 
                       BYTE* res,
                       uint32_t size);

/** hash_drgb_clean
 * Frees all memory allocated within the drgb
 */
void hash_drgb_clean(Hash_DRGB* drgb);

#endif