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

#ifndef ANU_VOICE_H_
#define ANU_VOICE_H_

#include "avrlib/base.h"
#include "avrlib/op.h"

#include "anu/lfo.h"
#include "anu/envelope.h"
#include "anu/note_stack.h"

namespace anu {

enum PatchParameter {
  PRM_PATCH_VCO_DCO_RANGE,
  PRM_PATCH_VCO_DCO_FINE,
  PRM_PATCH_VCO_DETUNE,
  PRM_PATCH_VCO_FINE,
  PRM_PATCH_VCO_ENV_AMOUNT,
  PRM_PATCH_VCO_LFO_AMOUNT,
  
  PRM_PATCH_PW_ENV_AMOUNT,
  PRM_PATCH_PW_LFO_AMOUNT,
  
  PRM_PATCH_CUTOFF_BIAS,
  PRM_PATCH_CUTOFF_TRACKING,
  PRM_PATCH_CUTOFF_ENV_AMOUNT,
  PRM_PATCH_CUTOFF_LFO_AMOUNT,
  
  PRM_PATCH_ENV_ATTACK,
  PRM_PATCH_ENV_DECAY,
  PRM_PATCH_ENV_SUSTAIN,
  PRM_PATCH_ENV_RELEASE,
  PRM_PATCH_ENV_VCA_MORPH,
  PRM_PATCH_ENV_LEGATO_MODE,
  PRM_PATCH_ENV_PADDING_1,
  PRM_PATCH_ENV_PADDING_2,
  
  PRM_PATCH_LFO_SHAPE,
  PRM_PATCH_LFO_RATE,
  PRM_PATCH_VIBRATO_RATE,
  PRM_PATCH_VIBRATO_DESTINATION,
  
  PRM_PATCH_KBD_GLIDE,
  PRM_PATCH_KBD_VCF_VELOCITY_AMOUNT,
  PRM_PATCH_KBD_VCA_VELOCITY_AMOUNT,
  PRM_PATCH_PADDING,
  
  PRM_PATCH_LAST
};

static const uint8_t kDACStateBufferSize = 4;

struct Patch {
  int8_t vco_dco_range;
  int8_t vco_dco_fine;
  int8_t vco_detune;
  int8_t vco_fine;
  uint8_t vco_env_amount;
  uint8_t vco_lfo_amount;
  
  uint8_t pw_env_amount;
  uint8_t pw_lfo_amount;
  
  uint8_t cutoff_bias;
  uint8_t cutoff_tracking;
  uint8_t cutoff_env_amount;
  uint8_t cutoff_lfo_amount;
  
  uint8_t env_attack;
  uint8_t env_decay;
  uint8_t env_sustain;
  uint8_t env_release;
  uint8_t env_vca_morph;
  uint8_t env_legato_mode;
  uint8_t env_padding[2];
  
  uint8_t lfo_shape;
  uint8_t lfo_rate;
  uint8_t vibrato_rate;
  uint8_t vibrato_destination;
  
  uint8_t kbd_glide;
  uint8_t kbd_velocity_vcf_amount;
  uint8_t kbd_velocity_vca_amount;
  uint8_t padding;
};

struct DACState {
  uint16_t vco_cv;
  uint16_t pw_cv;
  uint16_t vcf_cv;
  uint16_t vca_cv;
};

class Voice {
 public:
  Voice() { }
  ~Voice() { }
  void Init();
  void Refresh();
  
  void NoteOn(
      uint8_t note,
      uint8_t velocity,
      uint8_t slide,
      uint8_t accent,
      bool legato);
  void NoteOff(uint8_t note);
  void ControlChange(uint8_t controller, uint8_t value);
  void PitchBend(uint16_t pitch_bend);
  void Aftertouch(uint8_t velocity);
  void AllSoundOff();
  void ResetAllControllers();
  void GateOn();
  void GateOff();
  
  void SetValue(uint8_t offset, uint8_t value);
  uint8_t GetValue(uint8_t offset) const {
    const uint8_t* bytes;
    bytes = static_cast<const uint8_t*>(static_cast<const void*>(&patch_));
    return bytes[offset];
  }
  
  const DACState& dac_state() const { return dac_state_; }
  
  inline int16_t dco_pitch() const { return dco_pitch_; }
  inline bool gate() const { return vca_envelope_.gate(); }
  inline bool retriggered() const { return retriggered_; }
  inline void ClearRetriggeredFlag() { retriggered_ = false; }
  inline uint8_t lfo() const { return lfo_8_bits_; }
  inline bool at_rest() const {
    return vca_envelope_.segment() == ENV_SEGMENT_DEAD;
  }
  
  inline void set_note(uint8_t note) {
    pitch_ = pitch_target_ = avrlib::U8U8Mul(note, 128);
  }
  
  inline void set_lfo_pll_target_phase(uint8_t step) {
    if (patch_.lfo_rate < 2) {
      lfo_.set_target_phase(static_cast<uint16_t>(step) * 2730);
    }
  }
  
  void SavePatch();
  void ResetToFactoryDefaults();
  
  void Lock(uint16_t vco_cv, uint16_t pw_cv, uint16_t vcf_cv, uint16_t vca_cv) {
    dac_state_.vco_cv = vco_cv;
    dac_state_.pw_cv = pw_cv;
    dac_state_.vcf_cv = vcf_cv;
    dac_state_.vca_cv = vca_cv;
    locked_ = true;
  }
  
  void ReadDACStateSample() {
    if (!locked_) {
      uint8_t r = dac_state_read_ptr_;
      dac_state_ = dac_state_buffer_[r];
      dac_state_read_ptr_ = (r + 1) & (kDACStateBufferSize - 1);
    }
  }
  
  inline uint8_t writable() const {
    return (dac_state_read_ptr_ - dac_state_write_ptr_ - 1) & \
        (kDACStateBufferSize - 1);
  }
  
  inline uint8_t readable() const {
    return (dac_state_write_ptr_ - dac_state_read_ptr_) & \
        (kDACStateBufferSize - 1);
  }
  
  void Unlock() {
    locked_ = false;
  }
  
  inline Patch* mutable_patch() {
    return &patch_;
  }
  
  void Touch() {
    UpdateEnvelopeParameters();
  }
  
 private:
  void WriteDACStateSample();
  void UpdateEnvelopeParameters();
   
  Patch patch_;
  Lfo lfo_;
  Lfo vibrato_lfo_;
  Envelope vcf_envelope_;
  Envelope mod_envelope_;
  Envelope vca_envelope_;
  
  bool locked_;
  bool dirty_;
  bool retriggered_;
  
  uint8_t lfo_8_bits_;
  
  uint16_t vco_cv_;
  uint16_t pw_cv_;
  uint16_t vcf_cv_;
  uint16_t vca_cv_;
  int16_t dco_pitch_;
  
  int16_t vco_cv_offset_;
  uint16_t vco_cv_scale_low_;
  uint16_t vco_cv_scale_high_;

  uint16_t pitch_counter_;
  uint16_t pitch_increment_;
  int16_t pitch_source_;
  int16_t pitch_target_;
  int16_t pitch_;
  
  int16_t mod_pitch_bend_;
  uint8_t mod_wheel_;
  uint8_t mod_wheel_2_;
  uint8_t mod_aftertoutch_;
  uint8_t mod_velocity_;
  uint8_t mod_accent_;
  uint8_t volume_;
  
  DACState dac_state_;
  DACState dac_state_buffer_[kDACStateBufferSize];
  uint8_t dac_state_read_ptr_;
  uint8_t dac_state_write_ptr_;
  
  DISALLOW_COPY_AND_ASSIGN(Voice);
};

};  // namespace anu

#endif  // ANU_VOICE_H_
