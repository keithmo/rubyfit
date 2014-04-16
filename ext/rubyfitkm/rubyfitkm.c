////////////////////////////////////////////////////////////////////////////////
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

#include "stdio.h"
#include "string.h"
#include "ruby.h"
#include "math.h"

#include "fit_convert.h"


VALUE mRubyFitKM;
VALUE cFitParser;
VALUE cFitHandler;
VALUE cPrintFun;
int respondsToPrint;
VALUE cHeaderFun;
int respondsToHeader;
VALUE cActivityFun;
int respondsToActivity;
VALUE cRecordFun;
int respondsToRecord;
VALUE cLapFun;
int respondsToLap;
VALUE cSessionFun;
int respondsToSession;
VALUE cDeviceInfoFun;
int respondsToDeviceInfo;
VALUE cUserProfileFun;
int respondsToUserProfile;
VALUE cEventFun;
int respondsToEvent;
VALUE cWeightScaleInfoFun;
int respondsToWeightScaleInfo;
static ID HANDLER_ATTR;


/*
 * garmin/dynastream, decided to pinch pennies on bits by tinkering with well
 * established time offsets.  This is the magic number of seconds needed to add
 * to their number to get the true number of seconds since the epoch.
 * This is 20 years of seconds.
 */
const long GARMIN_TIME_OFFSET = 631065600;


static VALUE init(VALUE self, VALUE handler) {
	cFitHandler = handler;
	rb_ivar_set(self, HANDLER_ATTR, handler);

	//callbacks
	cPrintFun = rb_intern("print_msg");
	cHeaderFun = rb_intern("on_header");
	cActivityFun = rb_intern("on_activity");
	cSessionFun = rb_intern("on_session");
	cLapFun = rb_intern("on_lap");
	cRecordFun = rb_intern("on_record");
	cEventFun = rb_intern("on_event");
	cDeviceInfoFun = rb_intern("on_device_info");
	cUserProfileFun = rb_intern("on_user_profile");
	cWeightScaleInfoFun = rb_intern("on_weight_scale_info");

    respondsToPrint = rb_respond_to(cFitHandler, cPrintFun);
    respondsToHeader = rb_respond_to(cFitHandler, cHeaderFun);
    respondsToActivity = rb_respond_to(cFitHandler, cActivityFun);
    respondsToSession = rb_respond_to(cFitHandler, cSessionFun);
    respondsToLap = rb_respond_to(cFitHandler, cLapFun);
    respondsToRecord = rb_respond_to(cFitHandler, cRecordFun);
    respondsToEvent = rb_respond_to(cFitHandler, cEventFun);
    respondsToDeviceInfo = rb_respond_to(cFitHandler, cDeviceInfoFun);
    respondsToUserProfile = rb_respond_to(cFitHandler, cUserProfileFun);
    respondsToWeightScaleInfo = rb_respond_to(cFitHandler, cWeightScaleInfoFun);

	return Qnil;
}


static VALUE fit_pos_to_rb(FIT_SINT32 pos) {
	float tmp = pos * (180.0 / pow(2,31));
	tmp -= (tmp > 180.0 ? 360.0 : 0.0);
	return rb_float_new(tmp);
}


static void save_message(VALUE rh, void *raw_mesg, long raw_length, void *cooked_mesg, long cooked_length) {
    rb_hash_aset(rh, rb_str_new2("raw_mesg"), rb_str_new((char *)raw_mesg, raw_length));
    rb_hash_aset(rh, rb_str_new2("cooked_mesg"), rb_str_new((char *)cooked_mesg, cooked_length));
}


static void pass_message(char *msg) {
    if (!respondsToPrint) {
        return;
    }

    rb_funcall(cFitHandler, cPrintFun, 1, rb_str_new2(msg));
}


static void pass_header(const FIT_FILE_HDR *header) {
    if (!respondsToHeader) {
        return;
    }

    VALUE rh = rb_hash_new();

    rb_hash_aset(rh, rb_str_new2("header_size"), CHR2FIX(header->header_size));
    rb_hash_aset(rh, rb_str_new2("protocol_version"), CHR2FIX(header->protocol_version));
    rb_hash_aset(rh, rb_str_new2("profile_version"), UINT2NUM(header->profile_version));
    rb_hash_aset(rh, rb_str_new2("data_size"), UINT2NUM(header->data_size));
    rb_hash_aset(rh, rb_str_new2("data_type"), rb_str_new((const char *)header->data_type, 4));
    rb_hash_aset(rh, rb_str_new2("crc"), UINT2NUM(header->crc));
    
    save_message(rh, (void *)header, FIT_FILE_HDR_SIZE, (void *)header, FIT_FILE_HDR_SIZE);
    rb_funcall(cFitHandler, cHeaderFun, 1, rh);
}

static void pass_activity(const FIT_ACTIVITY_MESG *mesg, void *raw_mesg, long raw_length) {
    if (!respondsToActivity) {
        return;
    }

    VALUE rh = rb_hash_new();

    if(mesg->timestamp != FIT_DATE_TIME_INVALID)
        rb_hash_aset(rh, rb_str_new2("timestamp"), UINT2NUM(mesg->timestamp + GARMIN_TIME_OFFSET));
    if(mesg->total_timer_time != FIT_UINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("total_timer_time"), rb_float_new(mesg->total_timer_time / 1000.0));
    if(mesg->local_timestamp != FIT_DATE_TIME_INVALID)
        rb_hash_aset(rh, rb_str_new2("local_timestamp"), rb_float_new(mesg->local_timestamp + GARMIN_TIME_OFFSET));
    if(mesg->num_sessions != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("num_sessions"), UINT2NUM(mesg->num_sessions));
    if(mesg->type != FIT_ENUM_INVALID)
        rb_hash_aset(rh, rb_str_new2("type"), CHR2FIX(mesg->type));
    if(mesg->event != FIT_ENUM_INVALID)
        rb_hash_aset(rh, rb_str_new2("event"), CHR2FIX(mesg->event));
    if(mesg->event_type != FIT_ENUM_INVALID)
        rb_hash_aset(rh, rb_str_new2("event_type"), CHR2FIX(mesg->event_type));
    if(mesg->event_group != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("event_group"), UINT2NUM(mesg->event_group));

    save_message(rh, raw_mesg, raw_length, (void *)mesg, (long)sizeof(*mesg));
    rb_funcall(cFitHandler, cActivityFun, 1, rh);
}


static void pass_record(const FIT_RECORD_MESG *mesg, void *raw_mesg, long raw_length) {
    if (!respondsToRecord) {
        return;
    }

    VALUE rh = rb_hash_new();

    if(mesg->timestamp != FIT_DATE_TIME_INVALID)
        rb_hash_aset(rh, rb_str_new2("timestamp"), UINT2NUM(mesg->timestamp + GARMIN_TIME_OFFSET));
    if(mesg->position_lat != FIT_SINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("position_lat"), fit_pos_to_rb(mesg->position_lat));
    if(mesg->position_long != FIT_SINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("position_long"), fit_pos_to_rb(mesg->position_long));
    if(mesg->distance != FIT_UINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("distance"), rb_float_new(mesg->distance / 100.0));
    if(mesg->time_from_course != FIT_SINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("time_from_course"), rb_float_new(mesg->time_from_course / 1000.0));
    if(mesg->heart_rate != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("heart_rate"), UINT2NUM(mesg->heart_rate));
    if(mesg->altitude != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("altitude"), rb_float_new(mesg->altitude / 5.0 - 500));
    if(mesg->speed != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("speed"), rb_float_new(mesg->speed / 1000.0));
    if(mesg->grade != FIT_SINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("grade"), rb_float_new(mesg->grade / 100.0));
    if(mesg->power != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("power"), UINT2NUM(mesg->power));
    if(mesg->cadence != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("cadence"), UINT2NUM(mesg->cadence));
    if(mesg->resistance != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("resistance"), UINT2NUM(mesg->resistance));
    if(mesg->cycle_length != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("cycle_length"), UINT2NUM(mesg->cycle_length));
    if(mesg->temperature != FIT_SINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("temperature"), INT2FIX(mesg->temperature));

    rb_funcall(cFitHandler, cRecordFun, 1, rh);
}


static void pass_lap(const FIT_LAP_MESG *mesg, void *raw_mesg, long raw_length) {
    if (!respondsToLap) {
        return;
    }

    VALUE rh = rb_hash_new();

    if(mesg->timestamp != FIT_DATE_TIME_INVALID)
        rb_hash_aset(rh, rb_str_new2("timestamp"), UINT2NUM(mesg->timestamp + GARMIN_TIME_OFFSET));
    if(mesg->start_time != FIT_DATE_TIME_INVALID)
        rb_hash_aset(rh, rb_str_new2("start_time"), UINT2NUM(mesg->start_time + GARMIN_TIME_OFFSET));
    if(mesg->start_position_lat != FIT_SINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("start_position_lat"), fit_pos_to_rb(mesg->start_position_lat));
    if(mesg->start_position_long != FIT_SINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("start_position_long"), fit_pos_to_rb(mesg->start_position_long));
    if(mesg->end_position_lat != FIT_SINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("end_position_lat"), fit_pos_to_rb(mesg->end_position_lat));
    if(mesg->end_position_long != FIT_SINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("end_position_long"), fit_pos_to_rb(mesg->end_position_long));
    if(mesg->total_elapsed_time != FIT_UINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("total_elapsed_time"), UINT2NUM(mesg->total_elapsed_time));
    if(mesg->total_timer_time != FIT_UINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("total_timer_time"), rb_float_new(mesg->total_timer_time / 1000.0));
    if(mesg->total_distance != FIT_UINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("total_distance"), rb_float_new(mesg->total_distance / 100.0));
    if(mesg->total_cycles != FIT_UINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("total_cycles"), UINT2NUM(mesg->total_cycles));
    if(mesg->message_index != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("message_index"), UINT2NUM(mesg->message_index));
    if(mesg->total_calories != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("total_calories"), UINT2NUM(mesg->total_calories));
    if(mesg->total_fat_calories != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("total_fat_calories"), UINT2NUM(mesg->total_fat_calories));
    if(mesg->avg_speed != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("avg_speed"), rb_float_new(mesg->avg_speed / 1000.0));
    if(mesg->max_speed != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("max_speed"), rb_float_new(mesg->max_speed / 1000.0));
    if(mesg->avg_power != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("avg_power"), UINT2NUM(mesg->avg_power));
    if(mesg->max_power != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("max_power"), UINT2NUM(mesg->max_power));
    if(mesg->total_ascent != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("total_ascent"), UINT2NUM(mesg->total_ascent));
    if(mesg->total_descent != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("total_descent"), UINT2NUM(mesg->total_descent));
    if(mesg->event != FIT_EVENT_INVALID)
        rb_hash_aset(rh, rb_str_new2("event"), CHR2FIX(mesg->event));
    if(mesg->event_type != FIT_EVENT_INVALID)
        rb_hash_aset(rh, rb_str_new2("event_type"), CHR2FIX(mesg->event_type));
    if(mesg->avg_heart_rate != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("avg_heart_rate"), UINT2NUM(mesg->avg_heart_rate));
    if(mesg->max_heart_rate != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("max_heart_rate"), UINT2NUM(mesg->max_heart_rate));
    if(mesg->avg_cadence != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("avg_cadence"), UINT2NUM(mesg->avg_cadence));
    if(mesg->max_cadence != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("max_cadence"), UINT2NUM(mesg->max_cadence));
    if(mesg->intensity != FIT_INTENSITY_INVALID)
        rb_hash_aset(rh, rb_str_new2("intensity"), CHR2FIX(mesg->intensity));
    if(mesg->lap_trigger != FIT_LAP_TRIGGER_INVALID)
        rb_hash_aset(rh, rb_str_new2("lap_trigger"), CHR2FIX(mesg->lap_trigger));
    if(mesg->sport != FIT_SPORT_INVALID)
        rb_hash_aset(rh, rb_str_new2("sport"), CHR2FIX(mesg->sport));
    if(mesg->event_group != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("event_group"), UINT2NUM(mesg->event_group));

    rb_funcall(cFitHandler, cLapFun, 1, rh);
}


static void pass_session(const FIT_SESSION_MESG *mesg, void *raw_mesg, long raw_length) {
    if (!respondsToSession) {
        return;
    }

    VALUE rh = rb_hash_new();

    if(mesg->timestamp != FIT_DATE_TIME_INVALID)
        rb_hash_aset(rh, rb_str_new2("timestamp"), UINT2NUM(mesg->timestamp + GARMIN_TIME_OFFSET));
    if(mesg->start_time != FIT_DATE_TIME_INVALID)
        rb_hash_aset(rh, rb_str_new2("start_time"), UINT2NUM(mesg->start_time + GARMIN_TIME_OFFSET));
    if(mesg->start_position_lat != FIT_SINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("start_position_lat"), fit_pos_to_rb(mesg->start_position_lat));
    if(mesg->start_position_long != FIT_SINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("start_position_long"), fit_pos_to_rb(mesg->start_position_long));
    if(mesg->total_elapsed_time != FIT_UINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("total_elapsed_time"), rb_float_new(mesg->total_elapsed_time / 1000.0));
    if(mesg->total_timer_time != FIT_UINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("total_timer_time"), rb_float_new(mesg->total_timer_time / 1000.0));
    if(mesg->total_distance != FIT_UINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("total_distance"), rb_float_new(mesg->total_distance / 100.0));
    if(mesg->total_cycles != FIT_UINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("total_cycles"), UINT2NUM(mesg->total_cycles));
    if(mesg->nec_lat != FIT_SINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("nec_lat"), fit_pos_to_rb(mesg->nec_lat));
    if(mesg->nec_long != FIT_SINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("nec_long"), fit_pos_to_rb(mesg->nec_long));
    if(mesg->swc_lat != FIT_SINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("swc_lat"), fit_pos_to_rb(mesg->swc_lat));
    if(mesg->swc_long != FIT_SINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("swc_long"), fit_pos_to_rb(mesg->swc_long));
    if(mesg->message_index != FIT_MESSAGE_INDEX_INVALID)
        rb_hash_aset(rh, rb_str_new2("message_index"), UINT2NUM(mesg->message_index));
    if(mesg->total_calories != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("total_calories"), UINT2NUM(mesg->total_calories));
    if(mesg->total_fat_calories != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("total_fat_calories"), UINT2NUM(mesg->total_fat_calories));
    if(mesg->avg_speed != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("avg_speed"), rb_float_new(mesg->avg_speed / 1000.0));
    if(mesg->max_speed != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("max_speed"), rb_float_new(mesg->max_speed / 1000.0));
    if(mesg->avg_power != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("avg_power"), UINT2NUM(mesg->avg_power));
    if(mesg->max_power != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("max_power"), UINT2NUM(mesg->max_power));
    if(mesg->total_ascent != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("total_ascent"), UINT2NUM(mesg->total_ascent));
    if(mesg->total_descent != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("total_descent"), UINT2NUM(mesg->total_descent));
    if(mesg->first_lap_index != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("first_lap_index"), UINT2NUM(mesg->first_lap_index));
    if(mesg->num_laps != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("num_laps"), UINT2NUM(mesg->num_laps));
    if(mesg->event != FIT_EVENT_INVALID)
        rb_hash_aset(rh, rb_str_new2("event"), CHR2FIX(mesg->event));
    if(mesg->event_type != FIT_EVENT_INVALID)
        rb_hash_aset(rh, rb_str_new2("event_type"), CHR2FIX(mesg->event_type));
    if(mesg->avg_heart_rate != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("avg_heart_rate"), UINT2NUM(mesg->avg_heart_rate));
    if(mesg->max_heart_rate != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("max_heart_rate"), UINT2NUM(mesg->max_heart_rate));
    if(mesg->avg_cadence != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("avg_cadence"), UINT2NUM(mesg->avg_cadence));
    if(mesg->max_cadence != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("max_cadence"), UINT2NUM(mesg->max_cadence));
    if(mesg->sport != FIT_SPORT_INVALID)
        rb_hash_aset(rh, rb_str_new2("sport"), CHR2FIX(mesg->sport));
    if(mesg->sub_sport != FIT_SUB_SPORT_INVALID)
        rb_hash_aset(rh, rb_str_new2("sub_sport"), CHR2FIX(mesg->sub_sport));
    if(mesg->event_group != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("event_group"), UINT2NUM(mesg->event_group));
    if(mesg->total_training_effect != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("total_training_effect"), UINT2NUM(mesg->total_training_effect));

    rb_funcall(cFitHandler, cSessionFun, 1, rh);
}


static void pass_user_profile(const FIT_USER_PROFILE_MESG *mesg, void *raw_mesg, long raw_length) {
    if (!respondsToUserProfile) {
        return;
    }

    VALUE rh = rb_hash_new();

    if(mesg->friendly_name != FIT_STRING_INVALID)
        rb_hash_aset(rh, rb_str_new2("friendly_name"), rb_str_new2(mesg->friendly_name));
    if(mesg->message_index != FIT_MESSAGE_INDEX_INVALID)
        rb_hash_aset(rh, rb_str_new2("message_index"), UINT2NUM(mesg->message_index));
    if(mesg->weight != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("weight"), rb_float_new(mesg->weight / 10.0));
    if(mesg->gender != FIT_GENDER_INVALID)
        rb_hash_aset(rh, rb_str_new2("gender"), UINT2NUM(mesg->gender));
    if(mesg->age != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("age"), UINT2NUM(mesg->age));
    if(mesg->height != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("height"), rb_float_new(mesg->height / 100.0));
    if(mesg->language != FIT_LANGUAGE_INVALID)
        rb_hash_aset(rh, rb_str_new2("language"), UINT2NUM(mesg->language));
    if(mesg->elev_setting != FIT_DISPLAY_MEASURE_INVALID)
        rb_hash_aset(rh, rb_str_new2("elev_setting"), UINT2NUM(mesg->elev_setting));
    if(mesg->weight_setting != FIT_DISPLAY_MEASURE_INVALID)
        rb_hash_aset(rh, rb_str_new2("weight_setting"), UINT2NUM(mesg->weight_setting));
    if(mesg->resting_heart_rate != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("resting_heart_rate"), UINT2NUM(mesg->resting_heart_rate));
    if(mesg->default_max_running_heart_rate != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("default_max_running_heart_rate"), UINT2NUM(mesg->default_max_running_heart_rate));
    if(mesg->default_max_biking_heart_rate != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("default_max_biking_heart_rate"), UINT2NUM(mesg->default_max_biking_heart_rate));
    if(mesg->default_max_heart_rate != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("default_max_heart_rate"), UINT2NUM(mesg->default_max_heart_rate));
    if(mesg->hr_setting != FIT_DISPLAY_HEART_INVALID)
        rb_hash_aset(rh, rb_str_new2("hr_setting"), UINT2NUM(mesg->hr_setting));
    if(mesg->speed_setting != FIT_DISPLAY_MEASURE_INVALID)
        rb_hash_aset(rh, rb_str_new2("speed_setting"), UINT2NUM(mesg->speed_setting));
    if(mesg->dist_setting != FIT_DISPLAY_MEASURE_INVALID)
        rb_hash_aset(rh, rb_str_new2("dist_setting"), UINT2NUM(mesg->dist_setting));
    if(mesg->power_setting != FIT_DISPLAY_POWER_INVALID)
        rb_hash_aset(rh, rb_str_new2("power_setting"), UINT2NUM(mesg->power_setting));
    if(mesg->activity_class != FIT_ACTIVITY_CLASS_INVALID)
        rb_hash_aset(rh, rb_str_new2("activity_class"), UINT2NUM(mesg->activity_class));
    if(mesg->position_setting != FIT_DISPLAY_POSITION_INVALID)
        rb_hash_aset(rh, rb_str_new2("position_setting"), UINT2NUM(mesg->position_setting));

    rb_funcall(cFitHandler, cUserProfileFun, 1, rh);
}


static void pass_event(const FIT_EVENT_MESG *mesg, void *raw_mesg, long raw_length) {
    if (!respondsToEvent) {
        return;
    }

    VALUE rh = rb_hash_new();

    if(mesg->timestamp != FIT_DATE_TIME_INVALID)
        rb_hash_aset(rh, rb_str_new2("timestamp"), UINT2NUM(mesg->timestamp + GARMIN_TIME_OFFSET));
    if(mesg->data != FIT_UINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("data"), UINT2NUM(mesg->data));
    if(mesg->data16 != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("data16"), UINT2NUM(mesg->data16));
    if(mesg->timestamp != FIT_EVENT_INVALID)
        rb_hash_aset(rh, rb_str_new2("event"), CHR2FIX(mesg->event));
    if(mesg->timestamp != FIT_EVENT_TYPE_INVALID)
        rb_hash_aset(rh, rb_str_new2("event_type"), CHR2FIX(mesg->event_type));
    if(mesg->event_group != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("event_group"), UINT2NUM(mesg->event_group));

    rb_funcall(cFitHandler, cEventFun, 1, rh);
}


static void pass_device_info(const FIT_DEVICE_INFO_MESG *mesg, void *raw_mesg, long raw_length) {
    if (!respondsToDeviceInfo) {
        return;
    }

    VALUE rh = rb_hash_new();

    if(mesg->timestamp != FIT_DATE_TIME_INVALID)
        rb_hash_aset(rh, rb_str_new2("timestamp"), UINT2NUM(mesg->timestamp + GARMIN_TIME_OFFSET));
    if(mesg->serial_number != FIT_UINT32Z_INVALID)
        rb_hash_aset(rh, rb_str_new2("serial_number"), UINT2NUM(mesg->serial_number));
    if(mesg->manufacturer != FIT_MANUFACTURER_INVALID)
        rb_hash_aset(rh, rb_str_new2("manufacturer"), UINT2NUM(mesg->manufacturer));
    if(mesg->product != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("product"), UINT2NUM(mesg->product));
    if(mesg->software_version != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("software_version"), UINT2NUM(mesg->software_version));
    if(mesg->battery_voltage != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("battery_voltage"), UINT2NUM(mesg->battery_voltage));
    if(mesg->device_index != FIT_DEVICE_INDEX_INVALID)
        rb_hash_aset(rh, rb_str_new2("device_index"), UINT2NUM(mesg->device_index));
    if(mesg->device_type != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("device_type"), UINT2NUM(mesg->device_type));
    if(mesg->hardware_version != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("hardware_version"), UINT2NUM(mesg->hardware_version));
    if(mesg->battery_status != FIT_BATTERY_STATUS_INVALID)
        rb_hash_aset(rh, rb_str_new2("battery_status"), UINT2NUM(mesg->battery_status));

    rb_funcall(cFitHandler, cDeviceInfoFun, 1, rh);
}


static void pass_weight_scale_info(const FIT_WEIGHT_SCALE_MESG *mesg, void *raw_mesg, long raw_length) {
    if (!respondsToWeightScaleInfo) {
        return;
    }

    VALUE rh = rb_hash_new();

    if(mesg->timestamp != FIT_DATE_TIME_INVALID)
        rb_hash_aset(rh, rb_str_new2("timestamp"), UINT2NUM(mesg->timestamp + GARMIN_TIME_OFFSET));
    if(mesg->weight != FIT_WEIGHT_INVALID)
        rb_hash_aset(rh, rb_str_new2("weight"), rb_float_new(mesg->weight / 100.0));
    if(mesg->percent_fat != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("percent_fat"), rb_float_new(mesg->percent_fat / 100.0));
    if(mesg->percent_hydration != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("percent_hydration"), rb_float_new(mesg->percent_hydration / 100.0));
    if(mesg->visceral_fat_mass != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("visceral_fat_mass"), rb_float_new(mesg->visceral_fat_mass / 100.0));
    if(mesg->bone_mass != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("bone_mass"), rb_float_new(mesg->bone_mass / 100.0));
    if(mesg->muscle_mass != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("muscle_mass"), rb_float_new(mesg->muscle_mass / 100.0));
    if(mesg->basal_met != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("basal_met"), rb_float_new(mesg->basal_met / 4.0));
    if(mesg->active_met != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("active_met"), rb_float_new(mesg->active_met / 4.0));
    if(mesg->physique_rating != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("physique_rating"), rb_float_new(mesg->physique_rating));
    if(mesg->metabolic_age != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("metabolic_age"), rb_float_new(mesg->metabolic_age));
    if(mesg->visceral_fat_rating != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("visceral_fat_rating"), rb_float_new(mesg->visceral_fat_rating));

    rb_funcall(cFitHandler, cWeightScaleInfoFun, 1, rh);
}


static VALUE parse(VALUE self, VALUE original_str) {
	int i = 0;
	VALUE str = StringValue(original_str);
	char *p = RSTRING_PTR(str);
    char *header = p;
	char err_msg[128];

	FIT_UINT8 buf[1];
	FIT_CONVERT_RETURN convert_return = FIT_CONVERT_CONTINUE;
	FIT_UINT32 buf_size;
    void *raw_data;
    long raw_length;
	FIT_CONVERT_STATE state;

	FitConvert_Init(&state, FIT_TRUE);

	if(RSTRING_LEN(str) == 0) {
		//sprintf(err_msg, "Passed in string with length of 0!");
		//pass_message(err_msg);
		return Qnil;
	}

    raw_data = p;

	while(i < RSTRING_LEN(str) && (convert_return == FIT_CONVERT_CONTINUE)) {
		for(buf_size=0;(buf_size < sizeof(buf)) && (p != NULL); buf_size++) {
			buf[buf_size] = *p;
			p++;
			i++;
		}

//        printf("*** buf_size = %lu\n", buf_size);

		do {
			convert_return = FitConvert_Read(&state, buf, buf_size);

			switch(convert_return) {
				case FIT_CONVERT_MESSAGE_AVAILABLE: {
                    raw_length = (long)(p - (char *)raw_data);
					const FIT_UINT8 *mesg = FitConvert_GetMessageData(&state);
					FIT_UINT16 mesg_num = FitConvert_GetMessageNumber(&state);

					//pass_message(err_msg);
                    
                    if (header) {
                        pass_header((FIT_FILE_HDR *)header);
                        header = NULL;
                    }

					switch(mesg_num) {
						case FIT_MESG_NUM_FILE_ID: {
							//pass_message(err_msg);
							break;
						}

						case FIT_MESG_NUM_USER_PROFILE: {
							const FIT_USER_PROFILE_MESG *user_profile = (FIT_USER_PROFILE_MESG *) mesg;
							//sprintf(err_msg, "User Profile: weight=%0.1fkg\n", user_profile->weight / 10.0f);
							//pass_message(err_msg);
							pass_user_profile(user_profile, raw_data, raw_length);
							break;
						}

						case FIT_MESG_NUM_ACTIVITY: {
							const FIT_ACTIVITY_MESG *activity = (FIT_ACTIVITY_MESG *) mesg;
							//sprintf(err_msg, "Activity: timestamp=%u, type=%u, event=%u, event_type=%u, num_sessions=%u\n", activity->timestamp, activity->type, activity->event, activity->event_type, activity->num_sessions);
							//pass_message(err_msg);
							pass_activity(activity, raw_data, raw_length);

							{
								FIT_ACTIVITY_MESG old_mesg;
								old_mesg.num_sessions = 1;
								FitConvert_RestoreFields(&state, &old_mesg);
								//sprintf(err_msg, "Restored num_sessions=1 - Activity: timestamp=%u, type=%u, event=%u, event_type=%u, num_sessions=%u\n", activity->timestamp, activity->type, activity->event, activity->event_type, activity->num_sessions);
								//pass_message(err_msg);
							}
							break;
						}

						case FIT_MESG_NUM_SESSION: {
							const FIT_SESSION_MESG *session = (FIT_SESSION_MESG *) mesg;
							//sprintf(err_msg, "Session: timestamp=%u\n", session->timestamp);
							//pass_message(err_msg);
							pass_session(session, raw_data, raw_length);
							break;
						}

						case FIT_MESG_NUM_LAP: {
							const FIT_LAP_MESG *lap = (FIT_LAP_MESG *) mesg;
							//sprintf(err_msg, "Lap: timestamp=%u, total_ascent=%u, total_distance=%f\n", lap->timestamp, lap->total_ascent, (lap->total_distance / 100.0) / 1000.0 * 0.621371192);
							//pass_message(err_msg);
							pass_lap(lap, raw_data, raw_length);
							break;
						}

						case FIT_MESG_NUM_RECORD: {
							const FIT_RECORD_MESG *record = (FIT_RECORD_MESG *) mesg;

							//sprintf(err_msg, "Record: timestamp=%u", record->timestamp);
							//pass_message(err_msg);
							pass_record(record, raw_data, raw_length);
							break;
						}

						case FIT_MESG_NUM_EVENT: {
							const FIT_EVENT_MESG *event = (FIT_EVENT_MESG *) mesg;
							//sprintf(err_msg, "Event: timestamp=%u, event_type = %i\n", event->timestamp, event->event_type);
							//pass_message(err_msg);
							pass_event(event, raw_data, raw_length);
							break;
						}

						case FIT_MESG_NUM_DEVICE_INFO: {
							const FIT_DEVICE_INFO_MESG *device_info = (FIT_DEVICE_INFO_MESG *) mesg;
							//sprintf(err_msg, "Device Info: timestamp=%u, battery_status=%u\n", (unsigned int)device_info->timestamp, device_info->battery_voltage);
							//pass_message(err_msg);
							pass_device_info(device_info, raw_data, raw_length);
							break;
						}

						case FIT_MESG_NUM_WEIGHT_SCALE: {
							const FIT_WEIGHT_SCALE_MESG *weight_scale_info = (FIT_WEIGHT_SCALE_MESG *) mesg;
							//sprintf(err_msg, "Device Info: timestamp=%u, battery_status=%u\n", (unsigned int)device_info->timestamp, device_info->battery_voltage);
							//pass_message(err_msg);
							pass_weight_scale_info(weight_scale_info, raw_data, raw_length);
							break;
						}

						default: {
							//sprintf(err_msg, "Unknown\n");
							//pass_message(err_msg);
							break;
						}
					}

                    raw_data = p;
					break;
				}
				default:
					break;
			}
		} while (convert_return == FIT_CONVERT_MESSAGE_AVAILABLE);
	}

	if (convert_return == FIT_CONVERT_ERROR) {
		//sprintf(err_msg, "Error decoding file.\n");
		//pass_message(err_msg);
		//fclose(file);
		return Qnil;
	}

	if (convert_return == FIT_CONVERT_CONTINUE) {
		//sprintf(err_msg, "Unexpected end of file.\n");
		//pass_message(err_msg);
		//fclose(file);
		//return Qnil;
	}

	if (convert_return == FIT_CONVERT_PROTOCOL_VERSION_NOT_SUPPORTED) {
		//sprintf(err_msg, "Protocol version not supported.\n");
		//pass_message(err_msg);
		//fclose(file);
		return Qnil;
	}

	if (convert_return == FIT_CONVERT_END_OF_FILE) {
		//sprintf(err_msg, "File converted successfully.\n");
		//pass_message(err_msg);
	}

	return Qnil;
}


void Init_rubyfitkm() {
    mRubyFitKM = rb_define_module("RubyFitKM");
    cFitParser = rb_define_class_under(mRubyFitKM, "FitParser", rb_cObject);

	//instance methods
	rb_define_method(cFitParser, "initialize", init, 1);
	rb_define_method(cFitParser, "parse", parse, 1);

	//attributes
	HANDLER_ATTR = rb_intern("@handler");
	rb_define_attr(cFitParser, "handler", 1, 1);
}
