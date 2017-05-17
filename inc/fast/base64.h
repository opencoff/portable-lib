/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * base64.h - portable base64 encode/decode routines.
 *
 * This is part of the libb64 project, and has been placed in the public domain.
 * For details, see http://sourceforge.net/projects/libb64
 *
 * Unification of C/C++ api's into one headerfile by Sudhi Herle  <sw at herle.net>
 */

#ifndef __FAST_BASE64_H_1160583589__
#define __FAST_BASE64_H_1160583589__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


typedef enum
{
	step_A, step_B, step_C
} base64_encodestep;

typedef enum
{
	step_a, step_b, step_c, step_d
} base64_decodestep;


struct base64_encode_state
{
	int stepcount;
	base64_encodestep step;
	char result;
};
typedef struct base64_encode_state base64_encode_state;

struct base64_decode_state
{
	base64_decodestep step;
	char plainchar;
};
typedef struct base64_decode_state base64_decode_state;

extern void base64_encode_init(base64_encode_state* state_in);

extern int base64_encode_block(base64_encode_state*, const char* plaintext_in, int length_in, char* code_out);

extern int base64_encode_finish(base64_encode_state*, char* code_out);

extern int base64_encode_buf(char* outbuf, int buflen, void* input, int inlen);



extern void base64_decode_init(base64_decode_state* state_in);

extern int base64_decode_block(base64_decode_state*, const char* code_in, const int length_in, char* plaintext_out);

#define base64_decode_finish(b) do { } while (0)

extern int base64_decode_buf(void* dest, int destlen, char* src, int srclen);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! __FAST_BASE64_H_1160583589__ */

/* EOF */
