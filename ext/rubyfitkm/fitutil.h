///////////////////////////////////////////////////////////////////////////////
// The following .FIT software provided may be used with .FIT devices only and
// remains the copyrighted property of Dynastream Innovations Inc.
// The software is being provided on an "as-is" basis and as an accommodation,
// and therefore all warranties, representations, or guarantees of any kind
// (whether express, implied or statutory) including, without limitation,
// warranties of merchantability, non-infringement, or fitness for a particular
// purpose, are specifically disclaimed.
//
// Copyright 2008 Dynastream Innovations Inc.
// All rights reserved. This software may not be reproduced by any means
// without express written approval of Dynastream Innovations Inc.
////////////////////////////////////////////////////////////////////////////////

#if !defined(FITUTIL_H)
#define FITUTIL_H


//
// Calculate the dimension of a constant-sized C array.
//

#define DIM(x) (sizeof(x)/sizeof(x[0]))


//
// C preprocessor games to properly concatentate two symbols.
//

#define CONCAT2(a,b) a ## b
#define CONCAT(a,b) CONCAT2(a,b)


int fit_validate_header(void *buffer, long length);
VALUE fit_pos_to_rb(FIT_SINT32 pos);
VALUE fit_time_to_rb(FIT_UINT32 fit_time);
VALUE fit_uint8_array_to_rb_int_array(FIT_UINT8 *data, long length);
VALUE fit_uint8z_array_to_rb_int_array(FIT_UINT8Z *data, long length);
VALUE fit_uint16_array_to_rb_int_array(FIT_UINT16 *data, long length);
VALUE fit_uint8_array_to_rb_float_array(FIT_UINT8 *data, long length, double scale);
VALUE fit_uint16_array_to_rb_float_array(FIT_UINT16 *data, long length, double scale);
VALUE fit_uint32_array_to_rb_float_array(FIT_UINT32 *data, long length, double scale);
VALUE fit_bool_to_rb_bool(FIT_BOOL flag);


#endif // !defined(FITUTIL_H)
