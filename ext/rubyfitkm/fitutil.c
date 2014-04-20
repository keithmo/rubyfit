#include "ruby.h"
#include "ruby/dl.h"

#include "fit_convert.h"
#include "fitutil.h"


/*
 * garmin/dynastream, decided to pinch pennies on bits by tinkering with well
 * established time offsets.  This is the magic number of seconds needed to add
 * to their number to get the true number of seconds since the epoch.
 * This is 20 years of seconds.
 */
const long GARMIN_TIME_OFFSET = 631065600;


int fit_validate_header(void *buffer, long length) {
    FIT_FILE_HDR *header = (FIT_FILE_HDR *)buffer;

    // Ensure the data length is at least 12 (the minimum header size
    // according to section 3.3.1 of spec revision 1.5) and is also large
    // enough for the header, the data, and the trailing two-byte CRC.

    if (length < 12 || length < ((long)header->header_size + (long)header->data_size + 2)) {
        return 0;
    }

    return 1;
}


VALUE fit_pos_to_rb(FIT_SINT32 pos) {
	float tmp = pos * (180.0 / pow(2,31));
	tmp -= (tmp > 180.0 ? 360.0 : 0.0);
	return rb_float_new(tmp);
}


VALUE fit_time_to_rb(FIT_UINT32 fit_time) {
    return UINT2NUM(fit_time + GARMIN_TIME_OFFSET);
}


VALUE fit_uint8_array_to_rb_int_array(FIT_UINT8 *data, long length) {
    VALUE ra = rb_ary_new();
    int i;

    for (i = 0 ; i < length ; i++) {
        if (data[i] != FIT_UINT8_INVALID) {
            rb_ary_store(ra, i, UINT2NUM(data[i]));
        }
    }

    return ra;
}


VALUE fit_uint8z_array_to_rb_int_array(FIT_UINT8Z *data, long length) {
    VALUE ra = rb_ary_new();
    int i;

    for (i = 0 ; i < length ; i++) {
        if (data[i] != FIT_UINT8Z_INVALID) {
            rb_ary_store(ra, i, UINT2NUM(data[i]));
        }
    }

    return ra;
}


VALUE fit_uint16_array_to_rb_int_array(FIT_UINT16 *data, long length) {
    VALUE ra = rb_ary_new();
    int i;

    for (i = 0 ; i < length ; i++) {
        if (data[i] != FIT_UINT16_INVALID) {
            rb_ary_store(ra, i, UINT2NUM(data[i]));
        }
    }

    return ra;
}


VALUE fit_uint8_array_to_rb_float_array(FIT_UINT8 *data, long length, double scale) {
    VALUE ra = rb_ary_new();
    int i;

    for (i = 0 ; i < length ; i++) {
        if (data[i] != FIT_UINT8_INVALID) {
            rb_ary_store(ra, i, rb_float_new(data[i] / scale));
        }
    }

    return ra;
}


VALUE fit_uint16_array_to_rb_float_array(FIT_UINT16 *data, long length, double scale) {
    VALUE ra = rb_ary_new();
    int i;

    for (i = 0 ; i < length ; i++) {
        if (data[i] != FIT_UINT16_INVALID) {
            rb_ary_store(ra, i, rb_float_new(data[i] / scale));
        }
    }

    return ra;
}


VALUE fit_uint32_array_to_rb_float_array(FIT_UINT32 *data, long length, double scale) {
    VALUE ra = rb_ary_new();
    int i;

    for (i = 0 ; i < length ; i++) {
        if (data[i] != FIT_UINT32_INVALID) {
            rb_ary_store(ra, i, rb_float_new(data[i] / scale));
        }
    }

    return ra;
}


VALUE fit_bool_to_rb_bool(FIT_BOOL flag) {
    return INT2BOOL(flag == FIT_BOOL_TRUE);
}
