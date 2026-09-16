#include "help.hpp"
#include <map>
namespace drumfoundry::ui {
// Ear-first guidance, retained from the workbench. Equations belong in DSP
// docs.
std::string ParameterHelp(const std::string &key) {
  static const std::map<std::string, std::string> help{
      {"model_level_db",
       "Overall synth volume, without changing its character or bloom. "
       "Reference selection never adjusts it automatically."},
      {"direct_gain", "How much of the initial stick, mallet or brush contact "
                      "you hear alongside the ringing body. Raise for a closer "
                      "attack; lower for a body-led sound."},
      {"impact_tone_noise",
       "Balances pitched stick ping against broadband contact noise."},
      {"impact_width",
       "Shorter gives a sharper tap; longer softens and spreads the contact. "
       "This changes the attack, not the ringing tail."},
      {"impact_chirp_pitch",
       "Retunes the initial ping without retuning the body. Most noticeable "
       "when the contact mix favours ping."},
      {"impact_noise_tilt",
       "Raise for brighter contact hiss; lower for a rounder attack. The "
       "body's tonal balance is unchanged."},
      {"impact_micro_density",
       "Raise for a smoother brush gesture; lower for more separated tiny "
       "contacts. The effect depends on the chosen implement."},
      {"velocity_brightness",
       "Raise for more contrast between dark light taps and bright hard "
       "crashes. Lower for a more consistent colour across playing strengths."},
      {"bloom_rate", "How quickly ringing spreads through the spectrum. Raise "
                     "for a faster crash; lower for a slower bloom. A dark "
                     "strike usually spreads into the highs, but energy can "
                     "travel downward too. Zero turns this movement off."},
      {"bloom_energy_acceleration",
       "Near zero, weakly filled frequency regions let energy through readily. "
       "Higher values favour concentrated regions and can hold back the "
       "developing highs. Use Energy sensitivity for hard-versus-soft strike "
       "response."},
      {"bloom_energy_sensitivity",
       "Makes bloom faster while more energy is stored: hard strikes and "
       "repeated hits can spread faster, then settle as the tail fades. Zero "
       "removes this speed change, not other velocity effects. Does not add "
       "energy."},
      {"body_excitation",
       "How strongly contact drives the ringing body. Raising it also drives "
       "energy-dependent bloom harder. For volume alone, use Model level."},
      {"field_gain", "Listening balance of the ringing body against direct "
                     "contact. It does not change the energy driving bloom."},
      {"body_brightness",
       "Where the strike starts the body ringing. Negative values favour a "
       "dark low start; positive values excite highs immediately. For a low "
       "note opening into a bright bloom, start darker."},
      {"body_excitation_centre",
       "Where the initial brightness slope begins. Lower leaves more highs for "
       "bloom to develop; higher excites a wider range immediately. Use with "
       "Initial excitation tilt."},
      {"body_tune", "Moves body tones together while keeping their frequency "
                    "ratios. One is the painted tuning; two is an octave "
                    "higher. Contact ping has its own pitch control."},
      {"field_turbulence",
       "How much of a ring near 1 kHz spreads into surrounding tones. Lower "
       "keeps a clear pitch; higher gives a fuller cluster. Spread sets their "
       "distance; Density sets how many."},
      {"field_turbulence_slope",
       "Positive values keep lows clearer and make highs noisier; negative "
       "values do the opposite. The amount at 1 kHz stays fixed. This also "
       "changes surrounding tones available for bloom and decay."},
      {"field_packet_spread",
       "Moves surrounding tones farther from each painted mode: focused ring "
       "at low values, broader metallic shimmer at high values. Changes "
       "spacing, not count. Little effect on a mode with zero surrounding "
       "rings."},
      {"field_satellite_density",
       "How many surrounding tones fill each packet. Low values expose "
       "whistles and beats; high values give fuller texture. Zero leaves "
       "painted centres only. Changes count, not intended width; rebuilds the "
       "sound."},
      {"field_distribution",
       "Choose how surrounding tones are spaced. Scattered and even layouts "
       "fill a band; paired layouts favour audible beating. Try Beating "
       "doublets for metallic sizzle with defined ridges."},
      {"field_doublet_split",
       "Paired pulsation speed at 125 Hz: 1 Hz is one beat per second. Try "
       "0.3-1 Hz for slow breathing. Treble speed scaling changes higher "
       "rings. Zero removes deliberate splitting; other close tones can still "
       "beat."},
      {"field_beat_depth",
       "Strength of paired pulsation, independent of its speed. Try 0.1-0.3 "
       "for gentle movement. One gives equal partners and deepest pulses; zero "
       "removes the weaker partner. Packet excitation energy stays the same."},
      {"field_beat_rate_tilt",
       "Let high packets beat faster than low ones. Zero uses the same gap "
       "everywhere; +0.25 doubles the gap over four octaves. Pitch drift adds "
       "irregular timing instead of fixed frequency scaling."},
      {"field_wander_hz",
       "Let pitches drift gently out of tune and back. Try 0.3-1 Hz to make "
       "beating less clockwork. Larger amounts audibly detune; zero switches "
       "drift off. Can combine with faster shimmer."},
      {"field_wander_rate",
       "How often drifting pitches change direction. Try 0.2-1 changes per "
       "second for slow detuning. Each tone moves independently; needs drift "
       "Amount above zero."},
      {"field_motion_depth",
       "Irregular shimmer around ringing pitches. Start gently; increase for "
       "animated sizzle. Zero switches it off. Unlike pitch drift, movement "
       "stays anchored around the original phase. Try with Blur off."},
      {"field_motion_rate",
       "Slow for breathing, faster for flutter and sizzle. Try 10-80 changes "
       "per second, or 80-200 for finer texture. Irregular movement, not "
       "repeating vibrato; needs Shimmer amount above zero."},
      {"field_motion_sharing",
       "Higher values move tones in a packet together, preserving their "
       "beating. Lower values move them separately for less regular beating. "
       "Different packets always move independently."},
      {"field_phase_bandwidth",
       "Softens steady ringing towards a noise wash. A little can tame "
       "synthetic whistles; too much sounds like hiss. Zero preserves tones "
       "and natural beating. Changes coherence, not the damping curve."},
      {"field_phase_tilt",
       "Positive values move blur toward treble; negative values soften lows "
       "and keep highs clearer. Pivots at 1 kHz. Zero retains the base blur "
       "profile, not equal blur everywhere. Needs Blur amount above zero."},
      {"output_eq_enabled", "One final EQ shapes the complete contact/body "
                            "mix. Bypass hears the unfiltered mix. Neither "
                            "setting changes energy inside the instrument."}};
  if (const auto found = help.find(key); found != help.end())
    return found->second;
  if (key.find("low_cut") != std::string::npos)
    return "Raise to remove rumble or weight; lower for a fuller bottom end. "
           "This filters what you hear, not energy stored in the body.";
  if (key.find("high_cut") != std::string::npos)
    return "Lower to soften the top end. To shorten high ringing rather than "
           "just quieten it, use damping/T60 instead.";
  if (key.find("colour_frequency") != std::string::npos)
    return "Where the final colour boost or cut sits. Sweep to find the region "
           "to bring forward or soften.";
  if (key.find("colour_gain") != std::string::npos)
    return "Boosts or softens the region around Colour frequency. Zero leaves "
           "it unchanged.";
  if (key == "output_colour_q")
    return "Width of the colour boost or cut. Low Q shapes a broad region; "
           "high Q focuses on a narrow ring. 0.7 is the original broad setting. "
           "Scroll over the colour handle to change Q; Shift makes finer edits.";
  return {};
}
} // namespace drumfoundry::ui
