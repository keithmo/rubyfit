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
#include "fitutil.h"


VALUE mRubyFitKM;

VALUE cFitParser;
VALUE cFitHandler;

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
VALUE cUnknownFun;

static ID HANDLER_ATTR;


typedef struct {
    unsigned int id;
    const char *name;
} ID_MAP;
#define MAP(suffix, name) { CONCAT(PREFIX,suffix), (name) }

typedef struct {
    ID_MAP *map;
    size_t count;
} ID_MAPS;

#define IS_GARMIN(mfg)  ((mfg) == FIT_MANUFACTURER_GARMIN || (mfg) == FIT_MANUFACTURER_GARMIN_FR405_ANTFS) 

#include "mappers.h"

int compare_map(const void *aa, const void *bb) {
    const ID_MAP *a = (const ID_MAP *)aa;
    const ID_MAP *b = (const ID_MAP *)bb;

    if (a->id == b->id) {
        return 0;
    } else if (a->id < b->id) {
        return -1;
    } else {
        return 1;
    }
}

VALUE id_to_name(ID_MAP *map, size_t count, unsigned int id) {
    ID_MAP key;
    ID_MAP *found;

    key.id = id;
    found = bsearch(&key, map, count, sizeof(ID_MAP), &compare_map);

    return (found == NULL) ? 0 : rb_str_new2(found->name);
}
#define MAPID(map, id) id_to_name(map, DIM(map), (unsigned int)id)

void hash_set(VALUE hash, VALUE _mapped, const char *key, VALUE value, VALUE mapped_value) {
    rb_hash_aset(hash, rb_str_new2(key), value);

    if (mapped_value) {
        rb_hash_aset(_mapped, rb_str_new2(key), mapped_value);
    }
}

void init_maps() {
    size_t i;

    for (i = 0 ; i < DIM(maps) ; i++) {
        qsort(maps[i].map, maps[i].count, sizeof(ID_MAP), compare_map);
    }
}


static void pass_file_id(FIT_FILE_ID_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cFileIdFun)) {
        return;
    }

    VALUE rh = rb_hash_new();
    VALUE _mapped = rb_hash_new();

    if (mesg->serial_number != FIT_UINT32Z_INVALID) {
        hash_set(rh, _mapped, "serial_number", UINT2NUM(mesg->serial_number), 0);
    }

    if (mesg->time_created != FIT_DATE_TIME_INVALID) {
        hash_set(rh, _mapped, "time_created",
            fit_time_to_rb(mesg->time_created),
            fit_time_to_rb_str(mesg->time_created));
    }

    if (mesg->manufacturer != FIT_MANUFACTURER_INVALID) {
        hash_set(rh, _mapped, "manufacturer", UINT2NUM(mesg->manufacturer), MAPID(map_manufacturer, mesg->manufacturer));

    }

    if (mesg->product != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "product", UINT2NUM(mesg->product),
            IS_GARMIN(mesg->manufacturer)
                ? MAPID(map_garmin_product, mesg->product)
                : 0);
    }

    if (mesg->number != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "number", UINT2NUM(mesg->number), 0);
    }

    if (mesg->type != FIT_FILE_INVALID) {
        hash_set(rh, _mapped, "type", UINT2NUM(mesg->type), MAPID(map_file, mesg->type));
    }

    rb_hash_aset(rh, rb_str_new2("_mapped"), _mapped);
    rb_funcall(cFitHandler, cFileIdFun, 1, rh);
}


static void pass_capabilities(FIT_CAPABILITIES_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cCapabilitiesFun)) {
        return;
    }

    VALUE rh = rb_hash_new();
    VALUE _mapped = rb_hash_new();

    if (is_valid_uint8z_array(mesg->languages, DIM(mesg->languages))) {
        hash_set(rh, _mapped, "languages",
            fit_uint8z_array_to_rb_int_array(mesg->languages, DIM(mesg->languages)),
            0);
    }

    if (mesg->workouts_supported != FIT_WORKOUT_CAPABILITIES_INVALID) {
        hash_set(rh, _mapped, "workouts_supported",
            UINT2NUM(mesg->workouts_supported),
            0);
    }

    if (mesg->connectivity_supported != FIT_CONNECTIVITY_CAPABILITIES_INVALID) {
        hash_set(rh, _mapped, "connectivity_supported",
            UINT2NUM(mesg->connectivity_supported),
            0);
    }

    if (is_valid_uint8z_array(mesg->sports, DIM(mesg->sports))) {
        hash_set(rh, _mapped, "sports",
            fit_uint8z_array_to_rb_int_array(mesg->sports, DIM(mesg->sports)),
            0);
    }

    rb_hash_aset(rh, rb_str_new2("_mapped"), _mapped);
    rb_funcall(cFitHandler, cCapabilitiesFun, 1, rh);
}


static void pass_device_settings(FIT_DEVICE_SETTINGS_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cDeviceSettingsFun)) {
        return;
    }

    VALUE rh = rb_hash_new();
    VALUE _mapped = rb_hash_new();

    if (mesg->utc_offset != FIT_UINT32_INVALID) {
        hash_set(rh, _mapped, "utc_offset",
            UINT2NUM(mesg->utc_offset),
            0);
    }

    if (mesg->active_time_zone != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "active_time_zone",
            UINT2NUM(mesg->active_time_zone),
            MAPID(map_time_zone, mesg->active_time_zone));
    }

    if (is_valid_sint8_array(mesg->time_zone_offset, DIM(mesg->time_zone_offset))) {
        hash_set(rh, _mapped, "time_zone_offset",
            fit_sint8_array_to_rb_int_array(mesg->time_zone_offset, DIM(mesg->time_zone_offset)),
            0);
    }

    rb_hash_aset(rh, rb_str_new2("_mapped"), _mapped);
    rb_funcall(cFitHandler, cDeviceSettingsFun, 1, rh);
}


static void pass_user_profile(FIT_USER_PROFILE_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cUserProfileFun)) {
        return;
    }

    VALUE rh = rb_hash_new();
    VALUE _mapped = rb_hash_new();

    if (mesg->friendly_name[0] != FIT_STRING_INVALID) {
        hash_set(rh, _mapped, "friendly_name",
            rb_str_new2(mesg->friendly_name),
            0);
    }

    if (mesg->message_index != FIT_MESSAGE_INDEX_INVALID) {
        hash_set(rh, _mapped, "message_index", UINT2NUM(mesg->message_index), 0);
    }

    if (mesg->weight != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "weight", rb_float_new(mesg->weight / 10.0), 0);
    }

    if (mesg->local_id != FIT_USER_LOCAL_ID_INVALID) {
        hash_set(rh, _mapped, "local_id", UINT2NUM(mesg->local_id), 0);
    }

    if (mesg->gender != FIT_GENDER_INVALID) {
        hash_set(rh, _mapped, "gender", UINT2NUM(mesg->gender), MAPID(map_gender, mesg->gender));
    }

    if (mesg->age != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "age", UINT2NUM(mesg->age), 0);
    }

    if (mesg->height != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "height", rb_float_new(mesg->height / 100.0), 0);
    }

    if (mesg->language != FIT_LANGUAGE_INVALID) {
        hash_set(rh, _mapped, "language", UINT2NUM(mesg->language), MAPID(map_language, mesg->language));
    }

    if (mesg->elev_setting != FIT_DISPLAY_MEASURE_INVALID) {
        hash_set(rh, _mapped, "elev_setting", UINT2NUM(mesg->elev_setting), MAPID(map_display_measure, mesg->elev_setting));
    }

    if (mesg->weight_setting != FIT_DISPLAY_MEASURE_INVALID) {
        hash_set(rh, _mapped, "weight_setting", UINT2NUM(mesg->weight_setting), MAPID(map_display_measure, mesg->weight_setting));
    }

    if (mesg->resting_heart_rate != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "resting_heart_rate", UINT2NUM(mesg->resting_heart_rate), 0);
    }

    if (mesg->default_max_running_heart_rate != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "default_max_running_heart_rate", UINT2NUM(mesg->default_max_running_heart_rate), 0);
    }

    if (mesg->default_max_biking_heart_rate != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "default_max_biking_heart_rate", UINT2NUM(mesg->default_max_biking_heart_rate), 0);
    }

    if (mesg->default_max_heart_rate != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "default_max_heart_rate", UINT2NUM(mesg->default_max_heart_rate), 0);
    }

    if (mesg->hr_setting != FIT_DISPLAY_HEART_INVALID) {
        hash_set(rh, _mapped, "hr_setting", UINT2NUM(mesg->hr_setting), MAPID(map_display_heart, mesg->hr_setting));
    }

    if (mesg->speed_setting != FIT_DISPLAY_MEASURE_INVALID) {
        hash_set(rh, _mapped, "speed_setting", UINT2NUM(mesg->speed_setting), MAPID(map_display_measure, mesg->speed_setting));
    }

    if (mesg->dist_setting != FIT_DISPLAY_MEASURE_INVALID) {
        hash_set(rh, _mapped, "dist_setting", UINT2NUM(mesg->dist_setting), MAPID(map_display_measure, mesg->dist_setting));
    }

    if (mesg->power_setting != FIT_DISPLAY_POWER_INVALID) {
        hash_set(rh, _mapped, "power_setting", UINT2NUM(mesg->power_setting), MAPID(map_display_power, mesg->power_setting));
    }

    if (mesg->activity_class != FIT_ACTIVITY_CLASS_INVALID) {
        hash_set(rh, _mapped, "activity_class", UINT2NUM(mesg->activity_class), MAPID(map_activity_class, mesg->activity_class));
    }

    if (mesg->position_setting != FIT_DISPLAY_POSITION_INVALID) {
        hash_set(rh, _mapped, "position_setting", UINT2NUM(mesg->position_setting), MAPID(map_display_position, mesg->position_setting));
    }

    if (mesg->temperature_setting != FIT_DISPLAY_POSITION_INVALID) {
        hash_set(rh, _mapped, "temperature_setting", UINT2NUM(mesg->temperature_setting), MAPID(map_display_position, mesg->temperature_setting));
    }

    if (is_valid_uint8_array(mesg->global_id, DIM(mesg->global_id))) {
        hash_set(rh, _mapped, "global_id", fit_uint8_array_to_rb_int_array(mesg->global_id, DIM(mesg->global_id)), 0);
    }

    if (mesg->height_setting != FIT_DISPLAY_MEASURE_INVALID) {
        hash_set(rh, _mapped, "height_setting", UINT2NUM(mesg->height_setting), MAPID(map_display_measure, mesg->height_setting));
    }

    rb_hash_aset(rh, rb_str_new2("_mapped"), _mapped);
    rb_funcall(cFitHandler, cUserProfileFun, 1, rh);
}


static void pass_hrm_profile(FIT_HRM_PROFILE_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cHrmProfileFun)) {
        return;
    }

    VALUE rh = rb_hash_new();
    VALUE _mapped = rb_hash_new();

    if (mesg->message_index != FIT_MESSAGE_INDEX_INVALID) {
        hash_set(rh, _mapped, "message_index", UINT2NUM(mesg->message_index), 0);
    }

    if (mesg->hrm_ant_id != FIT_UINT16Z_INVALID) {
        hash_set(rh, _mapped, "hrm_ant_id", UINT2NUM(mesg->hrm_ant_id), 0);
    }

    if (mesg->enabled != FIT_BOOL_INVALID) {
        hash_set(rh, _mapped, "enabled", fit_bool_to_rb_bool(mesg->enabled), 0);
    }

    if (mesg->log_hrv != FIT_BOOL_INVALID) {
        hash_set(rh, _mapped, "log_hrv", fit_bool_to_rb_bool(mesg->log_hrv), 0);
    }

    if (mesg->hrm_ant_id_trans_type != FIT_UINT8Z_INVALID) {
        hash_set(rh, _mapped, "hrm_ant_id_trans_type", UINT2NUM(mesg->hrm_ant_id_trans_type), 0);
    }

    rb_hash_aset(rh, rb_str_new2("_mapped"), _mapped);
    rb_funcall(cFitHandler, cHrmProfileFun, 1, rh);
}


static void pass_sdm_profile(FIT_SDM_PROFILE_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cSdmProfileFun)) {
        return;
    }

    VALUE rh = rb_hash_new();
    VALUE _mapped = rb_hash_new();

    if (mesg->odometer != FIT_UINT32_INVALID) {
        hash_set(rh, _mapped, "odometer", rb_float_new(mesg->odometer / 100.0), 0);
    }

    if (mesg->message_index != FIT_MESSAGE_INDEX_INVALID) {
        hash_set(rh, _mapped, "message_index", UINT2NUM(mesg->message_index), 0);
    }

    if (mesg->sdm_ant_id != FIT_UINT16Z_INVALID) {
        hash_set(rh, _mapped, "sdm_ant_id", UINT2NUM(mesg->sdm_ant_id), 0);
    }

    if (mesg->sdm_cal_factor != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "sdm_cal_factor", rb_float_new(mesg->sdm_cal_factor / 10.0), 0);
    }

    if (mesg->enabled != FIT_BOOL_INVALID) {
        hash_set(rh, _mapped, "enabled", fit_bool_to_rb_bool(mesg->enabled), 0);
    }

    if (mesg->speed_source != FIT_BOOL_INVALID) {
        hash_set(rh, _mapped, "speed_source", fit_bool_to_rb_bool(mesg->speed_source), 0);
    }

    if (mesg->sdm_ant_id_trans_type != FIT_UINT8Z_INVALID) {
        hash_set(rh, _mapped, "sdm_ant_id_trans_type", UINT2NUM(mesg->sdm_ant_id_trans_type), 0);
    }

    if (mesg->odometer_rollover != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "odometer_rollover", UINT2NUM(mesg->odometer_rollover), 0);
    }

    rb_hash_aset(rh, rb_str_new2("_mapped"), _mapped);
    rb_funcall(cFitHandler, cSdmProfileFun, 1, rh);
}


static void pass_bike_profile(FIT_BIKE_PROFILE_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cBikeProfileFun)) {
        return;
    }

    VALUE rh = rb_hash_new();
    VALUE _mapped = rb_hash_new();

    if (mesg->name[0] != FIT_STRING_INVALID) {
        hash_set(rh, _mapped, "name", rb_str_new2(mesg->name), 0);
    }

    if (mesg->odometer != FIT_UINT32_INVALID) {
        hash_set(rh, _mapped, "odometer", rb_float_new(mesg->odometer / 100.0), 0);
    }

    if (mesg->message_index != FIT_MESSAGE_INDEX_INVALID) {
        hash_set(rh, _mapped, "message_index", UINT2NUM(mesg->message_index), 0);
    }

    if (mesg->bike_spd_ant_id != FIT_UINT16Z_INVALID) {
        hash_set(rh, _mapped, "bike_spd_ant_id", UINT2NUM(mesg->bike_spd_ant_id), 0);
    }

    if (mesg->bike_cad_ant_id != FIT_UINT16Z_INVALID) {
        hash_set(rh, _mapped, "bike_cad_ant_id", UINT2NUM(mesg->bike_cad_ant_id), 0);
    }

    if (mesg->bike_spdcad_ant_id != FIT_UINT16Z_INVALID) {
        hash_set(rh, _mapped, "bike_spdcad_ant_id", UINT2NUM(mesg->bike_spdcad_ant_id), 0);
    }

    if (mesg->bike_power_ant_id != FIT_UINT16Z_INVALID) {
        hash_set(rh, _mapped, "bike_power_ant_id", UINT2NUM(mesg->bike_power_ant_id), 0);
    }

    if (mesg->custom_wheelsize != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "custom_wheelsize", rb_float_new(mesg->custom_wheelsize / 1000.0), 0);
    }

    if (mesg->auto_wheelsize != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "auto_wheelsize", rb_float_new(mesg->auto_wheelsize / 1000.0), 0);
    }

    if (mesg->bike_weight != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "bike_weight", rb_float_new(mesg->bike_weight / 10.0), 0);
    }

    if (mesg->power_cal_factor != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "power_cal_factor", rb_float_new(mesg->power_cal_factor / 10.0), 0);
    }

    if (mesg->sport != FIT_SPORT_INVALID) {
        hash_set(rh, _mapped, "sport", UINT2NUM(mesg->sport), MAPID(map_sport, mesg->sport));
    }

    if (mesg->sub_sport != FIT_SUB_SPORT_INVALID) {
        hash_set(rh, _mapped, "sub_sport", UINT2NUM(mesg->sub_sport), MAPID(map_sub_sport, mesg->sub_sport));
    }

    if (mesg->auto_wheel_cal != FIT_BOOL_INVALID) {
        hash_set(rh, _mapped, "auto_wheel_cal", fit_bool_to_rb_bool(mesg->auto_wheel_cal), 0);
    }

    if (mesg->auto_power_zero != FIT_BOOL_INVALID) {
        hash_set(rh, _mapped, "auto_power_zero", fit_bool_to_rb_bool(mesg->auto_power_zero), 0);
    }

    if (mesg->id != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "id", UINT2NUM(mesg->id), 0);
    }

    if (mesg->spd_enabled != FIT_BOOL_INVALID) {
        hash_set(rh, _mapped, "spd_enabled", fit_bool_to_rb_bool(mesg->spd_enabled), 0);
    }

    if (mesg->cad_enabled != FIT_BOOL_INVALID) {
        hash_set(rh, _mapped, "cad_enabled", fit_bool_to_rb_bool(mesg->cad_enabled), 0);
    }

    if (mesg->spdcad_enabled != FIT_BOOL_INVALID) {
        hash_set(rh, _mapped, "spdcad_enabled", fit_bool_to_rb_bool(mesg->spdcad_enabled), 0);
    }

    if (mesg->power_enabled != FIT_BOOL_INVALID) {
        hash_set(rh, _mapped, "power_enabled", fit_bool_to_rb_bool(mesg->power_enabled), 0);
    }

    if (mesg->crank_length != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "crank_length", rb_float_new((mesg->crank_length + 110.0) / 2.0), 0);
    }

    if (mesg->enabled != FIT_BOOL_INVALID) {
        hash_set(rh, _mapped, "enabled", fit_bool_to_rb_bool(mesg->enabled), 0);
    }

    if (mesg->bike_spd_ant_id_trans_type != FIT_UINT8Z_INVALID) {
        hash_set(rh, _mapped, "bike_spd_ant_id_trans_type", UINT2NUM(mesg->bike_spd_ant_id_trans_type), 0);
    }

    if (mesg->bike_cad_ant_id_trans_type != FIT_UINT8Z_INVALID) {
        hash_set(rh, _mapped, "bike_cad_ant_id_trans_type", UINT2NUM(mesg->bike_cad_ant_id_trans_type), 0);
    }

    if (mesg->bike_spdcad_ant_id_trans_type != FIT_UINT8Z_INVALID) {
        hash_set(rh, _mapped, "bike_spdcad_ant_id_trans_type", UINT2NUM(mesg->bike_spdcad_ant_id_trans_type), 0);
    }

    if (mesg->bike_power_ant_id_trans_type != FIT_UINT8Z_INVALID) {
        hash_set(rh, _mapped, "bike_power_ant_id_trans_type", UINT2NUM(mesg->bike_power_ant_id_trans_type), 0);
    }

    if (mesg->odometer_rollover != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "odometer_rollover", UINT2NUM(mesg->odometer_rollover), 0);
    }

    if (mesg->front_gear_num != FIT_UINT8Z_INVALID) {
        hash_set(rh, _mapped, "front_gear", fit_uint8z_array_to_rb_int_array(mesg->front_gear, (long)mesg->front_gear_num), 0);
    }

    if (mesg->rear_gear_num != FIT_UINT8Z_INVALID) {
        hash_set(rh, _mapped, "rear_gear", fit_uint8z_array_to_rb_int_array(mesg->rear_gear, (long)mesg->rear_gear_num), 0);
    }

    rb_hash_aset(rh, rb_str_new2("_mapped"), _mapped);
    rb_funcall(cFitHandler, cBikeProfileFun, 1, rh);
}


static void pass_zones_target(FIT_ZONES_TARGET_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cZonesTargetFun)) {
        return;
    }

    VALUE rh = rb_hash_new();
    VALUE _mapped = rb_hash_new();

    if (mesg->functional_threshold_power != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "functional_threshold_power", UINT2NUM(mesg->functional_threshold_power), 0);
    }

    if (mesg->max_heart_rate != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "max_heart_rate", UINT2NUM(mesg->max_heart_rate), 0);
    }

    if (mesg->threshold_heart_rate != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "threshold_heart_rate", UINT2NUM(mesg->threshold_heart_rate), 0);
    }

    if (mesg->hr_calc_type != FIT_HR_ZONE_CALC_INVALID) {
        hash_set(rh, _mapped, "hr_calc_type", UINT2NUM(mesg->hr_calc_type), MAPID(map_hr_zone_calc, mesg->hr_calc_type));
    }

    if (mesg->pwr_calc_type != FIT_PWR_ZONE_CALC_INVALID) {
        hash_set(rh, _mapped, "pwr_calc_type", UINT2NUM(mesg->pwr_calc_type), MAPID(map_pwr_zone_calc, mesg->pwr_calc_type));
    }

    rb_hash_aset(rh, rb_str_new2("_mapped"), _mapped);
    rb_funcall(cFitHandler, cZonesTargetFun, 1, rh);
}


static void pass_hr_zone(FIT_HR_ZONE_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cHrZoneFun)) {
        return;
    }

    VALUE rh = rb_hash_new();
    VALUE _mapped = rb_hash_new();

    if (mesg->name[0] != FIT_STRING_INVALID) {
        hash_set(rh, _mapped, "name", rb_str_new2(mesg->name), 0);
    }

    if (mesg->message_index != FIT_MESSAGE_INDEX_INVALID) {
        hash_set(rh, _mapped, "message_index", UINT2NUM(mesg->message_index), 0);
    }

    if (mesg->high_bpm != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "high_bpm", UINT2NUM(mesg->high_bpm), 0);
    }

    rb_hash_aset(rh, rb_str_new2("_mapped"), _mapped);
    rb_funcall(cFitHandler, cHrZoneFun, 1, rh);
}


static void pass_power_zone(FIT_POWER_ZONE_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cPowerZoneFun)) {
        return;
    }

    VALUE rh = rb_hash_new();
    VALUE _mapped = rb_hash_new();

    if (mesg->name[0] != FIT_STRING_INVALID) {
        hash_set(rh, _mapped, "name", rb_str_new2(mesg->name), 0);
    }

    if (mesg->message_index != FIT_MESSAGE_INDEX_INVALID) {
        hash_set(rh, _mapped, "message_index", UINT2NUM(mesg->message_index), 0);
    }

    if (mesg->high_value != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "high_value", UINT2NUM(mesg->high_value), 0);
    }

    rb_hash_aset(rh, rb_str_new2("_mapped"), _mapped);
    rb_funcall(cFitHandler, cPowerZoneFun, 1, rh);
}


static void pass_met_zone(FIT_MET_ZONE_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cMetZoneFun)) {
        return;
    }

    VALUE rh = rb_hash_new();
    VALUE _mapped = rb_hash_new();

    if (mesg->message_index != FIT_MESSAGE_INDEX_INVALID) {
        hash_set(rh, _mapped, "message_index", UINT2NUM(mesg->message_index), 0);
    }

    if (mesg->calories != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "calories", rb_float_new(mesg->calories / 10.0), 0);
    }

    if (mesg->high_bpm != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "high_bpm", UINT2NUM(mesg->high_bpm), 0);
    }

    if (mesg->fat_calories != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "fat_calories", rb_float_new(mesg->fat_calories / 10.0), 0);
    }

    rb_hash_aset(rh, rb_str_new2("_mapped"), _mapped);
    rb_funcall(cFitHandler, cMetZoneFun, 1, rh);
}


static void pass_sport(FIT_SPORT_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cSportFun)) {
        return;
    }

    VALUE rh = rb_hash_new();
    VALUE _mapped = rb_hash_new();

    if (mesg->name[0] != FIT_STRING_INVALID) {
        hash_set(rh, _mapped, "name", rb_str_new2(mesg->name), 0);
    }

    if (mesg->sport != FIT_SPORT_INVALID) {
        hash_set(rh, _mapped, "sport", UINT2NUM(mesg->sport), MAPID(map_sport, mesg->sport));
    }

    if (mesg->sub_sport != FIT_SUB_SPORT_INVALID) {
        hash_set(rh, _mapped, "sub_sport", UINT2NUM(mesg->sub_sport), MAPID(map_sub_sport, mesg->sub_sport));
    }

    rb_hash_aset(rh, rb_str_new2("_mapped"), _mapped);
    rb_funcall(cFitHandler, cSportFun, 1, rh);
}


static void pass_goal(FIT_GOAL_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cGoalFun)) {
        return;
    }

    VALUE rh = rb_hash_new();
    VALUE _mapped = rb_hash_new();

    if (mesg->start_date != FIT_DATE_TIME_INVALID) {
        hash_set(rh, _mapped, "start_date",
            fit_time_to_rb(mesg->start_date),
            fit_time_to_rb_str(mesg->start_date));
    }

    if (mesg->end_date != FIT_DATE_TIME_INVALID) {
        hash_set(rh, _mapped, "end_date",
            fit_time_to_rb(mesg->end_date),
            fit_time_to_rb_str(mesg->end_date));
    }

    if (mesg->value != FIT_UINT32_INVALID) {
        hash_set(rh, _mapped, "value", UINT2NUM(mesg->value), 0);
    }

    if (mesg->target_value != FIT_UINT32_INVALID) {
        hash_set(rh, _mapped, "target_value", UINT2NUM(mesg->target_value), 0);
    }

    if (mesg->message_index != FIT_MESSAGE_INDEX_INVALID) {
        hash_set(rh, _mapped, "message_index", UINT2NUM(mesg->message_index), 0);
    }

    if (mesg->recurrence_value != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "recurrence_value", UINT2NUM(mesg->recurrence_value), 0);
    }

    if (mesg->sport != FIT_SPORT_INVALID) {
        hash_set(rh, _mapped, "sport", UINT2NUM(mesg->sport), MAPID(map_sport, mesg->sport));
    }

    if (mesg->sub_sport != FIT_SUB_SPORT_INVALID) {
        hash_set(rh, _mapped, "sub_sport", UINT2NUM(mesg->sub_sport), MAPID(map_sub_sport, mesg->sub_sport));
    }

    if (mesg->type != FIT_GOAL_INVALID) {
        hash_set(rh, _mapped, "type", UINT2NUM(mesg->type), MAPID(map_goal, mesg->type));
    }

    if (mesg->repeat != FIT_BOOL_INVALID) {
        hash_set(rh, _mapped, "repeat", fit_bool_to_rb_bool(mesg->repeat), 0);
    }

    if (mesg->recurrence != FIT_GOAL_RECURRENCE_INVALID) {
        hash_set(rh, _mapped, "recurrence", UINT2NUM(mesg->recurrence), MAPID(map_goal_recurrence, mesg->recurrence));
    }

    if (mesg->enabled != FIT_BOOL_INVALID) {
        hash_set(rh, _mapped, "enabled", fit_bool_to_rb_bool(mesg->enabled), 0);
    }

    rb_hash_aset(rh, rb_str_new2("_mapped"), _mapped);
    rb_funcall(cFitHandler, cGoalFun, 1, rh);
}


static void pass_session(FIT_SESSION_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cSessionFun)) {
        return;
    }

    VALUE rh = rb_hash_new();
    VALUE _mapped = rb_hash_new();

    if (mesg->timestamp != FIT_DATE_TIME_INVALID) {
        hash_set(rh, _mapped, "timestamp",
            fit_time_to_rb(mesg->timestamp),
            fit_time_to_rb_str(mesg->timestamp));
    }

    if (mesg->start_time != FIT_DATE_TIME_INVALID) {
        hash_set(rh, _mapped, "start_time",
            fit_time_to_rb(mesg->start_time),
            fit_time_to_rb_str(mesg->start_time));
    }

    if (mesg->start_position_lat != FIT_SINT32_INVALID) {
        hash_set(rh, _mapped, "start_position_lat", fit_pos_to_rb(mesg->start_position_lat), 0);
    }

    if (mesg->start_position_long != FIT_SINT32_INVALID) {
        hash_set(rh, _mapped, "start_position_long", fit_pos_to_rb(mesg->start_position_long), 0);
    }

    if (mesg->total_elapsed_time != FIT_UINT32_INVALID) {
        hash_set(rh, _mapped, "total_elapsed_time", rb_float_new(mesg->total_elapsed_time / 1000.0), 0);
    }

    if (mesg->total_timer_time != FIT_UINT32_INVALID) {
        hash_set(rh, _mapped, "total_timer_time", rb_float_new(mesg->total_timer_time / 1000.0), 0);
    }

    if (mesg->total_distance != FIT_UINT32_INVALID) {
        hash_set(rh, _mapped, "total_distance", rb_float_new(mesg->total_distance / 100.0), 0);
    }

    if (mesg->total_cycles != FIT_UINT32_INVALID) {
        hash_set(rh, _mapped, "total_cycles", UINT2NUM(mesg->total_cycles), 0);
    }

    if (mesg->nec_lat != FIT_SINT32_INVALID) {
        hash_set(rh, _mapped, "nec_lat", fit_pos_to_rb(mesg->nec_lat), 0);
    }

    if (mesg->nec_long != FIT_SINT32_INVALID) {
        hash_set(rh, _mapped, "nec_long", fit_pos_to_rb(mesg->nec_long), 0);
    }

    if (mesg->swc_lat != FIT_SINT32_INVALID) {
        hash_set(rh, _mapped, "swc_lat", fit_pos_to_rb(mesg->swc_lat), 0);
    }

    if (mesg->swc_long != FIT_SINT32_INVALID) {
        hash_set(rh, _mapped, "swc_long", fit_pos_to_rb(mesg->swc_long), 0);
    }

    if (mesg->avg_stroke_count != FIT_UINT32_INVALID) {
        hash_set(rh, _mapped, "avg_stroke_count", rb_float_new(mesg->avg_stroke_count / 10.0), 0);
    }

    if (mesg->total_work != FIT_UINT32_INVALID) {
        hash_set(rh, _mapped, "total_work", UINT2NUM(mesg->total_work), 0);
    }

    if (mesg->total_moving_time != FIT_UINT32_INVALID) {
        hash_set(rh, _mapped, "total_moving_time", rb_float_new(mesg->total_moving_time / 1000.0), 0);
    }

    if (is_valid_uint32_array(mesg->time_in_hr_zone, DIM(mesg->time_in_hr_zone))) {
        hash_set(rh, _mapped, "time_in_hr_zone", fit_uint32_array_to_rb_float_array(mesg->time_in_hr_zone, DIM(mesg->time_in_hr_zone), 1000.0), 0);
    }

    if (is_valid_uint32_array(mesg->time_in_speed_zone, DIM(mesg->time_in_speed_zone))) {
        hash_set(rh, _mapped, "time_in_speed_zone", fit_uint32_array_to_rb_float_array(mesg->time_in_speed_zone, DIM(mesg->time_in_speed_zone), 1000.0), 0);
    }

    if (is_valid_uint32_array(mesg->time_in_cadence_zone, DIM(mesg->time_in_cadence_zone))) {
        hash_set(rh, _mapped, "time_in_cadence_zone", fit_uint32_array_to_rb_float_array(mesg->time_in_cadence_zone, DIM(mesg->time_in_cadence_zone), 1000.0), 0);
    }

    if (is_valid_uint32_array(mesg->time_in_power_zone, DIM(mesg->time_in_power_zone))) {
        hash_set(rh, _mapped, "time_in_power_zone", fit_uint32_array_to_rb_float_array(mesg->time_in_power_zone, DIM(mesg->time_in_power_zone), 1000.0), 0);
    }

    if (mesg->avg_lap_time != FIT_UINT32_INVALID) {
        hash_set(rh, _mapped, "avg_lap_time", rb_float_new(mesg->avg_lap_time / 1000.0), 0);
    }

    if (mesg->message_index != FIT_MESSAGE_INDEX_INVALID) {
        hash_set(rh, _mapped, "message_index", UINT2NUM(mesg->message_index), 0);
    }

    if (mesg->total_calories != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "total_calories", UINT2NUM(mesg->total_calories), 0);
    }

    if (mesg->total_fat_calories != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "total_fat_calories", UINT2NUM(mesg->total_fat_calories), 0);
    }

    if (mesg->avg_speed != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "avg_speed", rb_float_new(mesg->avg_speed / 1000.0), 0);
    }

    if (mesg->max_speed != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "max_speed", rb_float_new(mesg->max_speed / 1000.0), 0);
    }

    if (mesg->avg_power != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "avg_power", UINT2NUM(mesg->avg_power), 0);
    }

    if (mesg->max_power != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "max_power", UINT2NUM(mesg->max_power), 0);
    }

    if (mesg->total_ascent != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "total_ascent", UINT2NUM(mesg->total_ascent), 0);
    }

    if (mesg->total_descent != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "total_descent", UINT2NUM(mesg->total_descent), 0);
    }

    if (mesg->first_lap_index != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "first_lap_index", UINT2NUM(mesg->first_lap_index), 0);
    }

    if (mesg->num_laps != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "num_laps", UINT2NUM(mesg->num_laps), 0);
    }

    if (mesg->normalized_power != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "normalized_power", UINT2NUM(mesg->normalized_power), 0);
    }

    if (mesg->training_stress_score != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "training_stress_score", rb_float_new(mesg->training_stress_score / 10.0), 0);
    }

    if (mesg->intensity_factor != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "intensity_factor", rb_float_new(mesg->intensity_factor / 1000.0), 0);
    }

    if (mesg->left_right_balance != FIT_LEFT_RIGHT_BALANCE_100_INVALID) {
        FIT_UINT16 tmp = mesg->left_right_balance;
        hash_set(rh, _mapped, "left_right_balance", rb_float_new((tmp & FIT_LEFT_RIGHT_BALANCE_100_MASK) / 100.0), 0);
        hash_set(rh, _mapped, "left_right_balance_flag", INT2BOOL((tmp & FIT_LEFT_RIGHT_BALANCE_100_RIGHT) != 0), 0);
    }

    if (mesg->avg_stroke_distance != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "avg_stroke_distance", rb_float_new(mesg->avg_stroke_distance / 100.0), 0);
    }

    if (mesg->pool_length != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "pool_length", rb_float_new(mesg->pool_length / 100.0), 0);
    }

    if (mesg->num_active_lengths != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "num_active_lengths", UINT2NUM(mesg->num_active_lengths), 0);
    }

    if (mesg->avg_altitude != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "avg_altitude", rb_float_new((mesg->avg_altitude - 500.0) / 5.0), 0);
    }

    if (mesg->max_altitude != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "max_altitude", rb_float_new((mesg->max_altitude - 500.0) / 5.0), 0);
    }

    if (mesg->avg_grade != FIT_SINT16_INVALID) {
        hash_set(rh, _mapped, "avg_grade", rb_float_new(mesg->avg_grade / 100.0), 0);
    }

    if (mesg->avg_pos_grade != FIT_SINT16_INVALID) {
        hash_set(rh, _mapped, "avg_pos_grade", rb_float_new(mesg->avg_pos_grade / 100.0), 0);
    }

    if (mesg->avg_neg_grade != FIT_SINT16_INVALID) {
        hash_set(rh, _mapped, "avg_neg_grade", rb_float_new(mesg->avg_neg_grade / 100.0), 0);
    }

    if (mesg->max_pos_grade != FIT_SINT16_INVALID) {
        hash_set(rh, _mapped, "max_pos_grade", rb_float_new(mesg->max_pos_grade / 100.0), 0);
    }

    if (mesg->max_neg_grade != FIT_SINT16_INVALID) {
        hash_set(rh, _mapped, "max_neg_grade", rb_float_new(mesg->max_neg_grade / 100.0), 0);
    }

    if (mesg->avg_pos_vertical_speed != FIT_SINT16_INVALID) {
        hash_set(rh, _mapped, "avg_pos_vertical_speed", rb_float_new(mesg->avg_pos_vertical_speed / 1000.0), 0);
    }

    if (mesg->avg_neg_vertical_speed != FIT_SINT16_INVALID) {
        hash_set(rh, _mapped, "avg_neg_vertical_speed", rb_float_new(mesg->avg_neg_vertical_speed / 1000.0), 0);
    }

    if (mesg->max_pos_vertical_speed != FIT_SINT16_INVALID) {
        hash_set(rh, _mapped, "max_pos_vertical_speed", rb_float_new(mesg->max_pos_vertical_speed / 1000.0), 0);
    }

    if (mesg->max_neg_vertical_speed != FIT_SINT16_INVALID) {
        hash_set(rh, _mapped, "max_neg_vertical_speed", rb_float_new(mesg->max_neg_vertical_speed / 1000.0), 0);
    }

    if (mesg->best_lap_index != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "best_lap_index", UINT2NUM(mesg->best_lap_index), 0);
    }

    if (mesg->min_altitude != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "min_altitude", rb_float_new((mesg->min_altitude - 500.0) / 5.0), 0);
    }

    if (mesg->player_score != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "player_score", UINT2NUM(mesg->player_score), 0);
    }

    if (mesg->opponent_score != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "opponent_score", UINT2NUM(mesg->opponent_score), 0);
    }

    if (is_valid_uint16_array(mesg->stroke_count, DIM(mesg->stroke_count))) {
        hash_set(rh, _mapped, "stroke_count", fit_uint16_array_to_rb_int_array(mesg->stroke_count, DIM(mesg->stroke_count)), 0);
    }

    if (is_valid_uint16_array(mesg->zone_count, DIM(mesg->zone_count))) {
        hash_set(rh, _mapped, "zone_count", fit_uint16_array_to_rb_int_array(mesg->zone_count, DIM(mesg->zone_count)), 0);
    }

    if (mesg->max_ball_speed != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "max_ball_speed", rb_float_new(mesg->max_ball_speed / 100.0), 0);
    }

    if (mesg->avg_ball_speed != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "avg_ball_speed", rb_float_new(mesg->avg_ball_speed / 100.0), 0);
    }

    if (mesg->avg_vertical_oscillation != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "avg_vertical_oscillation", rb_float_new(mesg->avg_vertical_oscillation / 10.0), 0);
    }

    if (mesg->avg_stance_time_percent != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "avg_stance_time_percent", rb_float_new(mesg->avg_stance_time_percent / 100.0), 0);
    }

    if (mesg->avg_stance_time != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "avg_stance_time", rb_float_new(mesg->avg_stance_time / 10.0), 0);
    }

    if (mesg->event != FIT_EVENT_INVALID) {
        hash_set(rh, _mapped, "event", UINT2NUM(mesg->event), MAPID(map_event, mesg->event));
    }

    if (mesg->event_type != FIT_EVENT_TYPE_INVALID) {
        hash_set(rh, _mapped, "event_type", UINT2NUM(mesg->event_type), MAPID(map_event_type, mesg->event_type));
    }

    if (mesg->sport != FIT_SPORT_INVALID) {
        hash_set(rh, _mapped, "sport", UINT2NUM(mesg->sport), MAPID(map_sport, mesg->sport));
    }

    if (mesg->sub_sport != FIT_SUB_SPORT_INVALID) {
        hash_set(rh, _mapped, "sub_sport", UINT2NUM(mesg->sub_sport), MAPID(map_sub_sport, mesg->sub_sport));
    }

    if (mesg->avg_heart_rate != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "avg_heart_rate", UINT2NUM(mesg->avg_heart_rate), 0);
    }

    if (mesg->max_heart_rate != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "max_heart_rate", UINT2NUM(mesg->max_heart_rate), 0);
    }

    if (mesg->avg_cadence != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "avg_cadence", UINT2NUM(mesg->avg_cadence), 0);
    }

    if (mesg->max_cadence != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "max_cadence", UINT2NUM(mesg->max_cadence), 0);
    }

    if (mesg->total_training_effect != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "total_training_effect", UINT2NUM(mesg->total_training_effect), 0);
    }

    if (mesg->event_group != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "event_group", UINT2NUM(mesg->event_group), 0);
    }

    if (mesg->trigger != FIT_SESSION_TRIGGER_INVALID) {
        hash_set(rh, _mapped, "trigger", UINT2NUM(mesg->trigger), MAPID(map_session_trigger, mesg->trigger));
    }

    if (mesg->swim_stroke != FIT_SWIM_STROKE_INVALID) {
        hash_set(rh, _mapped, "swim_stroke", UINT2NUM(mesg->swim_stroke), MAPID(map_swim_stroke, mesg->swim_stroke));
    }

    if (mesg->pool_length_unit != FIT_DISPLAY_MEASURE_INVALID) {
        hash_set(rh, _mapped, "pool_length_unit", UINT2NUM(mesg->pool_length_unit), MAPID(map_display_measure, mesg->pool_length_unit));
    }

    if (mesg->gps_accuracy != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "gps_accuracy", UINT2NUM(mesg->gps_accuracy), 0);
    }

    if (mesg->avg_temperature != FIT_SINT8_INVALID) {
        hash_set(rh, _mapped, "avg_temperature", INT2NUM(mesg->avg_temperature), 0);
    }

    if (mesg->max_temperature != FIT_SINT8_INVALID) {
        hash_set(rh, _mapped, "max_temperature", INT2NUM(mesg->max_temperature), 0);
    }

    if (mesg->min_heart_rate != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "min_heart_rate", UINT2NUM(mesg->min_heart_rate), 0);
    }

    if (mesg->opponent_name[0] != FIT_STRING_INVALID) {
        hash_set(rh, _mapped, "opponent_name", rb_str_new2(mesg->opponent_name), 0);
    }

    if (mesg->avg_fractional_cadence != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "avg_fractional_cadence", rb_float_new(mesg->avg_fractional_cadence / 128.0), 0);
    }

    if (mesg->max_fractional_cadence != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "max_fractional_cadence", rb_float_new(mesg->max_fractional_cadence / 128.0), 0);
    }

    if (mesg->total_fractional_cycles != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "total_fractional_cycles", rb_float_new(mesg->total_fractional_cycles / 128.0), 0);
    }

    rb_hash_aset(rh, rb_str_new2("_mapped"), _mapped);
    rb_funcall(cFitHandler, cSessionFun, 1, rh);
}


static void pass_lap(FIT_LAP_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cLapFun)) {
        return;
    }

    VALUE rh = rb_hash_new();
    VALUE _mapped = rb_hash_new();

    if (mesg->timestamp != FIT_DATE_TIME_INVALID) {
        hash_set(rh, _mapped, "timestamp",
            fit_time_to_rb(mesg->timestamp),
            fit_time_to_rb_str(mesg->timestamp));
    }

    if (mesg->start_time != FIT_DATE_TIME_INVALID) {
        hash_set(rh, _mapped, "start_time",
            fit_time_to_rb(mesg->start_time),
            fit_time_to_rb_str(mesg->start_time));
    }

    if (mesg->start_position_lat != FIT_SINT32_INVALID) {
        hash_set(rh, _mapped, "start_position_lat", fit_pos_to_rb(mesg->start_position_lat), 0);
    }

    if (mesg->start_position_long != FIT_SINT32_INVALID) {
        hash_set(rh, _mapped, "start_position_long", fit_pos_to_rb(mesg->start_position_long), 0);
    }

    if (mesg->end_position_lat != FIT_SINT32_INVALID) {
        hash_set(rh, _mapped, "end_position_lat", fit_pos_to_rb(mesg->end_position_lat), 0);
    }

    if (mesg->end_position_long != FIT_SINT32_INVALID) {
        hash_set(rh, _mapped, "end_position_long", fit_pos_to_rb(mesg->end_position_long), 0);
    }

    if (mesg->total_elapsed_time != FIT_UINT32_INVALID) {
        hash_set(rh, _mapped, "total_elapsed_time", rb_float_new(mesg->total_elapsed_time / 1000.0), 0);
    }

    if (mesg->total_timer_time != FIT_UINT32_INVALID) {
        hash_set(rh, _mapped, "total_timer_time", rb_float_new(mesg->total_timer_time / 1000.0), 0);
    }

    if (mesg->total_distance != FIT_UINT32_INVALID) {
        hash_set(rh, _mapped, "total_distance", rb_float_new(mesg->total_distance / 100.0), 0);
    }

    if (mesg->total_cycles != FIT_UINT32_INVALID) {
        hash_set(rh, _mapped, "total_cycles", UINT2NUM(mesg->total_cycles), 0);
    }

    if (mesg->total_work != FIT_UINT32_INVALID) {
        hash_set(rh, _mapped, "total_work", UINT2NUM(mesg->total_work), 0);
    }

    if (mesg->total_moving_time != FIT_UINT32_INVALID) {
        hash_set(rh, _mapped, "total_moving_time", rb_float_new(mesg->total_moving_time / 1000.0), 0);
    }

    if (is_valid_uint32_array(mesg->time_in_hr_zone, DIM(mesg->time_in_hr_zone))) {
        hash_set(rh, _mapped, "time_in_hr_zone", fit_uint32_array_to_rb_float_array(mesg->time_in_hr_zone, DIM(mesg->time_in_hr_zone), 1000.0), 0);
    }

    if (is_valid_uint32_array(mesg->time_in_speed_zone, DIM(mesg->time_in_speed_zone))) {
        hash_set(rh, _mapped, "time_in_speed_zone", fit_uint32_array_to_rb_float_array(mesg->time_in_speed_zone, DIM(mesg->time_in_speed_zone), 1000.0), 0);
    }

    if (is_valid_uint32_array(mesg->time_in_cadence_zone, DIM(mesg->time_in_cadence_zone))) {
        hash_set(rh, _mapped, "time_in_cadence_zone", fit_uint32_array_to_rb_float_array(mesg->time_in_cadence_zone, DIM(mesg->time_in_cadence_zone), 1000.0), 0);
    }

    if (is_valid_uint32_array(mesg->time_in_power_zone, DIM(mesg->time_in_power_zone))) {
        hash_set(rh, _mapped, "time_in_power_zone", fit_uint32_array_to_rb_float_array(mesg->time_in_power_zone, DIM(mesg->time_in_power_zone), 1000.0), 0);
    }

    if (mesg->message_index != FIT_MESSAGE_INDEX_INVALID) {
        hash_set(rh, _mapped, "message_index", UINT2NUM(mesg->message_index), 0);
    }
    
    if (mesg->total_calories != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "total_calories", UINT2NUM(mesg->total_calories), 0);
    }
    
    if (mesg->total_fat_calories != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "total_fat_calories", UINT2NUM(mesg->total_fat_calories), 0);
    }
    
    if (mesg->avg_speed != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "avg_speed", rb_float_new(mesg->avg_speed / 1000.0), 0);
    }
    
    if (mesg->max_speed != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "max_speed", rb_float_new(mesg->max_speed / 1000.0), 0);
    }
    
    if (mesg->avg_power != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "avg_power", UINT2NUM(mesg->avg_power), 0);
    }
    
    if (mesg->max_power != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "max_power", UINT2NUM(mesg->max_power), 0);
    }
    
    if (mesg->total_ascent != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "total_ascent", UINT2NUM(mesg->total_ascent), 0);
    }
    
    if (mesg->total_descent != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "total_descent", UINT2NUM(mesg->total_descent), 0);
    }
    
    if (mesg->num_lengths != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "num_lengths", UINT2NUM(mesg->num_lengths), 0);
    }
    
    if (mesg->normalized_power != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "normalized_power", UINT2NUM(mesg->normalized_power), 0);
    }
    
    if (mesg->avg_stroke_distance != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "avg_stroke_distance", rb_float_new(mesg->avg_stroke_distance / 100.0), 0);
    }
    
    if (mesg->num_active_lengths != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "num_active_lengths", UINT2NUM(mesg->num_active_lengths), 0);
    }
    
    if (mesg->avg_altitude != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "avg_altitude", rb_float_new((mesg->avg_altitude - 500.0) / 5.0), 0);
    }
    
    if (mesg->max_altitude != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "max_altitude", rb_float_new((mesg->max_altitude - 500.0) / 5.0), 0);
    }
    
    if (mesg->avg_grade != FIT_SINT16_INVALID) {
        hash_set(rh, _mapped, "avg_grade", rb_float_new(mesg->avg_grade / 100.0), 0);
    }
    
    if (mesg->avg_pos_grade != FIT_SINT16_INVALID) {
        hash_set(rh, _mapped, "avg_pos_grade", rb_float_new(mesg->avg_pos_grade / 100.0), 0);
    }
    
    if (mesg->avg_neg_grade != FIT_SINT16_INVALID) {
        hash_set(rh, _mapped, "avg_neg_grade", rb_float_new(mesg->avg_neg_grade / 100.0), 0);
    }
    
    if (mesg->max_pos_grade != FIT_SINT16_INVALID) {
        hash_set(rh, _mapped, "max_pos_grade", rb_float_new(mesg->max_pos_grade / 100.0), 0);
    }
    
    if (mesg->max_neg_grade != FIT_SINT16_INVALID) {
        hash_set(rh, _mapped, "max_neg_grade", rb_float_new(mesg->max_neg_grade / 100.0), 0);
    }
    
    if (mesg->avg_pos_vertical_speed != FIT_SINT16_INVALID) {
        hash_set(rh, _mapped, "avg_pos_vertical_speed", rb_float_new(mesg->avg_pos_vertical_speed / 1000.0), 0);
    }
    
    if (mesg->avg_neg_vertical_speed != FIT_SINT16_INVALID) {
        hash_set(rh, _mapped, "avg_neg_vertical_speed", rb_float_new(mesg->avg_neg_vertical_speed / 1000.0), 0);
    }
    
    if (mesg->max_pos_vertical_speed != FIT_SINT16_INVALID) {
        hash_set(rh, _mapped, "max_pos_vertical_speed", rb_float_new(mesg->max_pos_vertical_speed / 1000.0), 0);
    }
    
    if (mesg->max_neg_vertical_speed != FIT_SINT16_INVALID) {
        hash_set(rh, _mapped, "max_neg_vertical_speed", rb_float_new(mesg->max_neg_vertical_speed / 1000.0), 0);
    }
    
    if (mesg->repetition_num != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "repetition_num", UINT2NUM(mesg->repetition_num), 0);
    }
    
    if (mesg->min_altitude != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "min_altitude", rb_float_new((mesg->min_altitude - 500.0) / 5.0), 0);
    }
    
    if (mesg->wkt_step_index != FIT_MESSAGE_INDEX_INVALID) {
        hash_set(rh, _mapped, "wkt_step_index", UINT2NUM(mesg->wkt_step_index), 0);
    }
    
    if (mesg->opponent_score != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "opponent_score", UINT2NUM(mesg->opponent_score), 0);
    }
    
    if (is_valid_uint16_array(mesg->stroke_count, DIM(mesg->stroke_count))) {
        hash_set(rh, _mapped, "stroke_count", fit_uint16_array_to_rb_int_array(mesg->stroke_count, DIM(mesg->stroke_count)), 0);
    }

    if (is_valid_uint16_array(mesg->zone_count, DIM(mesg->zone_count))) {
        hash_set(rh, _mapped, "zone_count", fit_uint16_array_to_rb_int_array(mesg->zone_count, DIM(mesg->zone_count)), 0);
    }

    if (mesg->avg_vertical_oscillation != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "avg_vertical_oscillation", rb_float_new(mesg->avg_vertical_oscillation / 10.0), 0);
    }

    if (mesg->avg_stance_time_percent != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "avg_stance_time_percent", rb_float_new(mesg->avg_stance_time_percent / 100.0), 0);
    }

    if (mesg->avg_stance_time != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "avg_stance_time", rb_float_new(mesg->avg_stance_time / 10.0), 0);
    }

    if (mesg->player_score != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "player_score", UINT2NUM(mesg->player_score), 0);
    }

    if (is_valid_uint16_array(mesg->avg_total_hemoglobin_conc, DIM(mesg->avg_total_hemoglobin_conc))) {
        hash_set(rh, _mapped, "avg_total_hemoglobin_conc", fit_uint16_array_to_rb_float_array(mesg->avg_total_hemoglobin_conc, DIM(mesg->avg_total_hemoglobin_conc), 100.0), 0);
    }

    if (is_valid_uint16_array(mesg->min_total_hemoglobin_conc, DIM(mesg->min_total_hemoglobin_conc))) {
        hash_set(rh, _mapped, "min_total_hemoglobin_conc", fit_uint16_array_to_rb_float_array(mesg->min_total_hemoglobin_conc, DIM(mesg->min_total_hemoglobin_conc), 100.0), 0);
    }

    if (is_valid_uint16_array(mesg->max_total_hemoglobin_conc, DIM(mesg->max_total_hemoglobin_conc))) {
        hash_set(rh, _mapped, "max_total_hemoglobin_conc", fit_uint16_array_to_rb_float_array(mesg->max_total_hemoglobin_conc, DIM(mesg->max_total_hemoglobin_conc), 100.0), 0);
    }

    if (is_valid_uint16_array(mesg->avg_saturated_hemoglobin_percent, DIM(mesg->avg_saturated_hemoglobin_percent))) {
        hash_set(rh, _mapped, "avg_saturated_hemoglobin_percent", fit_uint16_array_to_rb_float_array(mesg->avg_saturated_hemoglobin_percent, DIM(mesg->avg_saturated_hemoglobin_percent), 10.0), 0);
    }

    if (is_valid_uint16_array(mesg->min_saturated_hemoglobin_percent, DIM(mesg->min_saturated_hemoglobin_percent))) {
        hash_set(rh, _mapped, "min_saturated_hemoglobin_percent", fit_uint16_array_to_rb_float_array(mesg->min_saturated_hemoglobin_percent, DIM(mesg->min_saturated_hemoglobin_percent), 10.0), 0);
    }

    if (is_valid_uint16_array(mesg->max_saturated_hemoglobin_percent, DIM(mesg->max_saturated_hemoglobin_percent))) {
        hash_set(rh, _mapped, "max_saturated_hemoglobin_percent", fit_uint16_array_to_rb_float_array(mesg->max_saturated_hemoglobin_percent, DIM(mesg->max_saturated_hemoglobin_percent), 10.0), 0);
    }

    if (mesg->event != FIT_EVENT_INVALID) {
        hash_set(rh, _mapped, "event", UINT2NUM(mesg->event), MAPID(map_event, mesg->event));
    }

    if (mesg->event_type != FIT_EVENT_TYPE_INVALID) {
        hash_set(rh, _mapped, "event_type", UINT2NUM(mesg->event_type), MAPID(map_event_type, mesg->event_type));
    }

    if (mesg->avg_heart_rate != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "avg_heart_rate", UINT2NUM(mesg->avg_heart_rate), 0);
    }

    if (mesg->max_heart_rate != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "max_heart_rate", UINT2NUM(mesg->max_heart_rate), 0);
    }

    if (mesg->avg_cadence != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "avg_cadence", UINT2NUM(mesg->avg_cadence), 0);
    }

    if (mesg->max_cadence != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "max_cadence", UINT2NUM(mesg->max_cadence), 0);
    }

    if (mesg->intensity != FIT_INTENSITY_INVALID) {
        hash_set(rh, _mapped, "intensity", UINT2NUM(mesg->intensity), MAPID(map_intensity, mesg->intensity));
    }

    if (mesg->lap_trigger != FIT_LAP_TRIGGER_INVALID) {
        hash_set(rh, _mapped, "lap_trigger", UINT2NUM(mesg->lap_trigger), MAPID(map_lap_trigger, mesg->lap_trigger));
    }

    if (mesg->sport != FIT_SPORT_INVALID) {
        hash_set(rh, _mapped, "sport", UINT2NUM(mesg->sport), MAPID(map_sport, mesg->sport));
    }

    if (mesg->event_group != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "event_group", UINT2NUM(mesg->event_group), 0);
    }

    if (mesg->swim_stroke != FIT_SWIM_STROKE_INVALID) {
        hash_set(rh, _mapped, "swim_stroke", UINT2NUM(mesg->swim_stroke), MAPID(map_swim_stroke, mesg->swim_stroke));
    }

    if (mesg->sub_sport != FIT_SUB_SPORT_INVALID) {
        hash_set(rh, _mapped, "sub_sport", UINT2NUM(mesg->sub_sport), MAPID(map_sub_sport, mesg->sub_sport));
    }

    if (mesg->gps_accuracy != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "gps_accuracy", UINT2NUM(mesg->gps_accuracy), 0);
    }

    if (mesg->avg_temperature != FIT_SINT8_INVALID) {
        hash_set(rh, _mapped, "avg_temperature", INT2NUM(mesg->avg_temperature), 0);
    }

    if (mesg->max_temperature != FIT_SINT8_INVALID) {
        hash_set(rh, _mapped, "max_temperature", INT2NUM(mesg->max_temperature), 0);
    }

    if (mesg->avg_fractional_cadence != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "avg_fractional_cadence", rb_float_new(mesg->avg_fractional_cadence / 128.0), 0);
    }

    if (mesg->max_fractional_cadence != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "max_fractional_cadence", rb_float_new(mesg->max_fractional_cadence / 128.0), 0);
    }

    if (mesg->total_fractional_cycles != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "total_fractional_cycles", rb_float_new(mesg->total_fractional_cycles / 128.0), 0);
    }

    rb_hash_aset(rh, rb_str_new2("_mapped"), _mapped);
    rb_funcall(cFitHandler, cLapFun, 1, rh);
}


static void pass_record(FIT_RECORD_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cRecordFun)) {
        return;
    }

    VALUE rh = rb_hash_new();
    VALUE _mapped = rb_hash_new();

    if (mesg->timestamp != FIT_DATE_TIME_INVALID) {
        hash_set(rh, _mapped, "timestamp",
            fit_time_to_rb(mesg->timestamp),
            fit_time_to_rb_str(mesg->timestamp));
    }

    if (mesg->position_lat != FIT_SINT32_INVALID) {
        hash_set(rh, _mapped, "position_lat", fit_pos_to_rb(mesg->position_lat), 0);
    }

    if (mesg->position_long != FIT_SINT32_INVALID) {
        hash_set(rh, _mapped, "position_long", fit_pos_to_rb(mesg->position_long), 0);
    }

    if (mesg->distance != FIT_UINT32_INVALID) {
        hash_set(rh, _mapped, "distance", rb_float_new(mesg->distance / 100.0), 0);
    }

    if (mesg->time_from_course != FIT_SINT32_INVALID) {
        hash_set(rh, _mapped, "time_from_course", rb_float_new(mesg->time_from_course / 1000.0), 0);
    }

    if (mesg->total_cycles != FIT_UINT32_INVALID) {
        hash_set(rh, _mapped, "total_cycles", UINT2NUM(mesg->total_cycles), 0);
    }

    if (mesg->accumulated_power != FIT_UINT32_INVALID) {
        hash_set(rh, _mapped, "accumulated_power", UINT2NUM(mesg->accumulated_power), 0);
    }

    if (mesg->altitude != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "altitude", rb_float_new(mesg->altitude / 5.0 - 500), 0);
    }

    if (mesg->speed != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "speed", rb_float_new(mesg->speed / 1000.0), 0);
    }

    if (mesg->power != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "power", UINT2NUM(mesg->power), 0);
    }

    if (mesg->grade != FIT_SINT16_INVALID) {
        hash_set(rh, _mapped, "grade", rb_float_new(mesg->grade / 100.0), 0);
    }

    if (mesg->compressed_accumulated_power != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "compressed_accumulated_power", UINT2NUM(mesg->compressed_accumulated_power), 0);
    }

    if (mesg->vertical_speed != FIT_SINT16_INVALID) {
        hash_set(rh, _mapped, "vertical_speed", rb_float_new(mesg->vertical_speed / 1000.0), 0);
    }

    if (mesg->calories != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "calories", UINT2NUM(mesg->calories), 0);
    }

    if (mesg->vertical_oscillation != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "vertical_oscillation", rb_float_new(mesg->vertical_oscillation / 10.0), 0);
    }

    if (mesg->stance_time_percent != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "stance_time_percent", rb_float_new(mesg->stance_time_percent / 100.0), 0);
    }

    if (mesg->stance_time != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "stance_time", rb_float_new(mesg->stance_time / 10.0), 0);
    }

    if (mesg->ball_speed != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "ball_speed", rb_float_new(mesg->ball_speed / 100.0), 0);
    }

    if (mesg->cadence256 != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "cadence256", rb_float_new(mesg->cadence256 / 256.0), 0);
    }

    if (mesg->total_hemoglobin_conc != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "total_hemoglobin_conc", rb_float_new(mesg->total_hemoglobin_conc / 100.0), 0);
    }

    if (mesg->total_hemoglobin_conc_min != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "total_hemoglobin_conc_min", rb_float_new(mesg->total_hemoglobin_conc_min / 100.0), 0);
    }

    if (mesg->total_hemoglobin_conc_max != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "total_hemoglobin_conc_max", rb_float_new(mesg->total_hemoglobin_conc_max / 100.0), 0);
    }

    if (mesg->saturated_hemoglobin_percent != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "saturated_hemoglobin_percent", rb_float_new(mesg->saturated_hemoglobin_percent / 10.0), 0);
    }

    if (mesg->saturated_hemoglobin_percent_min != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "saturated_hemoglobin_percent_min", rb_float_new(mesg->saturated_hemoglobin_percent_min / 10.0), 0);
    }

    if (mesg->saturated_hemoglobin_percent_max != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "saturated_hemoglobin_percent_max", rb_float_new(mesg->saturated_hemoglobin_percent_max / 10.0), 0);
    }

    if (mesg->heart_rate != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "heart_rate", UINT2NUM(mesg->heart_rate), 0);
    }

    if (mesg->cadence != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "cadence", UINT2NUM(mesg->cadence), 0);
    }

    if (is_valid_uint8_array(mesg->compressed_speed_distance, DIM(mesg->compressed_speed_distance))) {
        hash_set(rh, _mapped, "compressed_speed_distance", fit_uint8_array_to_rb_int_array(mesg->compressed_speed_distance, DIM(mesg->compressed_speed_distance)), 0);
    }

    if (mesg->resistance != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "resistance", UINT2NUM(mesg->resistance), 0);
    }
    
    if (mesg->cycle_length != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "cycle_length", rb_float_new(mesg->cycle_length / 100.0), 0);
    }
    
    if (mesg->temperature != FIT_SINT8_INVALID) {
        hash_set(rh, _mapped, "temperature", INT2NUM(mesg->temperature), 0);
    }
    
    if (is_valid_uint8_array(mesg->speed_1s, DIM(mesg->speed_1s))) {
        hash_set(rh, _mapped, "speed_1s", fit_uint8_array_to_rb_float_array(mesg->speed_1s, DIM(mesg->speed_1s), 16.0), 0);
    }

    if (mesg->cycles != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "cycles", UINT2NUM(mesg->cycles), 0);
    }

    if (mesg->left_right_balance != FIT_LEFT_RIGHT_BALANCE_INVALID) {
        FIT_UINT8 tmp = mesg->left_right_balance;
        hash_set(rh, _mapped, "left_right_balance", rb_float_new(tmp & FIT_LEFT_RIGHT_BALANCE_MASK), 0);
        hash_set(rh, _mapped, "left_right_balance_flag", INT2BOOL((tmp & FIT_LEFT_RIGHT_BALANCE_RIGHT) != 0), 0);
    }

    if (mesg->gps_accuracy != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "gps_accuracy", UINT2NUM(mesg->gps_accuracy), 0);
    }

    if (mesg->activity_type != FIT_ACTIVITY_TYPE_INVALID) {
        hash_set(rh, _mapped, "activity_type", UINT2NUM(mesg->activity_type), MAPID(map_activity_type, mesg->activity_type));
    }

    if (mesg->left_torque_effectiveness != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "left_torque_effectiveness", rb_float_new(mesg->left_torque_effectiveness / 2.0), 0);
    }

    if (mesg->right_torque_effectiveness != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "right_torque_effectiveness", rb_float_new(mesg->right_torque_effectiveness / 2.0), 0);
    }

    if (mesg->left_pedal_smoothness != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "left_pedal_smoothness", rb_float_new(mesg->left_pedal_smoothness / 2.0), 0);
    }

    if (mesg->right_pedal_smoothness != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "right_pedal_smoothness", rb_float_new(mesg->right_pedal_smoothness / 2.0), 0);
    }

    if (mesg->combined_pedal_smoothness != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "combined_pedal_smoothness", rb_float_new(mesg->combined_pedal_smoothness / 2.0), 0);
    }

    if (mesg->time128 != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "time128", rb_float_new(mesg->time128 / 128.0), 0);
    }

    if (mesg->stroke_type != FIT_STROKE_TYPE_INVALID) {
        hash_set(rh, _mapped, "stroke_type", UINT2NUM(mesg->stroke_type), MAPID(map_stroke_type, mesg->stroke_type));
    }

    if (mesg->zone != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "zone", UINT2NUM(mesg->zone), 0);
    }

    if (mesg->device_index != FIT_DEVICE_INDEX_INVALID) {
        hash_set(rh, _mapped, "device_index", UINT2NUM(mesg->device_index), 0);
    }
    
    rb_hash_aset(rh, rb_str_new2("_mapped"), _mapped);
    rb_funcall(cFitHandler, cRecordFun, 1, rh);
}


static void pass_event(FIT_EVENT_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cEventFun)) {
        return;
    }

    VALUE rh = rb_hash_new();
    VALUE _mapped = rb_hash_new();

    if (mesg->timestamp != FIT_DATE_TIME_INVALID) {
        hash_set(rh, _mapped, "timestamp",
            fit_time_to_rb(mesg->timestamp),
            fit_time_to_rb_str(mesg->timestamp));
    }

    if (mesg->data != FIT_UINT32_INVALID) {
        hash_set(rh, _mapped, "data", UINT2NUM(mesg->data), 0);
    }

    if (mesg->data16 != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "data16", UINT2NUM(mesg->data16), 0);
    }

    if (mesg->score != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "score", UINT2NUM(mesg->score), 0);
    }

    if (mesg->opponent_score != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "opponent_score", UINT2NUM(mesg->opponent_score), 0);
    }

    if (mesg->event != FIT_EVENT_INVALID) {
        hash_set(rh, _mapped, "event", UINT2NUM(mesg->event), MAPID(map_event, mesg->event));
    }

    if (mesg->event_type != FIT_EVENT_TYPE_INVALID) {
        hash_set(rh, _mapped, "event_type", UINT2NUM(mesg->event_type), MAPID(map_event_type, mesg->event_type));
    }

    if (mesg->event_group != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "event_group", UINT2NUM(mesg->event_group), 0);
    }

    if (mesg->front_gear_num != FIT_UINT8Z_INVALID) {
        hash_set(rh, _mapped, "front_gear", fit_uint8z_array_to_rb_int_array(mesg->front_gear, (long)mesg->front_gear_num), 0);
    }

    if (mesg->rear_gear_num != FIT_UINT8Z_INVALID) {
        hash_set(rh, _mapped, "rear_gear", fit_uint8z_array_to_rb_int_array(mesg->rear_gear, (long)mesg->rear_gear_num), 0);
    }

    rb_hash_aset(rh, rb_str_new2("_mapped"), _mapped);
    rb_funcall(cFitHandler, cEventFun, 1, rh);
}


static void pass_device_info(FIT_DEVICE_INFO_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cDeviceInfoFun)) {
        return;
    }

    VALUE rh = rb_hash_new();
    VALUE _mapped = rb_hash_new();

    if (mesg->timestamp != FIT_DATE_TIME_INVALID) {
        hash_set(rh, _mapped, "timestamp",
            fit_time_to_rb(mesg->timestamp),
            fit_time_to_rb_str(mesg->timestamp));
    }

    if (mesg->serial_number != FIT_UINT32Z_INVALID) {
        hash_set(rh, _mapped, "serial_number", UINT2NUM(mesg->serial_number), 0);
    }

    if (mesg->cum_operating_time != FIT_UINT32_INVALID) {
        hash_set(rh, _mapped, "cum_operating_time", UINT2NUM(mesg->cum_operating_time), 0);
    }

    if (mesg->manufacturer != FIT_MANUFACTURER_INVALID) {
        hash_set(rh, _mapped, "manufacturer", UINT2NUM(mesg->manufacturer), MAPID(map_manufacturer, mesg->manufacturer));
    }

    if (mesg->product != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "product", UINT2NUM(mesg->product),
            IS_GARMIN(mesg->manufacturer)
                ? MAPID(map_garmin_product, mesg->product)
                : 0);
    }

    if (mesg->software_version != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "software_version", UINT2NUM(mesg->software_version), 0);
    }

    if (mesg->battery_voltage != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "battery_voltage", rb_float_new(mesg->battery_voltage / 256.0), 0);
    }

    if (mesg->device_index != FIT_DEVICE_INDEX_INVALID) {
        hash_set(rh, _mapped, "device_index", UINT2NUM(mesg->device_index), 0);
    }

    if (mesg->device_type != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "device_type", UINT2NUM(mesg->device_type), MAPID(map_antplus_device_type, mesg->device_type));
    }

    if (mesg->hardware_version != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "hardware_version", UINT2NUM(mesg->hardware_version), 0);
    }

    if (mesg->battery_status != FIT_BATTERY_STATUS_INVALID) {
        hash_set(rh, _mapped, "battery_status", UINT2NUM(mesg->battery_status), MAPID(map_battery_status, mesg->battery_status));
    }

    if (mesg->sensor_position != FIT_BODY_LOCATION_INVALID) {
        hash_set(rh, _mapped, "sensor_position", UINT2NUM(mesg->sensor_position), MAPID(map_body_location, mesg->sensor_position));
    }

    if (mesg->descriptor[0] != FIT_STRING_INVALID) {
        hash_set(rh, _mapped, "descriptor", rb_str_new2(mesg->descriptor), 0);
    }

    if (mesg->ant_transmission_type != FIT_UINT8Z_INVALID) {
        hash_set(rh, _mapped, "ant_transmission_type", UINT2NUM(mesg->ant_transmission_type), 0);
    }

    if (mesg->ant_network != FIT_ANT_NETWORK_INVALID) {
        hash_set(rh, _mapped, "ant_network", UINT2NUM(mesg->ant_network), MAPID(map_ant_network, mesg->ant_network));
    }

    if (mesg->source_type != FIT_SOURCE_TYPE_INVALID) {
        hash_set(rh, _mapped, "source_type", UINT2NUM(mesg->source_type), MAPID(map_source_type, mesg->source_type));
    }

    rb_hash_aset(rh, rb_str_new2("_mapped"), _mapped);
    rb_funcall(cFitHandler, cDeviceInfoFun, 1, rh);
}


static void pass_workout(FIT_WORKOUT_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cWorkoutFun)) {
        return;
    }

    VALUE rh = rb_hash_new();
    VALUE _mapped = rb_hash_new();

    if (mesg->capabilities != FIT_WORKOUT_CAPABILITIES_INVALID) {
        hash_set(rh, _mapped, "capabilities", UINT2NUM(mesg->capabilities), 0);
    }

    if (mesg->wkt_name[0] != FIT_STRING_INVALID) {
        hash_set(rh, _mapped, "wkt_name", rb_str_new2(mesg->wkt_name), 0);
    }

    if (mesg->num_valid_steps != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "num_valid_steps", UINT2NUM(mesg->num_valid_steps), 0);
    }

    if (mesg->sport != FIT_SPORT_INVALID) {
        hash_set(rh, _mapped, "sport", UINT2NUM(mesg->sport), MAPID(map_sport, mesg->sport));
    }

    rb_hash_aset(rh, rb_str_new2("_mapped"), _mapped);
    rb_funcall(cFitHandler, cWorkoutFun, 1, rh);
}


static void pass_workout_step(FIT_WORKOUT_STEP_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cWorkoutStepFun)) {
        return;
    }

    VALUE rh = rb_hash_new();
    VALUE _mapped = rb_hash_new();

    if (mesg->wkt_step_name[0] != FIT_STRING_INVALID) {
        hash_set(rh, _mapped, "wkt_step_name", rb_str_new2(mesg->wkt_step_name), 0);
    }

    if (mesg->duration_value != FIT_UINT32_INVALID) {
        hash_set(rh, _mapped, "duration_value", UINT2NUM(mesg->duration_value), 0);
    }

    if (mesg->target_value != FIT_UINT32_INVALID) {
        hash_set(rh, _mapped, "target_value", UINT2NUM(mesg->target_value), 0);
    }

    if (mesg->custom_target_value_low != FIT_UINT32_INVALID) {
        hash_set(rh, _mapped, "custom_target_value_low", UINT2NUM(mesg->custom_target_value_low), 0);
    }

    if (mesg->custom_target_value_high != FIT_UINT32_INVALID) {
        hash_set(rh, _mapped, "custom_target_value_high", UINT2NUM(mesg->custom_target_value_high), 0);
    }

    if (mesg->message_index != FIT_MESSAGE_INDEX_INVALID) {
        hash_set(rh, _mapped, "message_index", UINT2NUM(mesg->message_index), 0);
    }

    if (mesg->duration_type != FIT_WKT_STEP_DURATION_INVALID) {
        hash_set(rh, _mapped, "duration_type", UINT2NUM(mesg->duration_type), MAPID(map_wkt_step_duration, mesg->duration_type));
    }

    if (mesg->target_type != FIT_WKT_STEP_TARGET_INVALID) {
        hash_set(rh, _mapped, "target_type", UINT2NUM(mesg->target_type), MAPID(map_wkt_step_target, mesg->target_type));
    }

    if (mesg->intensity != FIT_INTENSITY_INVALID) {
        hash_set(rh, _mapped, "intensity", UINT2NUM(mesg->intensity), MAPID(map_intensity, mesg->intensity));
    }

    rb_hash_aset(rh, rb_str_new2("_mapped"), _mapped);
    rb_funcall(cFitHandler, cWorkoutStepFun, 1, rh);
}


static void pass_schedule(FIT_SCHEDULE_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cScheduleFun)) {
        return;
    }

    VALUE rh = rb_hash_new();
    VALUE _mapped = rb_hash_new();

    if (mesg->serial_number != FIT_UINT32Z_INVALID) {
        hash_set(rh, _mapped, "serial_number", UINT2NUM(mesg->serial_number), 0);
    }

    if (mesg->time_created != FIT_DATE_TIME_INVALID) {
        hash_set(rh, _mapped, "time_created",
            fit_time_to_rb(mesg->time_created),
            fit_time_to_rb_str(mesg->time_created));
    }

    if (mesg->scheduled_time != FIT_DATE_TIME_INVALID) {
        hash_set(rh, _mapped, "scheduled_time",
            fit_time_to_rb(mesg->scheduled_time),
            fit_time_to_rb_str(mesg->scheduled_time));
    }

    if (mesg->manufacturer != FIT_MANUFACTURER_INVALID) {
        hash_set(rh, _mapped, "manufacturer", UINT2NUM(mesg->manufacturer), MAPID(map_manufacturer, mesg->manufacturer));
    }

    if (mesg->product != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "product", UINT2NUM(mesg->product),
            IS_GARMIN(mesg->manufacturer)
                ? MAPID(map_garmin_product, mesg->product)
                : 0);
    }

    if (mesg->completed != FIT_BOOL_INVALID) {
        hash_set(rh, _mapped, "completed", fit_bool_to_rb_bool(mesg->completed), 0);
    }

    if (mesg->type != FIT_SCHEDULE_INVALID) {
        hash_set(rh, _mapped, "type", UINT2NUM(mesg->type), MAPID(map_schedule, mesg->type));
    }

    rb_hash_aset(rh, rb_str_new2("_mapped"), _mapped);
    rb_funcall(cFitHandler, cScheduleFun, 1, rh);
}


static void pass_weight_scale_info(FIT_WEIGHT_SCALE_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cWeightScaleInfoFun)) {
        return;
    }

    VALUE rh = rb_hash_new();
    VALUE _mapped = rb_hash_new();

    if (mesg->timestamp != FIT_DATE_TIME_INVALID) {
        hash_set(rh, _mapped, "timestamp",
            fit_time_to_rb(mesg->timestamp),
            fit_time_to_rb_str(mesg->timestamp));
    }

    if (mesg->weight != FIT_WEIGHT_INVALID) {
        hash_set(rh, _mapped, "weight", rb_float_new(mesg->weight / 100.0), 0);
    }

    if (mesg->percent_fat != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "percent_fat", rb_float_new(mesg->percent_fat / 100.0), 0);
    }

    if (mesg->percent_hydration != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "percent_hydration", rb_float_new(mesg->percent_hydration / 100.0), 0);
    }

    if (mesg->visceral_fat_mass != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "visceral_fat_mass", rb_float_new(mesg->visceral_fat_mass / 100.0), 0);
    }

    if (mesg->bone_mass != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "bone_mass", rb_float_new(mesg->bone_mass / 100.0), 0);
    }

    if (mesg->muscle_mass != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "muscle_mass", rb_float_new(mesg->muscle_mass / 100.0), 0);
    }

    if (mesg->basal_met != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "basal_met", rb_float_new(mesg->basal_met / 4.0), 0);
    }

    if (mesg->active_met != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "active_met", rb_float_new(mesg->active_met / 4.0), 0);
    }

    if (mesg->user_profile_index != FIT_MESSAGE_INDEX_INVALID) {
        hash_set(rh, _mapped, "user_profile_index", UINT2NUM(mesg->user_profile_index), 0);
    }

    if (mesg->physique_rating != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "physique_rating", UINT2NUM(mesg->physique_rating), 0);
    }

    if (mesg->metabolic_age != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "metabolic_age", UINT2NUM(mesg->metabolic_age), 0);
    }

    if (mesg->visceral_fat_rating != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "visceral_fat_rating", UINT2NUM(mesg->visceral_fat_rating), 0);
    }

    rb_hash_aset(rh, rb_str_new2("_mapped"), _mapped);
    rb_funcall(cFitHandler, cWeightScaleInfoFun, 1, rh);
}


static void pass_course(FIT_COURSE_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cCourseFun)) {
        return;
    }

    VALUE rh = rb_hash_new();
    VALUE _mapped = rb_hash_new();

    if (mesg->name[0] != FIT_STRING_INVALID) {
        hash_set(rh, _mapped, "name", rb_str_new2(mesg->name), 0);
    }

    if (mesg->capabilities != FIT_COURSE_CAPABILITIES_INVALID) {
        hash_set(rh, _mapped, "capabilities", UINT2NUM(mesg->capabilities), 0);
    }

    if (mesg->sport != FIT_SPORT_INVALID) {
        hash_set(rh, _mapped, "sport", UINT2NUM(mesg->sport), MAPID(map_sport, mesg->sport));
    }

    rb_hash_aset(rh, rb_str_new2("_mapped"), _mapped);
    rb_funcall(cFitHandler, cCourseFun, 1, rh);
}


static void pass_course_point(FIT_COURSE_POINT_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cCoursePointFun)) {
        return;
    }

    VALUE rh = rb_hash_new();
    VALUE _mapped = rb_hash_new();

    if (mesg->timestamp != FIT_DATE_TIME_INVALID) {
        hash_set(rh, _mapped, "timestamp",
            fit_time_to_rb(mesg->timestamp),
            fit_time_to_rb_str(mesg->timestamp));
    }

    if (mesg->position_lat != FIT_SINT32_INVALID) {
        hash_set(rh, _mapped, "position_lat", fit_pos_to_rb(mesg->position_lat), 0);
    }

    if (mesg->position_long != FIT_SINT32_INVALID) {
        hash_set(rh, _mapped, "position_long", fit_pos_to_rb(mesg->position_long), 0);
    }

    if (mesg->distance != FIT_UINT32_INVALID) {
        hash_set(rh, _mapped, "distance", rb_float_new(mesg->distance / 100.0), 0);
    }

    if (mesg->name[0] != FIT_STRING_INVALID) {
        hash_set(rh, _mapped, "name", rb_str_new2(mesg->name), 0);
    }

    if (mesg->message_index != FIT_MESSAGE_INDEX_INVALID) {
        hash_set(rh, _mapped, "message_index", UINT2NUM(mesg->message_index), 0);
    }

    if (mesg->type != FIT_COURSE_POINT_INVALID) {
        hash_set(rh, _mapped, "type", UINT2NUM(mesg->type), MAPID(map_course_point, mesg->type));
    }

    rb_hash_aset(rh, rb_str_new2("_mapped"), _mapped);
    rb_funcall(cFitHandler, cCoursePointFun, 1, rh);
}


static void pass_totals(FIT_TOTALS_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cTotalsFun)) {
        return;
    }

    VALUE rh = rb_hash_new();
    VALUE _mapped = rb_hash_new();

    if (mesg->timestamp != FIT_DATE_TIME_INVALID) {
        hash_set(rh, _mapped, "timestamp",
            fit_time_to_rb(mesg->timestamp),
            fit_time_to_rb_str(mesg->timestamp));
    }

    if (mesg->timer_time != FIT_UINT32_INVALID) {
        hash_set(rh, _mapped, "timer_time", UINT2NUM(mesg->timer_time), 0);
    }

    if (mesg->distance != FIT_UINT32_INVALID) {
        hash_set(rh, _mapped, "distance", UINT2NUM(mesg->distance), 0);
    }

    if (mesg->calories != FIT_UINT32_INVALID) {
        hash_set(rh, _mapped, "calories", UINT2NUM(mesg->calories), 0);
    }

    if (mesg->elapsed_time != FIT_UINT32_INVALID) {
        hash_set(rh, _mapped, "elapsed_time", UINT2NUM(mesg->elapsed_time), 0);
    }

    if (mesg->active_time != FIT_UINT32_INVALID) {
        hash_set(rh, _mapped, "active_time", UINT2NUM(mesg->active_time), 0);
    }

    if (mesg->message_index != FIT_MESSAGE_INDEX_INVALID) {
        hash_set(rh, _mapped, "message_index", UINT2NUM(mesg->message_index), 0);
    }

    if (mesg->sessions != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "sessions", UINT2NUM(mesg->sessions), 0);
    }

    if (mesg->sport != FIT_SPORT_INVALID) {
        hash_set(rh, _mapped, "sport", UINT2NUM(mesg->sport), MAPID(map_sport, mesg->sport));
    }

    rb_hash_aset(rh, rb_str_new2("_mapped"), _mapped);
    rb_funcall(cFitHandler, cTotalsFun, 1, rh);
}


static void pass_activity(FIT_ACTIVITY_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cActivityFun)) {
        return;
    }

    VALUE rh = rb_hash_new();
    VALUE _mapped = rb_hash_new();

    if (mesg->timestamp != FIT_DATE_TIME_INVALID) {
        hash_set(rh, _mapped, "timestamp",
            fit_time_to_rb(mesg->timestamp),
            fit_time_to_rb_str(mesg->timestamp));
    }

    if (mesg->total_timer_time != FIT_UINT32_INVALID) {
        hash_set(rh, _mapped, "total_timer_time", rb_float_new(mesg->total_timer_time / 1000.0), 0);
    }

    if (mesg->local_timestamp != FIT_DATE_TIME_INVALID) {
        hash_set(rh, _mapped, "local_timestamp",
            fit_time_to_rb(mesg->local_timestamp),
            fit_time_to_rb_str(mesg->local_timestamp));
    }

    if (mesg->num_sessions != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "num_sessions", UINT2NUM(mesg->num_sessions), 0);
    }

    if (mesg->type != FIT_ACTIVITY_INVALID) {
        hash_set(rh, _mapped, "type", UINT2NUM(mesg->type), MAPID(map_activity, mesg->type));
    }

    if (mesg->event != FIT_EVENT_INVALID) {
        hash_set(rh, _mapped, "event", UINT2NUM(mesg->event), MAPID(map_event, mesg->event));
    }

    if (mesg->event_type != FIT_EVENT_TYPE_INVALID) {
        hash_set(rh, _mapped, "event_type", UINT2NUM(mesg->event_type), MAPID(map_event_type, mesg->event_type));
    }

    if (mesg->event_group != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "event_group", UINT2NUM(mesg->event_group), 0);
    }

    rb_hash_aset(rh, rb_str_new2("_mapped"), _mapped);
    rb_funcall(cFitHandler, cActivityFun, 1, rh);
}


static void pass_software(FIT_SOFTWARE_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cSoftwareFun)) {
        return;
    }

    VALUE rh = rb_hash_new();
    VALUE _mapped = rb_hash_new();

    if (mesg->part_number[0] != FIT_STRING_INVALID) {
        hash_set(rh, _mapped, "part_number", rb_str_new2(mesg->part_number), 0);
    }

    if (mesg->message_index != FIT_MESSAGE_INDEX_INVALID) {
        hash_set(rh, _mapped, "message_index", UINT2NUM(mesg->message_index), 0);
    }

    if (mesg->version != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "version", UINT2NUM(mesg->version), 0);
    }

    rb_hash_aset(rh, rb_str_new2("_mapped"), _mapped);
    rb_funcall(cFitHandler, cSoftwareFun, 1, rh);
}


static void pass_file_capabilities(FIT_FILE_CAPABILITIES_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cFileCapabilitiesFun)) {
        return;
    }

    VALUE rh = rb_hash_new();
    VALUE _mapped = rb_hash_new();

    if (mesg->directory[0] != FIT_STRING_INVALID) {
        hash_set(rh, _mapped, "directory", rb_str_new2(mesg->directory), 0);
    }

    if (mesg->max_size != FIT_UINT32_INVALID) {
        hash_set(rh, _mapped, "max_size", UINT2NUM(mesg->max_size), 0);
    }

    if (mesg->message_index != FIT_MESSAGE_INDEX_INVALID) {
        hash_set(rh, _mapped, "message_index", UINT2NUM(mesg->message_index), 0);
    }

    if (mesg->max_count != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "max_count", UINT2NUM(mesg->max_count), 0);
    }

    if (mesg->type != FIT_FILE_INVALID) {
        hash_set(rh, _mapped, "type", UINT2NUM(mesg->type), MAPID(map_file, mesg->type));
    }

    if (mesg->flags != FIT_FILE_FLAGS_INVALID) {
        hash_set(rh, _mapped, "flags", UINT2NUM(mesg->flags), 0);
    }

    rb_hash_aset(rh, rb_str_new2("_mapped"), _mapped);
    rb_funcall(cFitHandler, cFileCapabilitiesFun, 1, rh);
}


static void pass_mesg_capabilities(FIT_MESG_CAPABILITIES_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cMesgCapabilitiesFun)) {
        return;
    }

    VALUE rh = rb_hash_new();
    VALUE _mapped = rb_hash_new();

    if (mesg->message_index != FIT_MESSAGE_INDEX_INVALID) {
        hash_set(rh, _mapped, "message_index", UINT2NUM(mesg->message_index), 0);
    }
    
    if (mesg->mesg_num != FIT_MESG_NUM_INVALID) {
        hash_set(rh, _mapped, "mesg_num", UINT2NUM(mesg->mesg_num), MAPID(map_mesg_num, mesg->mesg_num));
    }
    
    if (mesg->count != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "count", UINT2NUM(mesg->count), 0);
    }
    
    if (mesg->file != FIT_FILE_INVALID) {
        hash_set(rh, _mapped, "file", UINT2NUM(mesg->file), MAPID(map_file, mesg->file));
    }
    
    if (mesg->count_type != FIT_MESG_COUNT_INVALID) {
        hash_set(rh, _mapped, "count_type", UINT2NUM(mesg->count_type), MAPID(map_mesg_count, mesg->count_type));
    }
    
    rb_hash_aset(rh, rb_str_new2("_mapped"), _mapped);
    rb_funcall(cFitHandler, cMesgCapabilitiesFun, 1, rh);
}


static void pass_field_capabilities(FIT_FIELD_CAPABILITIES_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cFieldCapabilitiesFun)) {
        return;
    }

    VALUE rh = rb_hash_new();
    VALUE _mapped = rb_hash_new();

    if (mesg->message_index != FIT_MESSAGE_INDEX_INVALID) {
        hash_set(rh, _mapped, "message_index", UINT2NUM(mesg->message_index), 0);
    }

    if (mesg->mesg_num != FIT_MESG_NUM_INVALID) {
        hash_set(rh, _mapped, "mesg_num", UINT2NUM(mesg->mesg_num), MAPID(map_mesg_num, mesg->mesg_num));
    }

    if (mesg->count != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "count", UINT2NUM(mesg->count), 0);
    }

    if (mesg->file != FIT_FILE_INVALID) {
        hash_set(rh, _mapped, "file", UINT2NUM(mesg->file), MAPID(map_file, mesg->file));
    }

    if (mesg->field_num != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "field_num", UINT2NUM(mesg->field_num), 0);
    }

    rb_hash_aset(rh, rb_str_new2("_mapped"), _mapped);
    rb_funcall(cFitHandler, cFieldCapabilitiesFun, 1, rh);
}


static void pass_file_creator(FIT_FILE_CREATOR_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cFileCreatorFun)) {
        return;
    }

    VALUE rh = rb_hash_new();
    VALUE _mapped = rb_hash_new();

    if (mesg->software_version != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "software_version", UINT2NUM(mesg->software_version), 0);
    }

    if (mesg->hardware_version != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "hardware_version", UINT2NUM(mesg->hardware_version), 0);
    }

    rb_hash_aset(rh, rb_str_new2("_mapped"), _mapped);
    rb_funcall(cFitHandler, cFileCreatorFun, 1, rh);
}


static void pass_blood_pressure(FIT_BLOOD_PRESSURE_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cBloodPressureFun)) {
        return;
    }

    VALUE rh = rb_hash_new();
    VALUE _mapped = rb_hash_new();

    if (mesg->timestamp != FIT_DATE_TIME_INVALID) {
        hash_set(rh, _mapped, "timestamp",
            fit_time_to_rb(mesg->timestamp),
            fit_time_to_rb_str(mesg->timestamp));
    }

    if (mesg->systolic_pressure != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "systolic_pressure", UINT2NUM(mesg->systolic_pressure), 0);
    }

    if (mesg->diastolic_pressure != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "diastolic_pressure", UINT2NUM(mesg->diastolic_pressure), 0);
    }

    if (mesg->mean_arterial_pressure != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "mean_arterial_pressure", UINT2NUM(mesg->mean_arterial_pressure), 0);
    }

    if (mesg->map_3_sample_mean != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "map_3_sample_mean", UINT2NUM(mesg->map_3_sample_mean), 0);
    }

    if (mesg->map_morning_values != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "map_morning_values", UINT2NUM(mesg->map_morning_values), 0);
    }

    if (mesg->map_evening_values != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "map_evening_values", UINT2NUM(mesg->map_evening_values), 0);
    }

    if (mesg->user_profile_index != FIT_MESSAGE_INDEX_INVALID) {
        hash_set(rh, _mapped, "user_profile_index", UINT2NUM(mesg->user_profile_index), 0);
    }

    if (mesg->heart_rate != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "heart_rate", UINT2NUM(mesg->heart_rate), 0);
    }

    if (mesg->heart_rate_type != FIT_HR_TYPE_INVALID) {
        hash_set(rh, _mapped, "heart_rate_type", UINT2NUM(mesg->heart_rate_type), MAPID(map_hr_type, mesg->heart_rate_type));
    }

    if (mesg->status != FIT_BP_STATUS_INVALID) {
        hash_set(rh, _mapped, "status", UINT2NUM(mesg->status), MAPID(map_bp_status, mesg->status));
    }

    rb_hash_aset(rh, rb_str_new2("_mapped"), _mapped);
    rb_funcall(cFitHandler, cBloodPressureFun, 1, rh);
}


static void pass_speed_zone(FIT_SPEED_ZONE_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cSpeedZoneFun)) {
        return;
    }

    VALUE rh = rb_hash_new();
    VALUE _mapped = rb_hash_new();

    if (mesg->name[0] != FIT_STRING_INVALID) {
        hash_set(rh, _mapped, "name", rb_str_new2(mesg->name), 0);
    }

    if (mesg->message_index != FIT_MESSAGE_INDEX_INVALID) {
        hash_set(rh, _mapped, "message_index", UINT2NUM(mesg->message_index), 0);
    }

    if (mesg->high_value != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "high_value", rb_float_new(mesg->high_value / 1000.0), 0);
    }

    rb_hash_aset(rh, rb_str_new2("_mapped"), _mapped);
    rb_funcall(cFitHandler, cSpeedZoneFun, 1, rh);
}


static void pass_monitoring(FIT_MONITORING_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cMonitoringFun)) {
        return;
    }

    VALUE rh = rb_hash_new();
    VALUE _mapped = rb_hash_new();

    if (mesg->timestamp != FIT_DATE_TIME_INVALID) {
        hash_set(rh, _mapped, "timestamp",
            fit_time_to_rb(mesg->timestamp),
            fit_time_to_rb_str(mesg->timestamp));
    }

    if (mesg->distance != FIT_UINT32_INVALID) {
        hash_set(rh, _mapped, "distance", rb_float_new(mesg->distance / 100.0), 0);
    }

    if (mesg->cycles != FIT_UINT32_INVALID) {
        hash_set(rh, _mapped, "cycles", rb_float_new(mesg->cycles / 2.0), 0);
    }

    if (mesg->active_time != FIT_UINT32_INVALID) {
        hash_set(rh, _mapped, "active_time", rb_float_new(mesg->active_time / 1000.0), 0);
    }

    if (mesg->local_timestamp != FIT_DATE_TIME_INVALID) {
        hash_set(rh, _mapped, "local_timestamp",
            fit_time_to_rb(mesg->local_timestamp),
            fit_time_to_rb_str(mesg->local_timestamp));
    }

    if (mesg->calories != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "calories", UINT2NUM(mesg->calories), 0);
    }

    if (mesg->distance_16 != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "distance_16", rb_float_new(mesg->distance_16 / 100.0), 0);
    }

    if (mesg->cycles_16 != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "cycles_16", rb_float_new(mesg->cycles_16 / 2.0), 0);
    }

    if (mesg->active_time_16 != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "active_time_16", UINT2NUM(mesg->active_time_16), 0);
    }

    if (mesg->device_index != FIT_DEVICE_INDEX_INVALID) {
        hash_set(rh, _mapped, "device_index", UINT2NUM(mesg->device_index), 0);
    }

    if (mesg->activity_type != FIT_ACTIVITY_TYPE_INVALID) {
        hash_set(rh, _mapped, "activity_type", UINT2NUM(mesg->activity_type), MAPID(map_activity_type, mesg->activity_type));
    }

    if (mesg->activity_subtype != FIT_ACTIVITY_SUBTYPE_INVALID) {
        hash_set(rh, _mapped, "activity_subtype", UINT2NUM(mesg->activity_subtype), MAPID(map_activity_subtype, mesg->activity_subtype));
    }

    rb_hash_aset(rh, rb_str_new2("_mapped"), _mapped);
    rb_funcall(cFitHandler, cMonitoringFun, 1, rh);
}


static void pass_hrv(FIT_HRV_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cHrvFun)) {
        return;
    }

    VALUE rh = rb_hash_new();
    VALUE _mapped = rb_hash_new();

    if (is_valid_uint16_array(mesg->time, DIM(mesg->time))) {
        hash_set(rh, _mapped, "time", fit_uint16_array_to_rb_float_array(mesg->time, DIM(mesg->time), 1000.0), 0);
    }

    rb_hash_aset(rh, rb_str_new2("_mapped"), _mapped);
    rb_funcall(cFitHandler, cHrvFun, 1, rh);
}


static void pass_length(FIT_LENGTH_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cLengthFun)) {
        return;
    }

    VALUE rh = rb_hash_new();
    VALUE _mapped = rb_hash_new();

    if (mesg->timestamp != FIT_DATE_TIME_INVALID) {
        hash_set(rh, _mapped, "timestamp",
            fit_time_to_rb(mesg->timestamp),
            fit_time_to_rb_str(mesg->timestamp));
    }

    if (mesg->start_time != FIT_DATE_TIME_INVALID) {
        hash_set(rh, _mapped, "start_time",
            fit_time_to_rb(mesg->start_time),
            fit_time_to_rb_str(mesg->start_time));
    }

    if (mesg->total_elapsed_time != FIT_UINT32_INVALID) {
        hash_set(rh, _mapped, "total_elapsed_time", rb_float_new(mesg->total_elapsed_time / 1000.0), 0);
    }

    if (mesg->total_timer_time != FIT_UINT32_INVALID) {
        hash_set(rh, _mapped, "total_timer_time", rb_float_new(mesg->total_timer_time / 1000.0), 0);
    }

    if (mesg->message_index != FIT_MESSAGE_INDEX_INVALID) {
        hash_set(rh, _mapped, "message_index", UINT2NUM(mesg->message_index), 0);
    }

    if (mesg->total_strokes != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "total_strokes", UINT2NUM(mesg->total_strokes), 0);
    }

    if (mesg->avg_speed != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "avg_speed", rb_float_new(mesg->avg_speed / 1000.0), 0);
    }

    if (mesg->total_calories != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "total_calories", UINT2NUM(mesg->total_calories), 0);
    }

    if (mesg->player_score != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "player_score", UINT2NUM(mesg->player_score), 0);
    }

    if (mesg->opponent_score != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "opponent_score", UINT2NUM(mesg->opponent_score), 0);
    }

    if (is_valid_uint16_array(mesg->stroke_count, DIM(mesg->stroke_count))) {
        hash_set(rh, _mapped, "stroke_count", fit_uint16_array_to_rb_int_array(mesg->stroke_count, DIM(mesg->stroke_count)), 0);
    }

    if (is_valid_uint16_array(mesg->zone_count, DIM(mesg->zone_count))) {
        hash_set(rh, _mapped, "zone_count", fit_uint16_array_to_rb_int_array(mesg->zone_count, DIM(mesg->zone_count)), 0);
    }

    if (mesg->event != FIT_EVENT_INVALID) {
        hash_set(rh, _mapped, "event", UINT2NUM(mesg->event), MAPID(map_event, mesg->event));
    }

    if (mesg->event_type != FIT_EVENT_TYPE_INVALID) {
        hash_set(rh, _mapped, "event_type", UINT2NUM(mesg->event_type), MAPID(map_event_type, mesg->event_type));
    }

    if (mesg->swim_stroke != FIT_SWIM_STROKE_INVALID) {
        hash_set(rh, _mapped, "swim_stroke", UINT2NUM(mesg->swim_stroke), MAPID(map_swim_stroke, mesg->swim_stroke));
    }

    if (mesg->avg_swimming_cadence != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "avg_swimming_cadence", UINT2NUM(mesg->avg_swimming_cadence), 0);
    }

    if (mesg->event_group != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "event_group", UINT2NUM(mesg->event_group), 0);
    }

    if (mesg->length_type != FIT_LENGTH_TYPE_INVALID) {
        hash_set(rh, _mapped, "length_type", UINT2NUM(mesg->length_type), MAPID(map_length_type, mesg->length_type));
    }

    rb_hash_aset(rh, rb_str_new2("_mapped"), _mapped);
    rb_funcall(cFitHandler, cLengthFun, 1, rh);
}


static void pass_monitoring_info(FIT_MONITORING_INFO_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cMonitoringInfoFun)) {
        return;
    }

    VALUE rh = rb_hash_new();
    VALUE _mapped = rb_hash_new();

    if (mesg->timestamp != FIT_DATE_TIME_INVALID) {
        hash_set(rh, _mapped, "timestamp",
            fit_time_to_rb(mesg->timestamp),
            fit_time_to_rb_str(mesg->timestamp));
    }

    if (mesg->local_timestamp != FIT_DATE_TIME_INVALID) {
        hash_set(rh, _mapped, "local_timestamp",
            fit_time_to_rb(mesg->local_timestamp),
            fit_time_to_rb_str(mesg->local_timestamp));
    }

    rb_hash_aset(rh, rb_str_new2("_mapped"), _mapped);
    rb_funcall(cFitHandler, cMonitoringInfoFun, 1, rh);
}


static void pass_slave_device(FIT_SLAVE_DEVICE_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cSlaveDeviceFun)) {
        return;
    }

    VALUE rh = rb_hash_new();
    VALUE _mapped = rb_hash_new();

    if (mesg->manufacturer != FIT_MANUFACTURER_INVALID) {
        hash_set(rh, _mapped, "manufacturer", UINT2NUM(mesg->manufacturer), MAPID(map_manufacturer, mesg->manufacturer));
    }

    if (mesg->product != FIT_UINT16_INVALID) {
        hash_set(rh, _mapped, "product", UINT2NUM(mesg->product),
            IS_GARMIN(mesg->manufacturer)
                ? MAPID(map_garmin_product, mesg->product)
                : 0);
    }

    rb_hash_aset(rh, rb_str_new2("_mapped"), _mapped);
    rb_funcall(cFitHandler, cSlaveDeviceFun, 1, rh);
}


static void pass_cadence_zone(FIT_CADENCE_ZONE_MESG *mesg) {
    if (!rb_respond_to(cFitHandler, cCadenceZoneFun)) {
        return;
    }

    VALUE rh = rb_hash_new();
    VALUE _mapped = rb_hash_new();

    if (mesg->name[0] != FIT_STRING_INVALID) {
        hash_set(rh, _mapped, "name", rb_str_new2(mesg->name), 0);
    }

    if (mesg->message_index != FIT_MESSAGE_INDEX_INVALID) {
        hash_set(rh, _mapped, "message_index", UINT2NUM(mesg->message_index), 0);
    }

    if (mesg->high_value != FIT_UINT8_INVALID) {
        hash_set(rh, _mapped, "high_value", UINT2NUM(mesg->high_value), 0);
    }

    rb_hash_aset(rh, rb_str_new2("_mapped"), _mapped);
    rb_funcall(cFitHandler, cCadenceZoneFun, 1, rh);
}


static void pass_pad(FIT_PAD_MESG_DEF *mesg) {
    if (!rb_respond_to(cFitHandler, cPadFun)) {
        return;
    }

    VALUE rh = rb_hash_new();
    VALUE _mapped = rb_hash_new();

    hash_set(rh, _mapped, "reserved_1", UINT2NUM(mesg->reserved_1), 0);
    hash_set(rh, _mapped, "arch", UINT2NUM(mesg->arch),
        (mesg->arch == 0) ? "little_endian" : "big_endian"
        );
    hash_set(rh, _mapped, "global_mesg_num", UINT2NUM(mesg->global_mesg_num), MAPID(map_mesg_num, mesg->global_mesg_num));
    hash_set(rh, _mapped, "num_fields", UINT2NUM(mesg->num_fields), 0);

    rb_hash_aset(rh, rb_str_new2("_mapped"), _mapped);
    rb_funcall(cFitHandler, cUnknownFun, 1, rh);
}


static void pass_unknown(FIT_UINT16 mesg_num, const FIT_UINT8 *mesg) {
    if (!rb_respond_to(cFitHandler, cUnknownFun)) {
        return;
    }

    VALUE rh = rb_hash_new();
    VALUE _mapped = rb_hash_new();

    hash_set(rh, _mapped, "mesg_num", UINT2NUM(mesg_num), 0);
    hash_set(rh, _mapped, "raw_mesg", rb_str_new((char *)mesg, FIT_MESG_SIZE), 0);

    rb_hash_aset(rh, rb_str_new2("_mapped"), _mapped);
    rb_funcall(cFitHandler, cUnknownFun, 1, rh);
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

        case FIT_MESG_NUM_SLAVE_DEVICE:
            pass_slave_device((FIT_SLAVE_DEVICE_MESG *)mesg);
            break;

        case FIT_MESG_NUM_CADENCE_ZONE:
            pass_cadence_zone((FIT_CADENCE_ZONE_MESG *)mesg);
            break;

        case FIT_MESG_NUM_PAD:
            pass_pad((FIT_PAD_MESG_DEF *)mesg);
            break;

        default:
            pass_unknown(mesg_num, mesg);
            break;

#if 0
        case FIT_MESG_NUM_MEMO_GLOB:
            pass_memo_glob((FIT_MEMO_GLOB_MESG *)mesg);
            break;
#endif
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

    if (!fit_validate_header(buffer, length)) {
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


static VALUE init(VALUE self, VALUE handler) {
	cFitHandler = handler;
	rb_ivar_set(self, HANDLER_ATTR, handler);

	//callbacks
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
    cUnknownFun = rb_intern("on_unknown");

	return Qnil;
}


void Init_rubyfitkm() {
    init_maps();

    mRubyFitKM = rb_define_module("RubyFitKM");
    cFitParser = rb_define_class_under(mRubyFitKM, "FitParser", rb_cObject);

	//instance methods
	rb_define_method(cFitParser, "initialize", init, 1);
	rb_define_method(cFitParser, "parse", parse, 1);

	//attributes
	HANDLER_ATTR = rb_intern("@handler");
	rb_define_attr(cFitParser, "handler", 1, 1);
}
