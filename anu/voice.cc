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

#include "anu/voice.h"

#include <avr/pgmspace.h>

#include "avrlib/op.h"
#include "midi/midi.h"

#include "anu/storage.h"
#include "anu/system_settings.h"

#define CLIP_12(x) if (x < 0) x = 0; if (x > 4095) x = 4095;

namespace anu {
  
using namespace avrlib;

typedef Patch PROGMEM prog_Patch;

static const prog_Patch init_patch PROGMEM = {
  // VCO
  0, 0, 0, 0, 0, 0, 
  // PW
  0, 128,
  // CUTOFF
  128, 128, 0, 0,
  // ENV
  0, 64, 128, 128, 128, 0, 0, 0,
  
  // LFO
  0, 96, 160, 0,
  
  // Glide and padding
  0, 0, 0, 0
};

const int16_t kVcfCvOffset = 60 * 128;
const int16_t kVcfCvScale = 32000 / 3;

template<>
struct StorageLayout<Patch> {
  static uint8_t* eeprom_address() { 
    return (uint8_t*)(0);
  }
  static const prog_char* init_data() {
    return (prog_char*)&init_patch;
  }
};

void Voice::Init() {
  STATIC_ASSERT(sizeof(Patch) == PRM_PATCH_LAST);
  
  storage.Load(&patch_);
  vcf_envelope_.Init();
  vca_envelope_.Init();
  mod_envelope_.Init();
  
  pitch_ = 0;
  locked_ = false;
  dirty_ = false;
  retriggered_ = false;
  volume_ = 240;

  ResetAllControllers();
  UpdateEnvelopeParameters();
}

void Voice::ControlChange(uint8_t controller, uint8_t value) {
  switch (controller) {
    case midi::kVolume:
      volume_ = value << 1;
      break;
      
    case midi::kModulationWheelMsb:
      mod_wheel_ = value;
      break;
      
    case midi::kBreathController:
      mod_wheel_2_ = value;
      break;
    
  }
}

void Voice::SetValue(uint8_t offset, uint8_t value) {
  uint8_t* bytes;
  bytes = static_cast<uint8_t*>(static_cast<void*>(&patch_));
  uint8_t previous_value = bytes[offset];
  bytes[offset] = value;
  if (offset >= PRM_PATCH_ENV_ATTACK && offset < PRM_PATCH_ENV_PADDING_1) {
    UpdateEnvelopeParameters();
  }
  if (previous_value != value) {
    dirty_ = true;
  }
}

void Voice::UpdateEnvelopeParameters() {
  vcf_envelope_.Update(
      patch_.env_attack,
      patch_.env_decay,
      patch_.env_sustain,
      patch_.env_release);

  mod_envelope_.Update(
      patch_.env_attack,
      patch_.env_decay,
      0,
      patch_.env_decay);

  uint8_t morph = patch_.env_vca_morph;
  uint8_t vca_a, vca_d, vca_s, vca_r;
  if (morph < 128) {
    morph <<= 1;
    vca_a = U8Mix(patch_.env_attack, 0, morph);
    vca_d = U8Mix(patch_.env_decay, patch_.env_decay >> 1, morph);
    vca_s = U8Mix(patch_.env_sustain, 192, morph);
    vca_r = U8Mix(patch_.env_release, patch_.env_release >> 1, morph);
  } else {
    morph = (morph - 128) << 1;
    vca_a = 0;
    vca_d = U8Mix(patch_.env_decay >> 1, 0, morph);
    vca_s = U8Mix(192, 255, morph);
    vca_r = U8Mix(patch_.env_release >> 1, 8, morph);
  }
  vca_envelope_.Update(vca_a, vca_d, vca_s, vca_r);
}

void Voice::PitchBend(uint16_t pitch_bend) {
  mod_pitch_bend_ = pitch_bend;
}

void Voice::Aftertouch(uint8_t velocity) {
  mod_aftertoutch_ = velocity;
}

void Voice::AllSoundOff() {
  vca_envelope_.Trigger(ENV_SEGMENT_DEAD);
  vcf_envelope_.Trigger(ENV_SEGMENT_DEAD);
  mod_envelope_.Trigger(ENV_SEGMENT_DEAD);
}

void Voice::ResetAllControllers() {
  mod_pitch_bend_ = 8192;
  mod_wheel_ = 0;
  mod_wheel_2_ = 0;
  mod_aftertoutch_ = 0;
}

void Voice::Refresh() {
  while (writable()) {
    WriteDACStateSample();
  }
}

void Voice::WriteDACStateSample() {
  uint8_t w = dac_state_write_ptr_;
      
  lfo_.set_shape(static_cast<LfoShape>(patch_.lfo_shape));
  if (patch_.lfo_rate >= 2) {
    lfo_.set_phase_increment(
        pgm_read_dword(lut_res_lfo_increments + patch_.lfo_rate));
  }
  vibrato_lfo_.set_phase_increment(
      pgm_read_dword(lut_res_lfo_increments + 96 + (patch_.vibrato_rate >> 1)));

  uint8_t mod_wheel_pitch = 0;
  uint8_t mod_wheel_growl = 0;
  uint8_t vibrato_destination = patch_.vibrato_destination;
  if (vibrato_destination < 128) {
    mod_wheel_pitch = mod_wheel_;
    mod_wheel_growl = U8U8MulShift8(vibrato_destination << 1, mod_wheel_);
  } else {
    vibrato_destination = ~vibrato_destination;
    mod_wheel_growl = mod_wheel_;
    mod_wheel_pitch = U8U8MulShift8(vibrato_destination << 1, mod_wheel_);
  }
  

  // Compute modulation sources.
  uint16_t lfo_unsigned = lfo_.Render();
  int16_t lfo = lfo_unsigned - 32768;
  int16_t vibrato_lfo = vibrato_lfo_.Render() - 32768;
  lfo_8_bits_ = lfo_unsigned >> 8;
  uint16_t mod_envelope = mod_envelope_.Render();

  // VCO CV.
  pitch_counter_ += pitch_increment_;
  if (pitch_counter_ < pitch_increment_) {
    pitch_counter_ = 0xffff;
    pitch_increment_ = 0;
  }
  int16_t pitch = Mix(pitch_source_, pitch_target_, pitch_counter_);
  pitch_ = pitch;
  pitch += (mod_pitch_bend_ - 8192) >> 5;
  pitch += S8U8Mul(patch_.vco_dco_range, 6) << 8;
  pitch += patch_.vco_dco_fine;
  pitch += S8U8MulShift8(vibrato_lfo >> 8, mod_wheel_pitch);
  dco_pitch_ = pitch;
  
  pitch += S8U8Mul(patch_.vco_detune, 128);
  pitch += patch_.vco_fine;
  pitch += U16U8MulShift8(mod_envelope, patch_.vco_env_amount) >> 4;
  pitch += S16U8MulShift8(lfo, patch_.vco_lfo_amount) >> 4;

  if (pitch < 60 * 128) {
    pitch = static_cast<int32_t>(pitch - system_settings.vco_cv_offset()) * \
        system_settings.vco_cv_scale_low() >> 16;
  } else {
    pitch = static_cast<int32_t>(pitch - system_settings.vco_cv_offset()) * \
        system_settings.vco_cv_scale_high() >> 16;
  }
  pitch += 2048;
  CLIP_12(pitch);
  dac_state_buffer_[w].vco_cv = pitch;
  
  // PW CV.
  int16_t pw = 0;
  pw += U16U8MulShift8(mod_envelope, patch_.pw_env_amount) >> 2;
  pw += U16U8MulShift8(lfo + 32768, patch_.pw_lfo_amount) >> 2;
  pw >>= 2;
  CLIP_12(pw);
  dac_state_buffer_[w].pw_cv = pw;
  
  // VCF CV.
  uint16_t vcf_envelope = vcf_envelope_.Render();
  int16_t cutoff = 60 * 128;
  cutoff += S16U8MulShift8(dco_pitch_ - 60 * 128, patch_.cutoff_tracking) << 1;
  cutoff += S8U8Mul(patch_.cutoff_bias + 128, 64);
  uint16_t growl_amount = mod_wheel_growl;
  growl_amount += mod_wheel_2_;
  // if (mod_aftertoutch_ > 112) {
  //   growl_amount += (mod_aftertoutch_ - 112) << 2;
  // }
  if (growl_amount >= 255) {
    growl_amount = 255;
  }
  cutoff += S16U8MulShift8(vibrato_lfo, growl_amount) >> 3;
  uint16_t env_amount = patch_.cutoff_env_amount + (mod_accent_ >> 1);
  env_amount += U8U8MulShift8(mod_velocity_, patch_.kbd_velocity_vcf_amount);
  if (env_amount > 255) {
    env_amount = 255;
  }
  cutoff += U16U8MulShift8(vcf_envelope, env_amount) >> 2;
  cutoff += S16U8MulShift8(lfo, patch_.cutoff_lfo_amount) >> 3;
  // cutoff += U8U8Mul(mod_aftertoutch_, 64);
  cutoff = static_cast<int32_t>(cutoff - kVcfCvOffset) * kVcfCvScale >> 16;
  cutoff += 2048;
  CLIP_12(cutoff);
  dac_state_buffer_[w].vcf_cv = cutoff;
  
  // VCA CV.
  uint16_t vca_envelope = U16U8MulShift8(
      vca_envelope_.Render(),
      U8Mix(255, mod_velocity_ << 1, patch_.kbd_velocity_vca_amount));
  vca_envelope = U16U8MulShift8(vca_envelope, volume_);
  dac_state_buffer_[w].vca_cv = U16ShiftRight4(vca_envelope);
  dac_state_write_ptr_ = (w + 1) & (kDACStateBufferSize - 1);
}

void Voice::GateOn() {
  vca_envelope_.Trigger(ENV_SEGMENT_ATTACK);
  vcf_envelope_.Trigger(ENV_SEGMENT_ATTACK);
  mod_envelope_.Trigger(ENV_SEGMENT_ATTACK);
  if (gate()) {
    retriggered_ = true;
  }
}

void Voice::GateOff() {
  vca_envelope_.Trigger(ENV_SEGMENT_RELEASE);
  vcf_envelope_.Trigger(ENV_SEGMENT_RELEASE);
  mod_envelope_.Trigger(ENV_SEGMENT_RELEASE);
}

void Voice::NoteOn(
    uint8_t note,
    uint8_t velocity,
    uint8_t slide,
    uint8_t accent,
    bool legato) {
  if (!legato || !patch_.env_legato_mode) {
    GateOn();
  }
  pitch_source_ = pitch_;
  pitch_target_ = U8U8Mul(note, 128);
  velocity += accent;
  if (velocity > 127) {
    velocity = 127;
  }
  mod_velocity_ = velocity;
  mod_accent_ = accent;
  
  // When legato mode is enabled, and when the notes are not played legato,
  // There is no glide applied.
  if (!pitch_source_ || (!legato && patch_.env_legato_mode)) {
    pitch_source_ = pitch_target_;
  }
  uint16_t slide_time = patch_.kbd_glide + slide;
  if (slide_time > 255) {
    patch_.kbd_glide = 255;
  }
  pitch_increment_ = pgm_read_word(lut_res_glide_increments + slide_time);
  pitch_counter_ = 0;
}

void Voice::NoteOff(uint8_t note) {
  if (note != 0xff) {
    GateOff();
  }
}

void Voice::SavePatch() {
  if (dirty_) {
    storage.Save(patch_);
  }
  dirty_ = false;
}

void Voice::ResetToFactoryDefaults() {
  storage.ResetToFactoryDefaults(&patch_);
}

};  // namespace anu
