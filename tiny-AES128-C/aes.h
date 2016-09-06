#ifndef _AES_H_
#define _AES_H_

#include <stdint.h>


// #define the macros below to 1/0 to enable/disable the mode of operation.
//
// CBC enables AES128 encryption in CBC-mode of operation and handles 0-padding.
// ECB enables the basic ECB 16-byte block algorithm. Both can be enabled simultaneously.

// The #ifndef-guard allows it to be configured before #include'ing or at compile time.
#ifndef CBC
  #define CBC 1
#endif

#ifndef ECB
  #define ECB 1
#endif


typedef uint8_t aes_state_t[4][4];
typedef struct _aes_t aes_t;

struct _aes_t {
	// state - array holding the intermediate results during decryption.
	aes_state_t* state;

	// The array that stores the round keys.
	uint8_t RoundKey[176];

	// The Key input to the AES Program
	const uint8_t* Key;

#if defined(CBC) && CBC
	// Initial Vector used only for CBC mode
	uint8_t* Iv;
#endif
};


#if defined(ECB) && ECB

void AES128_ECB_encrypt(aes_t *aes, uint8_t* input, const uint8_t* key, uint8_t *output);
void AES128_ECB_decrypt(aes_t *aes, uint8_t* input, const uint8_t* key, uint8_t *output);

#endif // #if defined(ECB) && ECB


#if defined(CBC) && CBC

void AES128_CBC_encrypt_buffer(aes_t *aes, uint8_t* output, uint8_t* input, uint32_t length, const uint8_t* key, const uint8_t* iv);
void AES128_CBC_decrypt_buffer(aes_t *aes, uint8_t* output, uint8_t* input, uint32_t length, const uint8_t* key, const uint8_t* iv);

#endif // #if defined(CBC) && CBC



#endif //_AES_H_
