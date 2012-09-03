// Copyright 2012 Olivier Gillet.
//
// Author: Olivier Gillet (ol.gillet@gmail.com)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// -----------------------------------------------------------------------------
//
// Resources definitions.
//
// Automatically generated with:
// make resources


#ifndef ANU_RESOURCES_H_
#define ANU_RESOURCES_H_


#include "avrlib/base.h"

#include <avr/pgmspace.h>


#include "avrlib/resources_manager.h"

namespace anu {

typedef uint8_t ResourceId;

extern const prog_char* string_table[];

extern const prog_uint16_t* lookup_table_table[];

extern const prog_uint32_t* lookup_table_32_table[];

extern const prog_uint8_t* waveform_table[];

extern const prog_uint16_t lut_res_glide_increments[] PROGMEM;
extern const prog_uint16_t lut_res_drm_env_increments[] PROGMEM;
extern const prog_uint16_t lut_res_drm_phase_increments[] PROGMEM;
extern const prog_uint16_t lut_res_dco_pitch[] PROGMEM;
extern const prog_uint16_t lut_res_env_expo[] PROGMEM;
extern const prog_uint16_t lut_res_groove_swing[] PROGMEM;
extern const prog_uint16_t lut_res_groove_shuffle[] PROGMEM;
extern const prog_uint16_t lut_res_groove_push[] PROGMEM;
extern const prog_uint16_t lut_res_groove_lag[] PROGMEM;
extern const prog_uint16_t lut_res_groove_human[] PROGMEM;
extern const prog_uint16_t lut_res_groove_monkey[] PROGMEM;
extern const prog_uint16_t lut_res_arpeggiator_patterns[] PROGMEM;
extern const prog_uint32_t lut_res_lfo_increments[] PROGMEM;
extern const prog_uint32_t lut_res_env_increments[] PROGMEM;
extern const prog_uint8_t wav_res_deadband[] PROGMEM;
extern const prog_uint8_t wav_res_pitch_deadband[] PROGMEM;
extern const prog_uint8_t wav_res_drm_envelope[] PROGMEM;
extern const prog_uint8_t wav_res_sine[] PROGMEM;
extern const prog_uint8_t wav_res_hh[] PROGMEM;
extern const prog_uint8_t wav_res_drum_map_node_0[] PROGMEM;
extern const prog_uint8_t wav_res_drum_map_node_1[] PROGMEM;
extern const prog_uint8_t wav_res_drum_map_node_2[] PROGMEM;
extern const prog_uint8_t wav_res_drum_map_node_3[] PROGMEM;
extern const prog_uint8_t wav_res_drum_map_node_4[] PROGMEM;
extern const prog_uint8_t wav_res_drum_map_node_5[] PROGMEM;
extern const prog_uint8_t wav_res_drum_map_node_6[] PROGMEM;
extern const prog_uint8_t wav_res_drum_map_node_7[] PROGMEM;
extern const prog_uint8_t wav_res_drum_map_node_8[] PROGMEM;
#define STR_RES_DUMMY 0  // dummy
#define LUT_RES_GLIDE_INCREMENTS 0
#define LUT_RES_GLIDE_INCREMENTS_SIZE 256
#define LUT_RES_DRM_ENV_INCREMENTS 1
#define LUT_RES_DRM_ENV_INCREMENTS_SIZE 256
#define LUT_RES_DRM_PHASE_INCREMENTS 2
#define LUT_RES_DRM_PHASE_INCREMENTS_SIZE 257
#define LUT_RES_DCO_PITCH 3
#define LUT_RES_DCO_PITCH_SIZE 97
#define LUT_RES_ENV_EXPO 4
#define LUT_RES_ENV_EXPO_SIZE 257
#define LUT_RES_GROOVE_SWING 5
#define LUT_RES_GROOVE_SWING_SIZE 16
#define LUT_RES_GROOVE_SHUFFLE 6
#define LUT_RES_GROOVE_SHUFFLE_SIZE 16
#define LUT_RES_GROOVE_PUSH 7
#define LUT_RES_GROOVE_PUSH_SIZE 16
#define LUT_RES_GROOVE_LAG 8
#define LUT_RES_GROOVE_LAG_SIZE 16
#define LUT_RES_GROOVE_HUMAN 9
#define LUT_RES_GROOVE_HUMAN_SIZE 16
#define LUT_RES_GROOVE_MONKEY 10
#define LUT_RES_GROOVE_MONKEY_SIZE 16
#define LUT_RES_ARPEGGIATOR_PATTERNS 11
#define LUT_RES_ARPEGGIATOR_PATTERNS_SIZE 6
#define LUT_RES_LFO_INCREMENTS 0
#define LUT_RES_LFO_INCREMENTS_SIZE 256
#define LUT_RES_ENV_INCREMENTS 1
#define LUT_RES_ENV_INCREMENTS_SIZE 256
#define WAV_RES_DEADBAND 0
#define WAV_RES_DEADBAND_SIZE 256
#define WAV_RES_PITCH_DEADBAND 1
#define WAV_RES_PITCH_DEADBAND_SIZE 256
#define WAV_RES_DRM_ENVELOPE 2
#define WAV_RES_DRM_ENVELOPE_SIZE 257
#define WAV_RES_SINE 3
#define WAV_RES_SINE_SIZE 257
#define WAV_RES_HH 4
#define WAV_RES_HH_SIZE 4097
#define WAV_RES_DRUM_MAP_NODE_0 5
#define WAV_RES_DRUM_MAP_NODE_0_SIZE 48
#define WAV_RES_DRUM_MAP_NODE_1 6
#define WAV_RES_DRUM_MAP_NODE_1_SIZE 48
#define WAV_RES_DRUM_MAP_NODE_2 7
#define WAV_RES_DRUM_MAP_NODE_2_SIZE 48
#define WAV_RES_DRUM_MAP_NODE_3 8
#define WAV_RES_DRUM_MAP_NODE_3_SIZE 48
#define WAV_RES_DRUM_MAP_NODE_4 9
#define WAV_RES_DRUM_MAP_NODE_4_SIZE 48
#define WAV_RES_DRUM_MAP_NODE_5 10
#define WAV_RES_DRUM_MAP_NODE_5_SIZE 48
#define WAV_RES_DRUM_MAP_NODE_6 11
#define WAV_RES_DRUM_MAP_NODE_6_SIZE 48
#define WAV_RES_DRUM_MAP_NODE_7 12
#define WAV_RES_DRUM_MAP_NODE_7_SIZE 48
#define WAV_RES_DRUM_MAP_NODE_8 13
#define WAV_RES_DRUM_MAP_NODE_8_SIZE 48
typedef avrlib::ResourcesManager<
    ResourceId,
    avrlib::ResourcesTables<
        string_table,
        lookup_table_table> > ResourcesManager; 

}  // namespace anu

#endif  // ANU_RESOURCES_H_
