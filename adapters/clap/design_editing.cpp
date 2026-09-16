#include "plugin.hpp"
#include <algorithm>
#include <stdexcept>
#include <thread>

namespace drumfoundry::clap_adapter {
void Plugin::InitializeDesignParameters(const Json &document) {
  const auto &patch = document.at("instrument");
  const auto recipe = ParseRecipe(patch.at("recipe"));
  const auto &table = DesignParameters(); // Warm registry before audio starts.
  for (std::size_t i = 0; i < table.size(); ++i) {
    const auto &p = table[i];
    double value = p.descriptor->defaultValue;
    if (p.recipe == recipe)
      for (const auto &node : patch.at("nodes"))
        if (node.at("id") == p.owner)
          value = node.at("parameters").at(p.descriptor->key);
    desiredDesignValues_[i] = value;
  }
  designPending_ = true;
  if (!active)
    AcceptPendingDesign();
  designRecipe_ = recipe;
  if (hostParams_ && hostParams_->rescan)
    hostParams_->rescan(host_,
                        CLAP_PARAM_RESCAN_INFO | CLAP_PARAM_RESCAN_VALUES);
}

void Plugin::AcceptPendingDesign() noexcept {
  // Called only with processing stopped. Audio retains exclusive ownership of
  // published values throughout the host's asynchronous restart interval.
  if (!designPending_)
    return;
  for (std::size_t i = 0; i < DesignParameters().size(); ++i)
    values_[ParameterCount + i] = desiredDesignValues_[i];
  designPending_ = false;
}

void Plugin::OverlayDesignParameters(Json &document) const {
  if (designPending_)
    return; // document already contains the validated, pending preset.
  const auto &table = DesignParameters();
  std::array<double, DesignCapacity> values{};
  std::array<uint64_t, DesignCapacity> acknowledged{};
  // All payloads are atomic: unlike a plain-data seqlock this has no data race.
  // Copy scalars first, then build JSON outside the retry window.
  for (;;) {
    const auto sequence = designSequence_.load();
    if (sequence & 1) {
      std::this_thread::yield();
      continue;
    }
    for (std::size_t i = 0; i < table.size(); ++i) {
      values[i] = values_[ParameterCount + i].load();
      acknowledged[i] = acknowledgedEdits_[ParameterCount + i].load();
    }
    if (sequence == designSequence_.load())
      break;
  }
  auto proposed = values;
  CrashMacroValues curve{};
  for (std::size_t i = 0; i < table.size(); ++i) {
    const auto &pending = pendingEdits_[ParameterCount + i];
    if (pending.serial > acknowledged[i])
      proposed[i] = pending.value;
    if (table[i].recipe == detail::Recipe::MetallicPlate)
      curve[table[i].index] = float(proposed[i]);
  }
  // Host automation may invalidate a still-queued UI curve. Never serialize
  // that mixture; show the coherent accepted state until the queue is drained.
  if (designRecipe_.load() != detail::Recipe::MetallicPlate ||
      ValidLiveDecay(curve))
    values = proposed;
  for (auto &node : document["instrument"]["nodes"])
    for (std::size_t i = 0; i < table.size(); ++i) {
      const auto &p = table[i];
      if (p.recipe == designRecipe_.load() && node.at("id") == p.owner)
        node["parameters"][p.descriptor->key] = values[i];
    }
}

void Plugin::SetDesignParameter(clap_id id, double value) noexcept {
  DesignWrite publication(*this);
  const auto slot = DesignSlot(id);
  if (slot == DesignCapacity)
    return;
  const auto &p = DesignParameters()[slot];
  const auto recipe =
      active && voice_ ? voice_->Recipe() : designRecipe_.load();
  if (p.recipe != recipe || !ValidDesignValue(p, value))
    return;
  if (active && voice_ && !voice_->StageParameter(p.index, float(value)))
    return;
  if (!active && recipe == detail::Recipe::MetallicPlate) {
    CrashMacroValues curve{};
    const auto &table = DesignParameters();
    for (std::size_t i = 0; i < table.size(); ++i)
      if (table[i].recipe == recipe)
        curve[table[i].index] = float(values_[ParameterCount + i].load());
    curve[p.index] = float(value);
    if (!ValidLiveDecay(curve))
      return;
  }
  values_[ParameterCount + slot] = value;
  ++automationRevision_;
}

void Plugin::QueueDesignEdits(const Json &before, const Json &next) {
  struct Change {
    clap_id id;
    double value;
    int priority;
  };
  std::vector<Change> changes;
  const auto recipe = ParseRecipe(next.at("instrument").at("recipe"));
  for (const auto &p : DesignParameters()) {
    if (p.recipe != recipe)
      continue;
    const auto &nodes = next.at("instrument").at("nodes");
    const auto &oldNodes = before.at("instrument").at("nodes");
    for (std::size_t n = 0; n < nodes.size(); ++n) {
      if (nodes[n].at("id") != p.owner)
        continue;
      const double v = nodes[n].at("parameters").at(p.descriptor->key);
      const double old = oldNodes[n].at("parameters").at(p.descriptor->key);
      if (v != old)
        changes.push_back({p.id, v, DesignEditPriority(p, v, old)});
    }
  }
  // Order coupled T60 edits so each intermediate state remains valid too.
  std::stable_sort(
      changes.begin(), changes.end(),
      [](const auto &a, const auto &b) { return a.priority < b.priority; });
  if (editorParams_->Available() < changes.size() + DesignCapacity)
    throw std::runtime_error(
        "Live control queue full; retry when audio resumes");
  for (const auto &change : changes)
    if (!QueueEdit(change.id, change.value))
      throw std::runtime_error("Invalid live parameter edit");
}

void Plugin::EndDesignGesture() {
  for (std::size_t i = 0; i < DesignParameters().size(); ++i) {
    if (!designGesturesMain_[i])
      continue;
    // QueueEdit reserves this capacity; the sole producer cannot overrun it.
    if (!editorParams_->Push(
            {false, DesignParameters()[i].id, 0, {}, 0, ++editSerial_, 3}))
      throw std::runtime_error("Could not finish automation gesture");
    designGesturesMain_[i] = false;
  }
  if (hostParams_ && hostParams_->request_flush)
    hostParams_->request_flush(host_);
}
} // namespace drumfoundry::clap_adapter
