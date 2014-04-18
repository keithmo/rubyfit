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

#include "stdio.h"
#include "string.h"
#include "ruby.h"
#include "ruby/dl.h"
#include "math.h"

#include "fit_convert.h"


VALUE mRubyFitKM;

VALUE cFitParser;
VALUE cFitHandler;

VALUE cPrintFun;
VALUE cFileIdFun;
VALUE cCapabilitiesFun;
VALUE cDeviceSettingsFun;
VALUE cUserProfileFun;
VALUE cHrmProfileFun;
VALUE cSdmProfileFun;
VALUE cBikeProfileFun;
VALUE cZonesTargetFun;
VALUE cHrZoneFun;
VALUE cPowerZoneFun;
VALUE cMetZoneFun;
VALUE cSportFun;
VALUE cGoalFun;
VALUE cSessionFun;
VALUE cLapFun;
VALUE cRecordFun;
VALUE cEventFun;
VALUE cDeviceInfoFun;
VALUE cWorkoutFun;
VALUE cWorkoutStepFun;
VALUE cScheduleFun;
VALUE cWeightScaleInfoFun;
VALUE cCourseFun;
VALUE cCoursePointFun;
VALUE cTotalsFun;
VALUE cActivityFun;
VALUE cSoftwareFun;
VALUE cFileCapabilitiesFun;
VALUE cMesgCapabilitiesFun;
VALUE cFieldCapabilitiesFun;
VALUE cFileCreatorFun;
VALUE cBloodPressureFun;
VALUE cSpeedZoneFun;
VALUE cMonitoringFun;
VALUE cHrvFun;
VALUE cLengthFun;
VALUE cMonitoringInfoFun;
VALUE cPadFun;
VALUE cSlaveDeviceFun;
VALUE cCadenceZoneFun;
VALUE cMemoGlobFun;

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
	cFileIdFun = rb_intern("on_file_id");
	cCapabilitiesFun = rb_intern("on_capabilities");
	cDeviceSettingsFun = rb_intern("on_device_settings");
	cUserProfileFun = rb_intern("on_user_profile");
	cHrmProfileFun = rb_intern("on_hrm_profile");
	cSdmProfileFun = rb_intern("on_sdm_profile");
	cBikeProfileFun = rb_intern("on_bike_profile");
	cZonesTargetFun = rb_intern("on_zones_target");
	cHrZoneFun = rb_intern("on_hr_zone");
	cPowerZoneFun = rb_intern("on_power_zone");
	cMetZoneFun = rb_intern("on_met_zone");
	cSportFun = rb_intern("on_sport");
	cGoalFun = rb_intern("on_goal");
	cSessionFun = rb_intern("on_session");
	cLapFun = rb_intern("on_lap");
	cRecordFun = rb_intern("on_record");
	cEventFun = rb_intern("on_event");
	cDeviceInfoFun = rb_intern("on_device_info");
	cWorkoutFun = rb_intern("on_workout");
	cWorkoutStepFun = rb_intern("on_workout_step");
	cScheduleFun = rb_intern("on_schedule");
	cWeightScaleInfoFun = rb_intern("on_weight_scale_info");
	cCourseFun = rb_intern("on_course");
	cCoursePointFun = rb_intern("on_course_point");
	cTotalsFun = rb_intern("on_totals");
	cActivityFun = rb_intern("on_activity");
	cSoftwareFun = rb_intern("on_software");
	cFileCapabilitiesFun = rb_intern("on_file_capabilities");
	cMesgCapabilitiesFun = rb_intern("on_mesg_capabilities");
	cFieldCapabilitiesFun = rb_intern("on_field_capabilities");
	cFileCreatorFun = rb_intern("on_file_creator");
	cBloodPressureFun = rb_intern("on_blood_pressure");
	cSpeedZoneFun = rb_intern("on_speed_zone");
	cMonitoringFun = rb_intern("on_monitoring");
	cHrvFun = rb_intern("on_hrv");
	cLengthFun = rb_intern("on_length");
	cMonitoringInfoFun = rb_intern("on_monitoring_info");
	cPadFun = rb_intern("on_pad");
	cSlaveDeviceFun = rb_intern("on_slave_device");
	cCadenceZoneFun = rb_intern("on_cadence_zone");
	cMemoGlobFun = rb_intern("on_memo_glob");

	return Qnil;
}


static VALUE fit_pos_to_rb(FIT_SINT32 pos) {
	float tmp = pos * (180.0 / pow(2,31));
	tmp -= (tmp > 180.0 ? 360.0 : 0.0);
	return rb_float_new(tmp);
}


static void pass_message(char *msg) {
    if (!rb_respond_to(cFitHandler, cPrintFun)) {
        return;
    }

    rb_funcall(cFitHandler, cPrintFun, 1, rb_str_new2(msg));
}


static void pass_file_id(FIT_FILE_ID_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cFileIdFun)) {
        return;
    }

    VALUE rh = rb_hash_new();

    if (mesg->serial_number != FIT_UINT32Z_INVALID)
        rb_hash_aset(rh, rb_str_new2("serial_number"), UINT2NUM(mesg->serial_number));
    if (mesg->time_created != FIT_DATE_TIME_INVALID)
        rb_hash_aset(rh, rb_str_new2("time_created"), UINT2NUM(mesg->time_created + GARMIN_TIME_OFFSET));
    if (mesg->manufacturer != FIT_MANUFACTURER_INVALID)
        rb_hash_aset(rh, rb_str_new2("manufacturer"), UINT2NUM(mesg->manufacturer));
    if (mesg->product != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("product"), UINT2NUM(mesg->product));
    if (mesg->number != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("number"), UINT2NUM(mesg->number));
    if (mesg->type != FIT_FILE_INVALID)
        rb_hash_aset(rh, rb_str_new2("type"), UINT2NUM(mesg->type));

    rb_funcall(cFitHandler, cFileIdFun, 1, rh);
}


static void pass_capabilities(FIT_CAPABILITIES_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cCapabilitiesFun)) {
        return;
    }

    VALUE rh = rb_hash_new();
    VALUE raLanguages = rb_ary_new();
    VALUE raSports = rb_ary_new();
    int i;

    for (i = 0 ; i < FIT_CAPABILITIES_MESG_LANGUAGES_COUNT ; i++) {
        if (mesg->languages[i] != FIT_UINT8_INVALID)
            rb_ary_store(raLanguages, i, UINT2NUM(mesg->languages[i]));
    }

    for (i = 0 ; i < FIT_CAPABILITIES_MESG_SPORTS_COUNT ; i++) {
        if (mesg->sports[i] != FIT_UINT8_INVALID)
            rb_ary_store(raSports, i, UINT2NUM(mesg->sports[i]));
    }

    rb_hash_aset(rh, rb_str_new2("languages"), raLanguages);
    if (mesg->workouts_supported != FIT_WORKOUT_CAPABILITIES_INVALID)
        rb_hash_aset(rh, rb_str_new2("workouts_supported"), UINT2NUM(mesg->workouts_supported));
    if (mesg->connectivity_supported != FIT_CONNECTIVITY_CAPABILITIES_INVALID)
        rb_hash_aset(rh, rb_str_new2("connectivity_supported"), UINT2NUM(mesg->connectivity_supported));
    rb_hash_aset(rh, rb_str_new2("sports"), raSports);

    rb_funcall(cFitHandler, cCapabilitiesFun, 1, rh);
}


static void pass_device_settings(FIT_DEVICE_SETTINGS_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cDeviceSettingsFun)) {
        return;
    }

    VALUE rh = rb_hash_new();

    if (mesg->utc_offset != FIT_UINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("utc_offset"), UINT2NUM(mesg->utc_offset));

    rb_funcall(cFitHandler, cDeviceSettingsFun, 1, rh);
}


static void pass_user_profile(FIT_USER_PROFILE_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cUserProfileFun)) {
        return;
    }

    VALUE rh = rb_hash_new();
    VALUE raGlobalId = rb_ary_new();
    int i;

    for (i = 0 ; i < FIT_USER_PROFILE_MESG_GLOBAL_ID_COUNT ; i++) {
        if (mesg->global_id[i] != FIT_UINT8_INVALID)
            rb_ary_store(raGlobalId, i, UINT2NUM(mesg->global_id[i]));
    }

    if (mesg->friendly_name[0] != FIT_STRING_INVALID)
        rb_hash_aset(rh, rb_str_new2("friendly_name"), rb_str_new2(mesg->friendly_name));
    if (mesg->message_index != FIT_MESSAGE_INDEX_INVALID)
        rb_hash_aset(rh, rb_str_new2("message_index"), UINT2NUM(mesg->message_index));
    if (mesg->weight != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("weight"), rb_float_new(mesg->weight / 10.0));
    if (mesg->local_id != FIT_USER_LOCAL_ID_INVALID)
        rb_hash_aset(rh, rb_str_new2("local_id"), UINT2NUM(mesg->local_id));
    if (mesg->gender != FIT_GENDER_INVALID)
        rb_hash_aset(rh, rb_str_new2("gender"), UINT2NUM(mesg->gender));
    if (mesg->age != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("age"), UINT2NUM(mesg->age));
    if (mesg->height != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("height"), rb_float_new(mesg->height / 100.0));
    if (mesg->language != FIT_LANGUAGE_INVALID)
        rb_hash_aset(rh, rb_str_new2("language"), UINT2NUM(mesg->language));
    if (mesg->elev_setting != FIT_DISPLAY_MEASURE_INVALID)
        rb_hash_aset(rh, rb_str_new2("elev_setting"), UINT2NUM(mesg->elev_setting));
    if (mesg->weight_setting != FIT_DISPLAY_MEASURE_INVALID)
        rb_hash_aset(rh, rb_str_new2("weight_setting"), UINT2NUM(mesg->weight_setting));
    if (mesg->resting_heart_rate != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("resting_heart_rate"), UINT2NUM(mesg->resting_heart_rate));
    if (mesg->default_max_running_heart_rate != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("default_max_running_heart_rate"), UINT2NUM(mesg->default_max_running_heart_rate));
    if (mesg->default_max_biking_heart_rate != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("default_max_biking_heart_rate"), UINT2NUM(mesg->default_max_biking_heart_rate));
    if (mesg->default_max_heart_rate != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("default_max_heart_rate"), UINT2NUM(mesg->default_max_heart_rate));
    if (mesg->hr_setting != FIT_DISPLAY_HEART_INVALID)
        rb_hash_aset(rh, rb_str_new2("hr_setting"), UINT2NUM(mesg->hr_setting));
    if (mesg->speed_setting != FIT_DISPLAY_MEASURE_INVALID)
        rb_hash_aset(rh, rb_str_new2("speed_setting"), UINT2NUM(mesg->speed_setting));
    if (mesg->dist_setting != FIT_DISPLAY_MEASURE_INVALID)
        rb_hash_aset(rh, rb_str_new2("dist_setting"), UINT2NUM(mesg->dist_setting));
    if (mesg->power_setting != FIT_DISPLAY_POWER_INVALID)
        rb_hash_aset(rh, rb_str_new2("power_setting"), UINT2NUM(mesg->power_setting));
    if (mesg->activity_class != FIT_ACTIVITY_CLASS_INVALID)
        rb_hash_aset(rh, rb_str_new2("activity_class"), UINT2NUM(mesg->activity_class));
    if (mesg->position_setting != FIT_DISPLAY_POSITION_INVALID)
        rb_hash_aset(rh, rb_str_new2("position_setting"), UINT2NUM(mesg->position_setting));
    if (mesg->temperature_setting != FIT_DISPLAY_POSITION_INVALID)
        rb_hash_aset(rh, rb_str_new2("temperature_setting"), UINT2NUM(mesg->temperature_setting));
    rb_hash_aset(rh, rb_str_new2("global_id"), raGlobalId);
    if (mesg->height_setting != FIT_DISPLAY_MEASURE_INVALID)
        rb_hash_aset(rh, rb_str_new2("height_setting"), UINT2NUM(mesg->height_setting));

    rb_funcall(cFitHandler, cUserProfileFun, 1, rh);
}


static void pass_hrm_profile(FIT_HRM_PROFILE_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cHrmProfileFun)) {
        return;
    }

    VALUE rh = rb_hash_new();

    if (mesg->message_index != FIT_MESSAGE_INDEX_INVALID)
        rb_hash_aset(rh, rb_str_new2("message_index"), UINT2NUM(mesg->message_index));
    if (mesg->hrm_ant_id != FIT_UINT16Z_INVALID)
        rb_hash_aset(rh, rb_str_new2("hrm_ant_id"), UINT2NUM(mesg->hrm_ant_id));
    if (mesg->enabled != FIT_BOOL_INVALID)
        rb_hash_aset(rh, rb_str_new2("enabled"), INT2BOOL(mesg->enabled == FIT_BOOL_TRUE));
    if (mesg->log_hrv != FIT_BOOL_INVALID)
        rb_hash_aset(rh, rb_str_new2("log_hrv"), INT2BOOL(mesg->log_hrv == FIT_BOOL_TRUE));
    if (mesg->hrm_ant_id_trans_type != FIT_UINT8Z_INVALID)
        rb_hash_aset(rh, rb_str_new2("hrm_ant_id_trans_type"), UINT2NUM(mesg->hrm_ant_id_trans_type));

    rb_funcall(cFitHandler, cHrmProfileFun, 1, rh);
}


static void pass_sdm_profile(FIT_SDM_PROFILE_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cSdmProfileFun)) {
        return;
    }

    VALUE rh = rb_hash_new();

    if (mesg->odometer != FIT_UINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("odometer"), UINT2NUM(mesg->odometer));
    if (mesg->message_index != FIT_MESSAGE_INDEX_INVALID)
        rb_hash_aset(rh, rb_str_new2("message_index"), UINT2NUM(mesg->message_index));
    if (mesg->sdm_ant_id != FIT_UINT16Z_INVALID)
        rb_hash_aset(rh, rb_str_new2("sdm_ant_id"), UINT2NUM(mesg->sdm_ant_id));
    if (mesg->sdm_cal_factor != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("sdm_cal_factor"), UINT2NUM(mesg->sdm_cal_factor));
    if (mesg->enabled != FIT_BOOL_INVALID)
        rb_hash_aset(rh, rb_str_new2("enabled"), INT2BOOL(mesg->enabled == FIT_BOOL_TRUE));
    if (mesg->speed_source != FIT_BOOL_INVALID)
        rb_hash_aset(rh, rb_str_new2("speed_source"), INT2BOOL(mesg->speed_source == FIT_BOOL_TRUE));
    if (mesg->sdm_ant_id_trans_type != FIT_UINT8Z_INVALID)
        rb_hash_aset(rh, rb_str_new2("sdm_ant_id_trans_type"), UINT2NUM(mesg->sdm_ant_id_trans_type));
    if (mesg->odometer_rollover != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("odometer_rollover"), UINT2NUM(mesg->odometer_rollover));

    rb_funcall(cFitHandler, cSdmProfileFun, 1, rh);
}


static void pass_bike_profile(FIT_BIKE_PROFILE_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cBikeProfileFun)) {
        return;
    }

    VALUE rh = rb_hash_new();

    if (mesg->name[0] != FIT_STRING_INVALID)
        rb_hash_aset(rh, rb_str_new2("name"), rb_str_new2(mesg->name));
    if (mesg->odometer != FIT_UINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("odometer"), UINT2NUM(mesg->odometer));
    if (mesg->message_index != FIT_MESSAGE_INDEX_INVALID)
        rb_hash_aset(rh, rb_str_new2("message_index"), UINT2NUM(mesg->message_index));
    if (mesg->bike_spd_ant_id != FIT_UINT16Z_INVALID)
        rb_hash_aset(rh, rb_str_new2("bike_spd_ant_id"), UINT2NUM(mesg->bike_spd_ant_id));
    if (mesg->bike_cad_ant_id != FIT_UINT16Z_INVALID)
        rb_hash_aset(rh, rb_str_new2("bike_cad_ant_id"), UINT2NUM(mesg->bike_cad_ant_id));
    if (mesg->bike_spdcad_ant_id != FIT_UINT16Z_INVALID)
        rb_hash_aset(rh, rb_str_new2("bike_spdcad_ant_id"), UINT2NUM(mesg->bike_spdcad_ant_id));
    if (mesg->bike_power_ant_id != FIT_UINT16Z_INVALID)
        rb_hash_aset(rh, rb_str_new2("bike_power_ant_id"), UINT2NUM(mesg->bike_power_ant_id));
    if (mesg->custom_wheelsize != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("custom_wheelsize"), UINT2NUM(mesg->custom_wheelsize));
    if (mesg->auto_wheelsize != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("auto_wheelsize"), UINT2NUM(mesg->auto_wheelsize));
    if (mesg->bike_weight != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("bike_weight"), UINT2NUM(mesg->bike_weight));
    if (mesg->power_cal_factor != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("power_cal_factor"), UINT2NUM(mesg->power_cal_factor));
    if (mesg->sport != FIT_SPORT_INVALID)
        rb_hash_aset(rh, rb_str_new2("sport"), UINT2NUM(mesg->sport));
    if (mesg->sub_sport != FIT_SUB_SPORT_INVALID)
        rb_hash_aset(rh, rb_str_new2("sub_sport"), UINT2NUM(mesg->sub_sport));
    if (mesg->auto_wheel_cal != FIT_BOOL_INVALID)
        rb_hash_aset(rh, rb_str_new2("auto_wheel_cal"), INT2BOOL(mesg->auto_wheel_cal == FIT_BOOL_TRUE));
    if (mesg->auto_power_zero != FIT_BOOL_INVALID)
        rb_hash_aset(rh, rb_str_new2("auto_power_zero"), INT2BOOL(mesg->auto_power_zero == FIT_BOOL_TRUE));
    if (mesg->id != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("id"), UINT2NUM(mesg->id));
    if (mesg->spd_enabled != FIT_BOOL_INVALID)
        rb_hash_aset(rh, rb_str_new2("spd_enabled"), INT2BOOL(mesg->spd_enabled == FIT_BOOL_TRUE));
    if (mesg->cad_enabled != FIT_BOOL_INVALID)
        rb_hash_aset(rh, rb_str_new2("cad_enabled"), INT2BOOL(mesg->cad_enabled == FIT_BOOL_TRUE));
    if (mesg->spdcad_enabled != FIT_BOOL_INVALID)
        rb_hash_aset(rh, rb_str_new2("spdcad_enabled"), INT2BOOL(mesg->spdcad_enabled == FIT_BOOL_TRUE));
    if (mesg->power_enabled != FIT_BOOL_INVALID)
        rb_hash_aset(rh, rb_str_new2("power_enabled"), INT2BOOL(mesg->power_enabled == FIT_BOOL_TRUE));
    if (mesg->crank_length != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("crank_length"), UINT2NUM(mesg->crank_length));
    if (mesg->enabled != FIT_BOOL_INVALID)
        rb_hash_aset(rh, rb_str_new2("enabled"), INT2BOOL(mesg->enabled == FIT_BOOL_TRUE));
    if (mesg->bike_spd_ant_id_trans_type != FIT_UINT8Z_INVALID)
        rb_hash_aset(rh, rb_str_new2("bike_spd_ant_id_trans_type"), UINT2NUM(mesg->bike_spd_ant_id_trans_type));
    if (mesg->bike_cad_ant_id_trans_type != FIT_UINT8Z_INVALID)
        rb_hash_aset(rh, rb_str_new2("bike_cad_ant_id_trans_type"), UINT2NUM(mesg->bike_cad_ant_id_trans_type));
    if (mesg->bike_spdcad_ant_id_trans_type != FIT_UINT8Z_INVALID)
        rb_hash_aset(rh, rb_str_new2("bike_spdcad_ant_id_trans_type"), UINT2NUM(mesg->bike_spdcad_ant_id_trans_type));
    if (mesg->bike_power_ant_id_trans_type != FIT_UINT8Z_INVALID)
        rb_hash_aset(rh, rb_str_new2("bike_power_ant_id_trans_type"), UINT2NUM(mesg->bike_power_ant_id_trans_type));
    if (mesg->odometer_rollover != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("odometer_rollover"), UINT2NUM(mesg->odometer_rollover));

    rb_funcall(cFitHandler, cBikeProfileFun, 1, rh);
}


static void pass_zones_target(FIT_ZONES_TARGET_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cZonesTargetFun)) {
        return;
    }

    VALUE rh = rb_hash_new();

    if (mesg->functional_threshold_power != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("functional_threshold_power"), UINT2NUM(mesg->functional_threshold_power));
    if (mesg->max_heart_rate != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("max_heart_rate"), UINT2NUM(mesg->max_heart_rate));
    if (mesg->threshold_heart_rate != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("threshold_heart_rate"), UINT2NUM(mesg->threshold_heart_rate));
    if (mesg->hr_calc_type != FIT_HR_ZONE_CALC_INVALID)
        rb_hash_aset(rh, rb_str_new2("hr_calc_type"), UINT2NUM(mesg->hr_calc_type));
    if (mesg->pwr_calc_type != FIT_PWR_ZONE_CALC_INVALID)
        rb_hash_aset(rh, rb_str_new2("pwr_calc_type"), UINT2NUM(mesg->pwr_calc_type));

    rb_funcall(cFitHandler, cZonesTargetFun, 1, rh);
}


static void pass_hr_zone(FIT_HR_ZONE_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cHrZoneFun)) {
        return;
    }

    VALUE rh = rb_hash_new();

    if (mesg->name[0] != FIT_STRING_INVALID)
        rb_hash_aset(rh, rb_str_new2("name"), rb_str_new2(mesg->name));
    if (mesg->message_index != FIT_MESSAGE_INDEX_INVALID)
        rb_hash_aset(rh, rb_str_new2("message_index"), UINT2NUM(mesg->message_index));
    if (mesg->high_bpm != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("high_bpm"), UINT2NUM(mesg->high_bpm));

    rb_funcall(cFitHandler, cHrZoneFun, 1, rh);
}


static void pass_power_zone(FIT_POWER_ZONE_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cPowerZoneFun)) {
        return;
    }

    VALUE rh = rb_hash_new();

    if (mesg->name[0] != FIT_STRING_INVALID)
        rb_hash_aset(rh, rb_str_new2("name"), rb_str_new2(mesg->name));
    if (mesg->message_index != FIT_MESSAGE_INDEX_INVALID)
        rb_hash_aset(rh, rb_str_new2("message_index"), UINT2NUM(mesg->message_index));
    if (mesg->high_value != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("high_value"), UINT2NUM(mesg->high_value));

    rb_funcall(cFitHandler, cPowerZoneFun, 1, rh);
}


static void pass_met_zone(FIT_MET_ZONE_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cMetZoneFun)) {
        return;
    }

    VALUE rh = rb_hash_new();

    if (mesg->message_index != FIT_MESSAGE_INDEX_INVALID)
        rb_hash_aset(rh, rb_str_new2("message_index"), UINT2NUM(mesg->message_index));
    if (mesg->calories != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("calories"), UINT2NUM(mesg->calories));
    if (mesg->high_bpm != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("high_bpm"), UINT2NUM(mesg->high_bpm));
    if (mesg->fat_calories != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("fat_calories"), UINT2NUM(mesg->fat_calories));

    rb_funcall(cFitHandler, cMetZoneFun, 1, rh);
}


static void pass_sport(FIT_SPORT_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cSportFun)) {
        return;
    }

    VALUE rh = rb_hash_new();

    if (mesg->name[0] != FIT_STRING_INVALID)
        rb_hash_aset(rh, rb_str_new2("name"), rb_str_new2(mesg->name));
    if (mesg->sport != FIT_SPORT_INVALID)
        rb_hash_aset(rh, rb_str_new2("sport"), CHR2FIX(mesg->sport));
    if (mesg->sub_sport != FIT_SUB_SPORT_INVALID)
        rb_hash_aset(rh, rb_str_new2("sub_sport"), CHR2FIX(mesg->sub_sport));

    rb_funcall(cFitHandler, cSportFun, 1, rh);
}


static void pass_goal(FIT_GOAL_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cGoalFun)) {
        return;
    }

    VALUE rh = rb_hash_new();

    if (mesg->start_date != FIT_DATE_TIME_INVALID)
        rb_hash_aset(rh, rb_str_new2("start_date"), UINT2NUM(mesg->start_date + GARMIN_TIME_OFFSET));
    if (mesg->end_date != FIT_DATE_TIME_INVALID)
        rb_hash_aset(rh, rb_str_new2("end_date"), UINT2NUM(mesg->end_date + GARMIN_TIME_OFFSET));
    if (mesg->value != FIT_UINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("value"), UINT2NUM(mesg->value));
    if (mesg->target_value != FIT_UINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("target_value"), UINT2NUM(mesg->target_value));
    if (mesg->message_index != FIT_MESSAGE_INDEX_INVALID)
        rb_hash_aset(rh, rb_str_new2("message_index"), UINT2NUM(mesg->message_index));
    if (mesg->recurrence_value != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("recurrence_value"), UINT2NUM(mesg->recurrence_value));
    if (mesg->sport != FIT_SPORT_INVALID)
        rb_hash_aset(rh, rb_str_new2("sport"), CHR2FIX(mesg->sport));
    if (mesg->sub_sport != FIT_SUB_SPORT_INVALID)
        rb_hash_aset(rh, rb_str_new2("sub_sport"), CHR2FIX(mesg->sub_sport));
    if (mesg->type != FIT_GOAL_INVALID)
        rb_hash_aset(rh, rb_str_new2("type"), CHR2FIX(mesg->type));
    if (mesg->repeat != FIT_BOOL_INVALID)
        rb_hash_aset(rh, rb_str_new2("repeat"), INT2BOOL(mesg->repeat == FIT_BOOL_TRUE));
    if (mesg->recurrence != FIT_GOAL_RECURRENCE_INVALID)
        rb_hash_aset(rh, rb_str_new2("recurrence"), CHR2FIX(mesg->recurrence));
    if (mesg->enabled != FIT_BOOL_INVALID)
        rb_hash_aset(rh, rb_str_new2("enabled"), INT2BOOL(mesg->enabled == FIT_BOOL_TRUE));

    rb_funcall(cFitHandler, cGoalFun, 1, rh);
}


static void pass_session(FIT_SESSION_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cSessionFun)) {
        return;
    }

    VALUE rh = rb_hash_new();

    if (mesg->timestamp != FIT_DATE_TIME_INVALID)
        rb_hash_aset(rh, rb_str_new2("timestamp"), UINT2NUM(mesg->timestamp + GARMIN_TIME_OFFSET));
    if (mesg->start_time != FIT_DATE_TIME_INVALID)
        rb_hash_aset(rh, rb_str_new2("start_time"), UINT2NUM(mesg->start_time + GARMIN_TIME_OFFSET));
    if (mesg->start_position_lat != FIT_SINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("start_position_lat"), fit_pos_to_rb(mesg->start_position_lat));
    if (mesg->start_position_long != FIT_SINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("start_position_long"), fit_pos_to_rb(mesg->start_position_long));
    if (mesg->total_elapsed_time != FIT_UINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("total_elapsed_time"), rb_float_new(mesg->total_elapsed_time / 1000.0));
    if (mesg->total_timer_time != FIT_UINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("total_timer_time"), rb_float_new(mesg->total_timer_time / 1000.0));
    if (mesg->total_distance != FIT_UINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("total_distance"), rb_float_new(mesg->total_distance / 100.0));
    if (mesg->total_cycles != FIT_UINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("total_cycles"), UINT2NUM(mesg->total_cycles));
    if (mesg->nec_lat != FIT_SINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("nec_lat"), fit_pos_to_rb(mesg->nec_lat));
    if (mesg->nec_long != FIT_SINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("nec_long"), fit_pos_to_rb(mesg->nec_long));
    if (mesg->swc_lat != FIT_SINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("swc_lat"), fit_pos_to_rb(mesg->swc_lat));
    if (mesg->swc_long != FIT_SINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("swc_long"), fit_pos_to_rb(mesg->swc_long));
    if (mesg->message_index != FIT_MESSAGE_INDEX_INVALID)
        rb_hash_aset(rh, rb_str_new2("message_index"), UINT2NUM(mesg->message_index));
    if (mesg->total_calories != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("total_calories"), UINT2NUM(mesg->total_calories));
    if (mesg->total_fat_calories != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("total_fat_calories"), UINT2NUM(mesg->total_fat_calories));
    if (mesg->avg_speed != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("avg_speed"), rb_float_new(mesg->avg_speed / 1000.0));
    if (mesg->max_speed != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("max_speed"), rb_float_new(mesg->max_speed / 1000.0));
    if (mesg->avg_power != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("avg_power"), UINT2NUM(mesg->avg_power));
    if (mesg->max_power != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("max_power"), UINT2NUM(mesg->max_power));
    if (mesg->total_ascent != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("total_ascent"), UINT2NUM(mesg->total_ascent));
    if (mesg->total_descent != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("total_descent"), UINT2NUM(mesg->total_descent));
    if (mesg->first_lap_index != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("first_lap_index"), UINT2NUM(mesg->first_lap_index));
    if (mesg->num_laps != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("num_laps"), UINT2NUM(mesg->num_laps));
    if (mesg->event != FIT_EVENT_INVALID)
        rb_hash_aset(rh, rb_str_new2("event"), CHR2FIX(mesg->event));
    if (mesg->event_type != FIT_EVENT_INVALID)
        rb_hash_aset(rh, rb_str_new2("event_type"), CHR2FIX(mesg->event_type));
    if (mesg->avg_heart_rate != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("avg_heart_rate"), UINT2NUM(mesg->avg_heart_rate));
    if (mesg->max_heart_rate != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("max_heart_rate"), UINT2NUM(mesg->max_heart_rate));
    if (mesg->avg_cadence != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("avg_cadence"), UINT2NUM(mesg->avg_cadence));
    if (mesg->max_cadence != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("max_cadence"), UINT2NUM(mesg->max_cadence));
    if (mesg->sport != FIT_SPORT_INVALID)
        rb_hash_aset(rh, rb_str_new2("sport"), CHR2FIX(mesg->sport));
    if (mesg->sub_sport != FIT_SUB_SPORT_INVALID)
        rb_hash_aset(rh, rb_str_new2("sub_sport"), CHR2FIX(mesg->sub_sport));
    if (mesg->event_group != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("event_group"), UINT2NUM(mesg->event_group));
    if (mesg->total_training_effect != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("total_training_effect"), UINT2NUM(mesg->total_training_effect));

    rb_funcall(cFitHandler, cSessionFun, 1, rh);
}


static void pass_lap(FIT_LAP_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cLapFun)) {
        return;
    }

    VALUE rh = rb_hash_new();

    if (mesg->timestamp != FIT_DATE_TIME_INVALID)
        rb_hash_aset(rh, rb_str_new2("timestamp"), UINT2NUM(mesg->timestamp + GARMIN_TIME_OFFSET));
    if (mesg->start_time != FIT_DATE_TIME_INVALID)
        rb_hash_aset(rh, rb_str_new2("start_time"), UINT2NUM(mesg->start_time + GARMIN_TIME_OFFSET));
    if (mesg->start_position_lat != FIT_SINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("start_position_lat"), fit_pos_to_rb(mesg->start_position_lat));
    if (mesg->start_position_long != FIT_SINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("start_position_long"), fit_pos_to_rb(mesg->start_position_long));
    if (mesg->end_position_lat != FIT_SINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("end_position_lat"), fit_pos_to_rb(mesg->end_position_lat));
    if (mesg->end_position_long != FIT_SINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("end_position_long"), fit_pos_to_rb(mesg->end_position_long));
    if (mesg->total_elapsed_time != FIT_UINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("total_elapsed_time"), UINT2NUM(mesg->total_elapsed_time));
    if (mesg->total_timer_time != FIT_UINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("total_timer_time"), rb_float_new(mesg->total_timer_time / 1000.0));
    if (mesg->total_distance != FIT_UINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("total_distance"), rb_float_new(mesg->total_distance / 100.0));
    if (mesg->total_cycles != FIT_UINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("total_cycles"), UINT2NUM(mesg->total_cycles));
    if (mesg->message_index != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("message_index"), UINT2NUM(mesg->message_index));
    if (mesg->total_calories != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("total_calories"), UINT2NUM(mesg->total_calories));
    if (mesg->total_fat_calories != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("total_fat_calories"), UINT2NUM(mesg->total_fat_calories));
    if (mesg->avg_speed != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("avg_speed"), rb_float_new(mesg->avg_speed / 1000.0));
    if (mesg->max_speed != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("max_speed"), rb_float_new(mesg->max_speed / 1000.0));
    if (mesg->avg_power != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("avg_power"), UINT2NUM(mesg->avg_power));
    if (mesg->max_power != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("max_power"), UINT2NUM(mesg->max_power));
    if (mesg->total_ascent != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("total_ascent"), UINT2NUM(mesg->total_ascent));
    if (mesg->total_descent != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("total_descent"), UINT2NUM(mesg->total_descent));
    if (mesg->event != FIT_EVENT_INVALID)
        rb_hash_aset(rh, rb_str_new2("event"), CHR2FIX(mesg->event));
    if (mesg->event_type != FIT_EVENT_INVALID)
        rb_hash_aset(rh, rb_str_new2("event_type"), CHR2FIX(mesg->event_type));
    if (mesg->avg_heart_rate != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("avg_heart_rate"), UINT2NUM(mesg->avg_heart_rate));
    if (mesg->max_heart_rate != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("max_heart_rate"), UINT2NUM(mesg->max_heart_rate));
    if (mesg->avg_cadence != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("avg_cadence"), UINT2NUM(mesg->avg_cadence));
    if (mesg->max_cadence != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("max_cadence"), UINT2NUM(mesg->max_cadence));
    if (mesg->intensity != FIT_INTENSITY_INVALID)
        rb_hash_aset(rh, rb_str_new2("intensity"), CHR2FIX(mesg->intensity));
    if (mesg->lap_trigger != FIT_LAP_TRIGGER_INVALID)
        rb_hash_aset(rh, rb_str_new2("lap_trigger"), CHR2FIX(mesg->lap_trigger));
    if (mesg->sport != FIT_SPORT_INVALID)
        rb_hash_aset(rh, rb_str_new2("sport"), CHR2FIX(mesg->sport));
    if (mesg->event_group != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("event_group"), UINT2NUM(mesg->event_group));

    rb_funcall(cFitHandler, cLapFun, 1, rh);
}


static void pass_record(FIT_RECORD_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cRecordFun)) {
        return;
    }

    VALUE rh = rb_hash_new();

    if (mesg->timestamp != FIT_DATE_TIME_INVALID)
        rb_hash_aset(rh, rb_str_new2("timestamp"), UINT2NUM(mesg->timestamp + GARMIN_TIME_OFFSET));
    if (mesg->position_lat != FIT_SINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("position_lat"), fit_pos_to_rb(mesg->position_lat));
    if (mesg->position_long != FIT_SINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("position_long"), fit_pos_to_rb(mesg->position_long));
    if (mesg->distance != FIT_UINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("distance"), rb_float_new(mesg->distance / 100.0));
    if (mesg->time_from_course != FIT_SINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("time_from_course"), rb_float_new(mesg->time_from_course / 1000.0));
    if (mesg->heart_rate != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("heart_rate"), UINT2NUM(mesg->heart_rate));
    if (mesg->altitude != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("altitude"), rb_float_new(mesg->altitude / 5.0 - 500));
    if (mesg->speed != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("speed"), rb_float_new(mesg->speed / 1000.0));
    if (mesg->grade != FIT_SINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("grade"), rb_float_new(mesg->grade / 100.0));
    if (mesg->power != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("power"), UINT2NUM(mesg->power));
    if (mesg->cadence != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("cadence"), UINT2NUM(mesg->cadence));
    if (mesg->resistance != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("resistance"), UINT2NUM(mesg->resistance));
    if (mesg->cycle_length != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("cycle_length"), UINT2NUM(mesg->cycle_length));
    if (mesg->temperature != FIT_SINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("temperature"), INT2FIX(mesg->temperature));

    rb_funcall(cFitHandler, cRecordFun, 1, rh);
}


static void pass_event(FIT_EVENT_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cEventFun)) {
        return;
    }

    VALUE rh = rb_hash_new();

    if (mesg->timestamp != FIT_DATE_TIME_INVALID)
        rb_hash_aset(rh, rb_str_new2("timestamp"), UINT2NUM(mesg->timestamp + GARMIN_TIME_OFFSET));
    if (mesg->data != FIT_UINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("data"), UINT2NUM(mesg->data));
    if (mesg->data16 != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("data16"), UINT2NUM(mesg->data16));
    if (mesg->timestamp != FIT_EVENT_INVALID)
        rb_hash_aset(rh, rb_str_new2("event"), CHR2FIX(mesg->event));
    if (mesg->timestamp != FIT_EVENT_TYPE_INVALID)
        rb_hash_aset(rh, rb_str_new2("event_type"), CHR2FIX(mesg->event_type));
    if (mesg->event_group != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("event_group"), UINT2NUM(mesg->event_group));

    rb_funcall(cFitHandler, cEventFun, 1, rh);
}


static void pass_device_info(FIT_DEVICE_INFO_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cDeviceInfoFun)) {
        return;
    }

    VALUE rh = rb_hash_new();

    if (mesg->timestamp != FIT_DATE_TIME_INVALID)
        rb_hash_aset(rh, rb_str_new2("timestamp"), UINT2NUM(mesg->timestamp + GARMIN_TIME_OFFSET));
    if (mesg->serial_number != FIT_UINT32Z_INVALID)
        rb_hash_aset(rh, rb_str_new2("serial_number"), UINT2NUM(mesg->serial_number));
    if (mesg->manufacturer != FIT_MANUFACTURER_INVALID)
        rb_hash_aset(rh, rb_str_new2("manufacturer"), UINT2NUM(mesg->manufacturer));
    if (mesg->product != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("product"), UINT2NUM(mesg->product));
    if (mesg->software_version != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("software_version"), UINT2NUM(mesg->software_version));
    if (mesg->battery_voltage != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("battery_voltage"), UINT2NUM(mesg->battery_voltage));
    if (mesg->device_index != FIT_DEVICE_INDEX_INVALID)
        rb_hash_aset(rh, rb_str_new2("device_index"), UINT2NUM(mesg->device_index));
    if (mesg->device_type != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("device_type"), UINT2NUM(mesg->device_type));
    if (mesg->hardware_version != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("hardware_version"), UINT2NUM(mesg->hardware_version));
    if (mesg->battery_status != FIT_BATTERY_STATUS_INVALID)
        rb_hash_aset(rh, rb_str_new2("battery_status"), UINT2NUM(mesg->battery_status));

    rb_funcall(cFitHandler, cDeviceInfoFun, 1, rh);
}


static void pass_workout(FIT_WORKOUT_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cWorkoutFun)) {
        return;
    }

    VALUE rh = rb_hash_new();

    if (mesg->capabilities != FIT_WORKOUT_CAPABILITIES_INVALID)
        rb_hash_aset(rh, rb_str_new2("capabilities"), UINT2NUM(mesg->capabilities));
    if (mesg->wkt_name[0] != FIT_STRING_INVALID)
        rb_hash_aset(rh, rb_str_new2("wkt_name"), rb_str_new2(mesg->wkt_name));
    if (mesg->num_valid_steps != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("num_valid_steps"), UINT2NUM(mesg->num_valid_steps));
    if (mesg->sport != FIT_SPORT_INVALID)
        rb_hash_aset(rh, rb_str_new2("sport"), UINT2NUM(mesg->sport));

    rb_funcall(cFitHandler, cWorkoutFun, 1, rh);
}


static void pass_workout_step(FIT_WORKOUT_STEP_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cWorkoutStepFun)) {
        return;
    }

    VALUE rh = rb_hash_new();

    if (mesg->wkt_step_name[0] != FIT_STRING_INVALID)
        rb_hash_aset(rh, rb_str_new2("wkt_step_name"), rb_str_new2(mesg->wkt_step_name));
    if (mesg->duration_value != FIT_UINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("duration_value"), UINT2NUM(mesg->duration_value));
    if (mesg->target_value != FIT_UINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("target_value"), UINT2NUM(mesg->target_value));
    if (mesg->custom_target_value_low != FIT_UINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("custom_target_value_low"), UINT2NUM(mesg->custom_target_value_low));
    if (mesg->custom_target_value_high != FIT_UINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("custom_target_value_high"), UINT2NUM(mesg->custom_target_value_high));
    if (mesg->message_index != FIT_MESSAGE_INDEX_INVALID)
        rb_hash_aset(rh, rb_str_new2("message_index"), UINT2NUM(mesg->message_index));
    if (mesg->duration_type != FIT_WKT_STEP_DURATION_INVALID)
        rb_hash_aset(rh, rb_str_new2("duration_type"), UINT2NUM(mesg->duration_type));
    if (mesg->target_type != FIT_WKT_STEP_TARGET_INVALID)
        rb_hash_aset(rh, rb_str_new2("target_type"), UINT2NUM(mesg->target_type));
    if (mesg->intensity != FIT_INTENSITY_INVALID)
        rb_hash_aset(rh, rb_str_new2("intensity"), UINT2NUM(mesg->intensity));

    rb_funcall(cFitHandler, cWorkoutStepFun, 1, rh);
}


static void pass_schedule(FIT_SCHEDULE_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cScheduleFun)) {
        return;
    }

    VALUE rh = rb_hash_new();

    if (mesg->serial_number != FIT_UINT32Z_INVALID)
        rb_hash_aset(rh, rb_str_new2("serial_number"), UINT2NUM(mesg->serial_number));
    if (mesg->time_created != FIT_DATE_TIME_INVALID)
        rb_hash_aset(rh, rb_str_new2("time_created"), UINT2NUM(mesg->time_created + GARMIN_TIME_OFFSET));
    if (mesg->scheduled_time != FIT_DATE_TIME_INVALID)
        rb_hash_aset(rh, rb_str_new2("scheduled_time"), rb_float_new(mesg->scheduled_time + GARMIN_TIME_OFFSET));
    if (mesg->manufacturer != FIT_MANUFACTURER_INVALID)
        rb_hash_aset(rh, rb_str_new2("manufacturer"), UINT2NUM(mesg->manufacturer));
    if (mesg->product != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("product"), UINT2NUM(mesg->product));
    if (mesg->completed != FIT_BOOL_INVALID)
        rb_hash_aset(rh, rb_str_new2("completed"), INT2BOOL(mesg->completed == FIT_BOOL_TRUE));
    if (mesg->type != FIT_SCHEDULE_INVALID)
        rb_hash_aset(rh, rb_str_new2("type"), UINT2NUM(mesg->type));

    rb_funcall(cFitHandler, cScheduleFun, 1, rh);
}


static void pass_weight_scale_info(FIT_WEIGHT_SCALE_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cWeightScaleInfoFun)) {
        return;
    }

    VALUE rh = rb_hash_new();

    if (mesg->timestamp != FIT_DATE_TIME_INVALID)
        rb_hash_aset(rh, rb_str_new2("timestamp"), UINT2NUM(mesg->timestamp + GARMIN_TIME_OFFSET));
    if (mesg->weight != FIT_WEIGHT_INVALID)
        rb_hash_aset(rh, rb_str_new2("weight"), rb_float_new(mesg->weight / 100.0));
    if (mesg->percent_fat != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("percent_fat"), rb_float_new(mesg->percent_fat / 100.0));
    if (mesg->percent_hydration != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("percent_hydration"), rb_float_new(mesg->percent_hydration / 100.0));
    if (mesg->visceral_fat_mass != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("visceral_fat_mass"), rb_float_new(mesg->visceral_fat_mass / 100.0));
    if (mesg->bone_mass != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("bone_mass"), rb_float_new(mesg->bone_mass / 100.0));
    if (mesg->muscle_mass != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("muscle_mass"), rb_float_new(mesg->muscle_mass / 100.0));
    if (mesg->basal_met != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("basal_met"), rb_float_new(mesg->basal_met / 4.0));
    if (mesg->active_met != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("active_met"), rb_float_new(mesg->active_met / 4.0));
    if (mesg->physique_rating != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("physique_rating"), rb_float_new(mesg->physique_rating));
    if (mesg->metabolic_age != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("metabolic_age"), rb_float_new(mesg->metabolic_age));
    if (mesg->visceral_fat_rating != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("visceral_fat_rating"), rb_float_new(mesg->visceral_fat_rating));

    rb_funcall(cFitHandler, cWeightScaleInfoFun, 1, rh);
}


static void pass_course(FIT_COURSE_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cCourseFun)) {
        return;
    }

    VALUE rh = rb_hash_new();

    if (mesg->name[0] != FIT_STRING_INVALID)
        rb_hash_aset(rh, rb_str_new2("name"), rb_str_new2(mesg->name));
    if (mesg->capabilities != FIT_COURSE_CAPABILITIES_INVALID)
        rb_hash_aset(rh, rb_str_new2("capabilities"), UINT2NUM(mesg->capabilities));
    if (mesg->sport != FIT_SPORT_INVALID)
        rb_hash_aset(rh, rb_str_new2("sport"), UINT2NUM(mesg->sport));

    rb_funcall(cFitHandler, cCourseFun, 1, rh);
}


static void pass_course_point(FIT_COURSE_POINT_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cCoursePointFun)) {
        return;
    }

    VALUE rh = rb_hash_new();

    if (mesg->timestamp != FIT_DATE_TIME_INVALID)
        rb_hash_aset(rh, rb_str_new2("timestamp"), UINT2NUM(mesg->timestamp + GARMIN_TIME_OFFSET));
    if (mesg->position_lat != FIT_SINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("position_lat"), fit_pos_to_rb(mesg->position_lat));
    if (mesg->position_long != FIT_SINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("position_long"), fit_pos_to_rb(mesg->position_long));
    if (mesg->distance != FIT_UINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("distance"), UINT2NUM(mesg->distance));
    if (mesg->name[0] != FIT_STRING_INVALID)
        rb_hash_aset(rh, rb_str_new2("name"), rb_str_new2(mesg->name));
    if (mesg->message_index != FIT_MESSAGE_INDEX_INVALID)
        rb_hash_aset(rh, rb_str_new2("message_index"), UINT2NUM(mesg->message_index));
    if (mesg->type != FIT_COURSE_POINT_INVALID)
        rb_hash_aset(rh, rb_str_new2("type"), UINT2NUM(mesg->type));

    rb_funcall(cFitHandler, cCoursePointFun, 1, rh);
}


static void pass_totals(FIT_TOTALS_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cTotalsFun)) {
        return;
    }

    VALUE rh = rb_hash_new();

    if (mesg->timestamp != FIT_DATE_TIME_INVALID)
        rb_hash_aset(rh, rb_str_new2("timestamp"), UINT2NUM(mesg->timestamp + GARMIN_TIME_OFFSET));
    if (mesg->timer_time != FIT_UINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("timer_time"), UINT2NUM(mesg->timer_time));
    if (mesg->distance != FIT_UINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("distance"), UINT2NUM(mesg->distance));
    if (mesg->calories != FIT_UINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("calories"), UINT2NUM(mesg->calories));
    if (mesg->elapsed_time != FIT_UINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("elapsed_time"), UINT2NUM(mesg->elapsed_time));
    if (mesg->active_time != FIT_UINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("active_time"), UINT2NUM(mesg->active_time));
    if (mesg->message_index != FIT_MESSAGE_INDEX_INVALID)
        rb_hash_aset(rh, rb_str_new2("message_index"), UINT2NUM(mesg->message_index));
    if (mesg->sessions != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("sessions"), UINT2NUM(mesg->sessions));
    if (mesg->sport != FIT_SPORT_INVALID)
        rb_hash_aset(rh, rb_str_new2("sport"), UINT2NUM(mesg->sport));

    rb_funcall(cFitHandler, cTotalsFun, 1, rh);
}


static void pass_activity(FIT_ACTIVITY_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cActivityFun)) {
        return;
    }

    VALUE rh = rb_hash_new();

    if (mesg->timestamp != FIT_DATE_TIME_INVALID)
        rb_hash_aset(rh, rb_str_new2("timestamp"), UINT2NUM(mesg->timestamp + GARMIN_TIME_OFFSET));
    if (mesg->total_timer_time != FIT_UINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("total_timer_time"), rb_float_new(mesg->total_timer_time / 1000.0));
    if (mesg->local_timestamp != FIT_DATE_TIME_INVALID)
        rb_hash_aset(rh, rb_str_new2("local_timestamp"), rb_float_new(mesg->local_timestamp + GARMIN_TIME_OFFSET));
    if (mesg->num_sessions != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("num_sessions"), UINT2NUM(mesg->num_sessions));
    if (mesg->type != FIT_ENUM_INVALID)
        rb_hash_aset(rh, rb_str_new2("type"), CHR2FIX(mesg->type));
    if (mesg->event != FIT_ENUM_INVALID)
        rb_hash_aset(rh, rb_str_new2("event"), CHR2FIX(mesg->event));
    if (mesg->event_type != FIT_ENUM_INVALID)
        rb_hash_aset(rh, rb_str_new2("event_type"), CHR2FIX(mesg->event_type));
    if (mesg->event_group != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("event_group"), UINT2NUM(mesg->event_group));

    rb_funcall(cFitHandler, cActivityFun, 1, rh);
}


static void pass_software(FIT_SOFTWARE_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cSoftwareFun)) {
        return;
    }

    VALUE rh = rb_hash_new();

    if (mesg->part_number[0] != FIT_STRING_INVALID)
        rb_hash_aset(rh, rb_str_new2("part_number"), rb_str_new2(mesg->part_number));
    if (mesg->message_index != FIT_MESSAGE_INDEX_INVALID)
        rb_hash_aset(rh, rb_str_new2("message_index"), UINT2NUM(mesg->message_index));
    if (mesg->version != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("version"), UINT2NUM(mesg->version));

    rb_funcall(cFitHandler, cSoftwareFun, 1, rh);
}


static void pass_file_capabilities(FIT_FILE_CAPABILITIES_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cFileCapabilitiesFun)) {
        return;
    }

    VALUE rh = rb_hash_new();

    if (mesg->directory[0] != FIT_STRING_INVALID)
        rb_hash_aset(rh, rb_str_new2("directory"), rb_str_new2(mesg->directory));
    if (mesg->max_size != FIT_UINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("max_size"), UINT2NUM(mesg->max_size));
    if (mesg->message_index != FIT_MESSAGE_INDEX_INVALID)
        rb_hash_aset(rh, rb_str_new2("message_index"), UINT2NUM(mesg->message_index));
    if (mesg->max_count != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("max_count"), UINT2NUM(mesg->max_count));
    if (mesg->type != FIT_FILE_INVALID)
        rb_hash_aset(rh, rb_str_new2("type"), UINT2NUM(mesg->type));
    if (mesg->flags != FIT_FILE_FLAGS_INVALID)
        rb_hash_aset(rh, rb_str_new2("flags"), UINT2NUM(mesg->flags));

    rb_funcall(cFitHandler, cFileCapabilitiesFun, 1, rh);
}


static void pass_mesg_capabilities(FIT_MESG_CAPABILITIES_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cMesgCapabilitiesFun)) {
        return;
    }

    VALUE rh = rb_hash_new();

    if (mesg->message_index != FIT_MESSAGE_INDEX_INVALID)
        rb_hash_aset(rh, rb_str_new2("message_index"), UINT2NUM(mesg->message_index));
    if (mesg->mesg_num != FIT_MESG_NUM_INVALID)
        rb_hash_aset(rh, rb_str_new2("mesg_num"), UINT2NUM(mesg->mesg_num));
    if (mesg->count != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("count"), UINT2NUM(mesg->count));
    if (mesg->file != FIT_FILE_INVALID)
        rb_hash_aset(rh, rb_str_new2("file"), UINT2NUM(mesg->file));
    if (mesg->count_type != FIT_MESG_COUNT_INVALID)
        rb_hash_aset(rh, rb_str_new2("count_type"), UINT2NUM(mesg->count_type));

    rb_funcall(cFitHandler, cMesgCapabilitiesFun, 1, rh);
}


static void pass_field_capabilities(FIT_FIELD_CAPABILITIES_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cFieldCapabilitiesFun)) {
        return;
    }

    VALUE rh = rb_hash_new();

    if (mesg->message_index != FIT_MESSAGE_INDEX_INVALID)
        rb_hash_aset(rh, rb_str_new2("message_index"), UINT2NUM(mesg->message_index));
    if (mesg->mesg_num != FIT_MESG_NUM_INVALID)
        rb_hash_aset(rh, rb_str_new2("mesg_num"), UINT2NUM(mesg->mesg_num));
    if (mesg->count != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("count"), UINT2NUM(mesg->count));
    if (mesg->file != FIT_FILE_INVALID)
        rb_hash_aset(rh, rb_str_new2("file"), UINT2NUM(mesg->file));
    if (mesg->field_num != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("field_num"), UINT2NUM(mesg->field_num));

    rb_funcall(cFitHandler, cFieldCapabilitiesFun, 1, rh);
}


static void pass_file_creator(FIT_FILE_CREATOR_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cFileCreatorFun)) {
        return;
    }

    VALUE rh = rb_hash_new();

    if (mesg->software_version != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("software_version"), UINT2NUM(mesg->software_version));
    if (mesg->hardware_version != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("hardware_version"), UINT2NUM(mesg->hardware_version));

    rb_funcall(cFitHandler, cFileCreatorFun, 1, rh);
}


static void pass_blood_pressure(FIT_BLOOD_PRESSURE_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cBloodPressureFun)) {
        return;
    }

    VALUE rh = rb_hash_new();


    if (mesg->timestamp != FIT_DATE_TIME_INVALID)
        rb_hash_aset(rh, rb_str_new2("timestamp"), UINT2NUM(mesg->timestamp + GARMIN_TIME_OFFSET));
    if (mesg->systolic_pressure != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("systolic_pressure"), UINT2NUM(mesg->systolic_pressure));
    if (mesg->diastolic_pressure != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("diastolic_pressure"), UINT2NUM(mesg->diastolic_pressure));
    if (mesg->mean_arterial_pressure != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("mean_arterial_pressure"), UINT2NUM(mesg->mean_arterial_pressure));
    if (mesg->map_3_sample_mean != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("map_3_sample_mean"), UINT2NUM(mesg->map_3_sample_mean));
    if (mesg->map_morning_values != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("map_morning_values"), UINT2NUM(mesg->map_morning_values));
    if (mesg->map_evening_values != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("map_evening_values"), UINT2NUM(mesg->map_evening_values));
    if (mesg->user_profile_index != FIT_MESSAGE_INDEX_INVALID)
        rb_hash_aset(rh, rb_str_new2("user_profile_index"), UINT2NUM(mesg->user_profile_index));
    if (mesg->heart_rate != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("heart_rate"), UINT2NUM(mesg->heart_rate));
    if (mesg->heart_rate_type != FIT_HR_TYPE_INVALID)
        rb_hash_aset(rh, rb_str_new2("heart_rate_type"), UINT2NUM(mesg->heart_rate_type));
    if (mesg->status != FIT_BP_STATUS_INVALID)
        rb_hash_aset(rh, rb_str_new2("status"), UINT2NUM(mesg->status));

    rb_funcall(cFitHandler, cBloodPressureFun, 1, rh);
}


static void pass_speed_zone(FIT_SPEED_ZONE_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cSpeedZoneFun)) {
        return;
    }

    VALUE rh = rb_hash_new();

    if (mesg->name[0] != FIT_STRING_INVALID)
        rb_hash_aset(rh, rb_str_new2("name"), rb_str_new2(mesg->name));
    if (mesg->message_index != FIT_MESSAGE_INDEX_INVALID)
        rb_hash_aset(rh, rb_str_new2("message_index"), UINT2NUM(mesg->message_index));
    if (mesg->high_value != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("high_value"), UINT2NUM(mesg->high_value));

    rb_funcall(cFitHandler, cSpeedZoneFun, 1, rh);
}


static void pass_monitoring(FIT_MONITORING_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cMonitoringFun)) {
        return;
    }

    VALUE rh = rb_hash_new();

    if (mesg->timestamp != FIT_DATE_TIME_INVALID)
        rb_hash_aset(rh, rb_str_new2("timestamp"), UINT2NUM(mesg->timestamp + GARMIN_TIME_OFFSET));
    if (mesg->distance != FIT_UINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("distance"), UINT2NUM(mesg->distance));
    if (mesg->cycles != FIT_UINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("cycles"), UINT2NUM(mesg->cycles));
    if (mesg->active_time != FIT_UINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("active_time"), UINT2NUM(mesg->active_time));
    if (mesg->local_timestamp != FIT_DATE_TIME_INVALID)
        rb_hash_aset(rh, rb_str_new2("local_timestamp"), rb_float_new(mesg->local_timestamp + GARMIN_TIME_OFFSET));
    if (mesg->calories != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("calories"), UINT2NUM(mesg->calories));
    if (mesg->distance_16 != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("distance_16"), UINT2NUM(mesg->distance_16));
    if (mesg->cycles_16 != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("cycles_16"), UINT2NUM(mesg->cycles_16));
    if (mesg->active_time_16 != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("active_time_16"), UINT2NUM(mesg->active_time_16));
    if (mesg->device_index != FIT_DEVICE_INDEX_INVALID)
        rb_hash_aset(rh, rb_str_new2("device_index"), UINT2NUM(mesg->device_index));
    if (mesg->activity_type != FIT_ACTIVITY_TYPE_INVALID)
        rb_hash_aset(rh, rb_str_new2("activity_type"), UINT2NUM(mesg->activity_type));
    if (mesg->activity_subtype != FIT_ACTIVITY_SUBTYPE_INVALID)
        rb_hash_aset(rh, rb_str_new2("activity_subtype"), UINT2NUM(mesg->activity_subtype));

    rb_funcall(cFitHandler, cMonitoringFun, 1, rh);
}


static void pass_hrv(FIT_HRV_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cHrvFun)) {
        return;
    }

    VALUE rh = rb_hash_new();
    VALUE raTime = rb_ary_new();
    int i;

    for (i = 0 ; i < FIT_HRV_MESG_TIME_COUNT ; i++) {
        if (mesg->time[i] != FIT_UINT16_INVALID)
            rb_ary_store(raTime, i, UINT2NUM(mesg->time[i]));
    }

    rb_hash_aset(rh, rb_str_new2("time"), raTime);

    rb_funcall(cFitHandler, cHrvFun, 1, rh);
}


static void pass_length(FIT_LENGTH_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cLengthFun)) {
        return;
    }

    VALUE rh = rb_hash_new();
    VALUE raStrokes = rb_ary_new();
    VALUE raZones = rb_ary_new();
    int i;

    for (i = 0 ; i < FIT_LENGTH_MESG_STROKE_COUNT_COUNT ; i++) {
        if (mesg->stroke_count[i] != FIT_UINT16_INVALID)
            rb_ary_store(raStrokes, i, UINT2NUM(mesg->stroke_count[i]));
    }

    for (i = 0 ; i < FIT_LENGTH_MESG_ZONE_COUNT_COUNT ; i++) {
        if (mesg->zone_count[i] != FIT_UINT16_INVALID)
            rb_ary_store(raZones, i, UINT2NUM(mesg->zone_count[i]));
    }

    if (mesg->timestamp != FIT_DATE_TIME_INVALID)
        rb_hash_aset(rh, rb_str_new2("timestamp"), UINT2NUM(mesg->timestamp + GARMIN_TIME_OFFSET));
    if (mesg->start_time != FIT_DATE_TIME_INVALID)
        rb_hash_aset(rh, rb_str_new2("start_time"), UINT2NUM(mesg->start_time + GARMIN_TIME_OFFSET));
    if (mesg->total_elapsed_time != FIT_UINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("total_elapsed_time"), UINT2NUM(mesg->total_elapsed_time));
    if (mesg->total_timer_time != FIT_UINT32_INVALID)
        rb_hash_aset(rh, rb_str_new2("total_timer_time"), UINT2NUM(mesg->total_timer_time));
    if (mesg->message_index != FIT_MESSAGE_INDEX_INVALID)
        rb_hash_aset(rh, rb_str_new2("message_index"), UINT2NUM(mesg->message_index));
    if (mesg->total_strokes != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("total_strokes"), UINT2NUM(mesg->total_strokes));
    if (mesg->avg_speed != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("avg_speed"), UINT2NUM(mesg->avg_speed));
    if (mesg->total_calories != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("total_calories"), UINT2NUM(mesg->total_calories));
    if (mesg->player_score != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("player_score"), UINT2NUM(mesg->player_score));
    if (mesg->opponent_score != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("opponent_score"), UINT2NUM(mesg->opponent_score));
    rb_hash_aset(rh, rb_str_new2("stroke_count"), raStrokes);
    rb_hash_aset(rh, rb_str_new2("zone_count"), raZones);
    if (mesg->event != FIT_EVENT_INVALID)
        rb_hash_aset(rh, rb_str_new2("event"), UINT2NUM(mesg->event));
    if (mesg->event_type != FIT_EVENT_TYPE_INVALID)
        rb_hash_aset(rh, rb_str_new2("event_type"), UINT2NUM(mesg->event_type));
    if (mesg->swim_stroke != FIT_SWIM_STROKE_INVALID)
        rb_hash_aset(rh, rb_str_new2("swim_stroke"), UINT2NUM(mesg->swim_stroke));
    if (mesg->avg_swimming_cadence != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("avg_swimming_cadence"), UINT2NUM(mesg->avg_swimming_cadence));
    if (mesg->event_group != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("event_group"), UINT2NUM(mesg->event_group));
    if (mesg->length_type != FIT_LENGTH_TYPE_INVALID)
        rb_hash_aset(rh, rb_str_new2("length_type"), UINT2NUM(mesg->length_type));

    rb_funcall(cFitHandler, cLengthFun, 1, rh);
}


static void pass_monitoring_info(FIT_MONITORING_INFO_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cMonitoringInfoFun)) {
        return;
    }

    VALUE rh = rb_hash_new();

    if (mesg->timestamp != FIT_DATE_TIME_INVALID)
        rb_hash_aset(rh, rb_str_new2("timestamp"), UINT2NUM(mesg->timestamp + GARMIN_TIME_OFFSET));
    if (mesg->local_timestamp != FIT_DATE_TIME_INVALID)
        rb_hash_aset(rh, rb_str_new2("local_timestamp"), rb_float_new(mesg->local_timestamp + GARMIN_TIME_OFFSET));

    rb_funcall(cFitHandler, cMonitoringInfoFun, 1, rh);
}


static void pass_slave_device(FIT_SLAVE_DEVICE_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cSlaveDeviceFun)) {
        return;
    }

    VALUE rh = rb_hash_new();

    if (mesg->manufacturer != FIT_MANUFACTURER_INVALID)
        rb_hash_aset(rh, rb_str_new2("manufacturer"), UINT2NUM(mesg->manufacturer));
    if (mesg->product != FIT_UINT16_INVALID)
        rb_hash_aset(rh, rb_str_new2("product"), UINT2NUM(mesg->product));

    rb_funcall(cFitHandler, cSlaveDeviceFun, 1, rh);
}


static void pass_cadence_zone(FIT_CADENCE_ZONE_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cCadenceZoneFun)) {
        return;
    }

    VALUE rh = rb_hash_new();

    if (mesg->name[0] != FIT_STRING_INVALID)
        rb_hash_aset(rh, rb_str_new2("name"), rb_str_new2(mesg->name));
    if (mesg->message_index != FIT_MESSAGE_INDEX_INVALID)
        rb_hash_aset(rh, rb_str_new2("message_index"), UINT2NUM(mesg->message_index));
    if (mesg->high_value != FIT_UINT8_INVALID)
        rb_hash_aset(rh, rb_str_new2("high_value"), UINT2NUM(mesg->high_value));

    rb_funcall(cFitHandler, cCadenceZoneFun, 1, rh);
}


static int validate_header(void *buffer, long length) {
    FIT_FILE_HDR *header = (FIT_FILE_HDR *)buffer;

    // Ensure the data length is at least 12 (the minimum header size
    // according to section 3.3.1 of spec revision 1.5) and is also large
    // enough for the header, the data, and the trailing two-byte CRC.

    if (length < 12 || length < ((long)header->header_size + (long)header->data_size + 2)) {
        return 0;
    }

    return 1;
}


static void process_mesg(FIT_UINT16 mesg_num, const FIT_UINT8 *mesg) {
    switch(mesg_num) {
        case FIT_MESG_NUM_FILE_ID:
            pass_file_id((FIT_FILE_ID_MESG *)mesg);
            break;

        case FIT_MESG_NUM_CAPABILITIES:
            pass_capabilities((FIT_CAPABILITIES_MESG *)mesg);
            break;

        case FIT_MESG_NUM_DEVICE_SETTINGS:
            pass_device_settings((FIT_DEVICE_SETTINGS_MESG *)mesg);
            break;

        case FIT_MESG_NUM_USER_PROFILE:
            pass_user_profile((FIT_USER_PROFILE_MESG *)mesg);
            break;

        case FIT_MESG_NUM_HRM_PROFILE:
            pass_hrm_profile((FIT_HRM_PROFILE_MESG *)mesg);
            break;

        case FIT_MESG_NUM_SDM_PROFILE:
            pass_sdm_profile((FIT_SDM_PROFILE_MESG *)mesg);
            break;

        case FIT_MESG_NUM_BIKE_PROFILE:
            pass_bike_profile((FIT_BIKE_PROFILE_MESG *)mesg);
            break;

        case FIT_MESG_NUM_ZONES_TARGET:
            pass_zones_target((FIT_ZONES_TARGET_MESG *)mesg);
            break;

        case FIT_MESG_NUM_HR_ZONE:
            pass_hr_zone((FIT_HR_ZONE_MESG *)mesg);
            break;

        case FIT_MESG_NUM_POWER_ZONE:
            pass_power_zone((FIT_POWER_ZONE_MESG *)mesg);
            break;

        case FIT_MESG_NUM_MET_ZONE:
            pass_met_zone((FIT_MET_ZONE_MESG *)mesg);
            break;

        case FIT_MESG_NUM_SPORT:
            pass_sport((FIT_SPORT_MESG *)mesg);
            break;

        case FIT_MESG_NUM_GOAL:
            pass_goal((FIT_GOAL_MESG *)mesg);
            break;

        case FIT_MESG_NUM_SESSION:
            pass_session((FIT_SESSION_MESG *)mesg);
            break;

        case FIT_MESG_NUM_LAP:
            pass_lap((FIT_LAP_MESG *)mesg);
            break;

        case FIT_MESG_NUM_RECORD:
            pass_record((FIT_RECORD_MESG *)mesg);
            break;

        case FIT_MESG_NUM_EVENT:
            pass_event((FIT_EVENT_MESG *)mesg);
            break;

        case FIT_MESG_NUM_DEVICE_INFO:
            pass_device_info((FIT_DEVICE_INFO_MESG *)mesg);
            break;

        case FIT_MESG_NUM_WORKOUT:
            pass_workout((FIT_WORKOUT_MESG *)mesg);
            break;

        case FIT_MESG_NUM_WORKOUT_STEP:
            pass_workout_step((FIT_WORKOUT_STEP_MESG *)mesg);
            break;

        case FIT_MESG_NUM_SCHEDULE:
            pass_schedule((FIT_SCHEDULE_MESG *)mesg);
            break;

        case FIT_MESG_NUM_WEIGHT_SCALE:
            pass_weight_scale_info((FIT_WEIGHT_SCALE_MESG *)mesg);
            break;

        case FIT_MESG_NUM_COURSE:
            pass_course((FIT_COURSE_MESG *)mesg);
            break;

        case FIT_MESG_NUM_COURSE_POINT:
            pass_course_point((FIT_COURSE_POINT_MESG *)mesg);
            break;

        case FIT_MESG_NUM_TOTALS:
            pass_totals((FIT_TOTALS_MESG *)mesg);
            break;

        case FIT_MESG_NUM_ACTIVITY:
            pass_activity((FIT_ACTIVITY_MESG *)mesg);
            break;

        case FIT_MESG_NUM_SOFTWARE:
            pass_software((FIT_SOFTWARE_MESG *)mesg);
            break;

        case FIT_MESG_NUM_FILE_CAPABILITIES:
            pass_file_capabilities((FIT_FILE_CAPABILITIES_MESG *)mesg);
            break;

        case FIT_MESG_NUM_MESG_CAPABILITIES:
            pass_mesg_capabilities((FIT_MESG_CAPABILITIES_MESG *)mesg);
            break;

        case FIT_MESG_NUM_FIELD_CAPABILITIES:
            pass_field_capabilities((FIT_FIELD_CAPABILITIES_MESG *)mesg);
            break;

        case FIT_MESG_NUM_FILE_CREATOR:
            pass_file_creator((FIT_FILE_CREATOR_MESG *)mesg);
            break;

        case FIT_MESG_NUM_BLOOD_PRESSURE:
            pass_blood_pressure((FIT_BLOOD_PRESSURE_MESG *)mesg);
            break;

        case FIT_MESG_NUM_SPEED_ZONE:
            pass_speed_zone((FIT_SPEED_ZONE_MESG *)mesg);
            break;

        case FIT_MESG_NUM_MONITORING:
            pass_monitoring((FIT_MONITORING_MESG *)mesg);
            break;

        case FIT_MESG_NUM_HRV:
            pass_hrv((FIT_HRV_MESG *)mesg);
            break;

        case FIT_MESG_NUM_LENGTH:
            pass_length((FIT_LENGTH_MESG *)mesg);
            break;

        case FIT_MESG_NUM_MONITORING_INFO:
            pass_monitoring_info((FIT_MONITORING_INFO_MESG *)mesg);
            break;

        case FIT_MESG_NUM_PAD:
            // NYI pass_pad((FIT_PAD_MESG *)mesg);
            break;

        case FIT_MESG_NUM_SLAVE_DEVICE:
            pass_slave_device((FIT_SLAVE_DEVICE_MESG *)mesg);
            break;

        case FIT_MESG_NUM_CADENCE_ZONE:
            pass_cadence_zone((FIT_CADENCE_ZONE_MESG *)mesg);
            break;

        case FIT_MESG_NUM_MEMO_GLOB:
            // NYI pass_memo_glob((FIT_MEMO_GLOB_MESG *)mesg);
            break;
    }
}


static VALUE parse(VALUE self, VALUE original_str) {
    VALUE rstr = StringValue(original_str);
    void *buffer = RSTRING_PTR(rstr);
    long length = RSTRING_LEN(rstr);
	FIT_CONVERT_RETURN convert_return;
	FIT_CONVERT_STATE state;
    const FIT_UINT8 *mesg;
    FIT_UINT16 mesg_num;

    if (!validate_header(buffer, length)) {
		return Qnil;
	}

	FitConvert_Init(&state, FIT_TRUE);
    convert_return = FitConvert_Read(&state, buffer, length);

    if (convert_return != FIT_CONVERT_MESSAGE_AVAILABLE) {
        return Qnil;
    }

    do {
        mesg = FitConvert_GetMessageData(&state);
        mesg_num = FitConvert_GetMessageNumber(&state);

        process_mesg(mesg_num, mesg);
        convert_return = FitConvert_Read(&state, buffer, length);
    } while (convert_return == FIT_CONVERT_MESSAGE_AVAILABLE);    

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
