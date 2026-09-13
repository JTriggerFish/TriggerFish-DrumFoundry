#pragma once
#include "document.hpp"

namespace drumfoundry::editing {
double Erb(double frequency);
double InverseErb(double rate);
double DecayPosition(double seconds);
double DecaySeconds(double position);
struct DecayKnot {
  int slot;
  double frequency, seconds;
  bool boundary;
};
std::vector<DecayKnot> DecayKnots(const Document &);
double DecayAt(const Document &, double frequency);
void SetDecay(Document &, int slot, double frequency, double seconds);
int InsertDecay(Document &, double frequency, double seconds);
void DeleteDecay(Document &, int slot);
void ShiftDecay(Document &, double octaves);
} // namespace drumfoundry::editing
