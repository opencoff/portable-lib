/* vim: ts=4:sw=4:expandtab:
 *
 * b64_encode.c - c source to a base64 encoding algorithm implementation
 *
 * This is part of the libb64 project, and has been placed in the public domain.
 * For details, see http://sourceforge.net/projects/libb64
 */

#include <errno.h>
#include <assert.h>
#include "utils/base64.h"

const int CHARS_PER_LINE = 72;


static int
base64_encode_value(int value_in)
{
	static const char encoding[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	if (value_in > 63) return '=';
	return encoding[value_in];
}


void
base64_encode_init(base64_encode_state* state_in)
{
	state_in->step      = step_A;
	state_in->result    = 0;
	state_in->stepcount = 0;
}

int
base64_encode_block(base64_encode_state* state_in, const char* plaintext_in,
                     int length_in, char* code_out)
{
    const char* plainchar = plaintext_in;
    const char* const plaintextend = plaintext_in + length_in;
    char* codechar = code_out;
    char result;
    char fragment;

    result = state_in->result;

    switch (state_in->step)
    {
        while (1)
        {
            case step_A:
                if (plainchar == plaintextend)
                {
                    state_in->result = result;
                    state_in->step   = step_A;
                    return codechar - code_out;
                }
                fragment    = *plainchar++;
                result      = (fragment & 0x0fc) >> 2;
                *codechar++ = base64_encode_value(result);
                result      = (fragment & 0x003) << 4;
            case step_B:
                if (plainchar == plaintextend)
                {
                    state_in->result = result;
                    state_in->step   = step_B;
                    return codechar - code_out;
                }
                fragment    = *plainchar++;
                result     |= (fragment & 0x0f0) >> 4;
                *codechar++ = base64_encode_value(result);
                result      = (fragment & 0x00f) << 2;
            case step_C:
                if (plainchar == plaintextend)
                {
                    state_in->result = result;
                    state_in->step   = step_C;
                    return codechar - code_out;
                }
                fragment    = *plainchar++;
                result     |= (fragment & 0x0c0) >> 6;
                *codechar++ = base64_encode_value(result);
                result      = (fragment & 0x03f) >> 0;
                *codechar++ = base64_encode_value(result);

                state_in->stepcount++;
                if (state_in->stepcount == CHARS_PER_LINE/4)
                {
                    *codechar++ = '\n';
                    state_in->stepcount = 0;
                }
        }
    }
    /* control should not reach here */
    return codechar - code_out;
}

int
base64_encode_finish(base64_encode_state* state_in, char* code_out)
{
    char* codechar = code_out;

    switch (state_in->step)
    {
        case step_B:
            *codechar++ = base64_encode_value(state_in->result);
            *codechar++ = '=';
            *codechar++ = '=';
            break;
        case step_C:
            *codechar++ = base64_encode_value(state_in->result);
            *codechar++ = '=';
            break;
        case step_A:
            break;
    }
    *codechar++ = '\n';
    *codechar   = 0;

    return codechar - code_out;
}


int
base64_encode_buf(char* dest, int destlen, void* src, int srclen)
{
    base64_encode_state state;
    int n;

    if (destlen < (2*srclen))
        return -ENOMEM;

    base64_encode_init(&state);
    n = base64_encode_block(&state, (char*)src, srclen, dest);
    assert(n < (destlen+5));
    return n + base64_encode_finish(&state, dest+n);
}

/* EOF */
