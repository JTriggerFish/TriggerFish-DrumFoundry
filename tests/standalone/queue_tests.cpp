#include "../../adapters/shared/prepared_mailbox.hpp"
#include "event_queue.hpp"
#include <iostream>
#include <stdexcept>
#include <thread>

using drumfoundry::standalone::Event;
using drumfoundry::standalone::EventQueue;
void Require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
int main() {
  try {
    EventQueue<4> small;
    Event event{};
    Require(!small.Pop(event), "empty queue");
    for (int i = 0; i < 3; ++i)
      Require(small.Push({false, 101, double(i), {}}), "capacity");
    Require(!small.Push({}) && small.Dropped() == 1,
            "visible overflow, no overwrite");
    for (int i = 0; i < 3; ++i)
      Require(small.Pop(event) && event.value == i, "FIFO");
    Require(!small.Pop(event), "fully drained");
    small.Push({});
    small.DiscardPending();
    Require(!small.Pop(event), "discard stale events");
    Require(small.Push({false, 101, 42, {}}) && small.Pop(event) &&
                event.value == 42,
            "publication after discard");

    EventQueue<> queue;
    constexpr unsigned count = 100000;
    std::thread writer([&] {
      for (unsigned i = 0; i < count; ++i)
        while (!queue.Push({false, 101, double(i), {}}))
          std::this_thread::yield();
    });
    bool ordered = true;
    for (unsigned i = 0; i < count; ++i) {
      while (!queue.Pop(event))
        std::this_thread::yield();
      ordered = ordered && event.value == i;
    }
    writer.join();
    Require(ordered, "concurrent wraparound publication");
    drumfoundry::host::PreparedMailbox<unsigned> prepared;
    std::atomic<bool> finished{};
    std::thread preparer([&] {
      for (unsigned i = 1; i <= count; ++i)
        prepared.Publish(std::make_unique<unsigned>(i));
      finished.store(true);
    });
    unsigned last = 0;
    bool monotonic = true;
    const auto consume = [&](unsigned &value) {
      monotonic &= value > last;
      last = value;
    };
    while (!finished.load())
      prepared.Consume(consume);
    prepared.Consume(consume);
    preparer.join();
    Require(monotonic && last == count,
            "latest prepared state survives concurrent publication");
    prepared.Publish(std::make_unique<unsigned>(count + 1));
    prepared.Cancel();
    prepared.Consume(consume);
    Require(last == count, "cancelled prepared state cannot reach audio");
    std::cout << "Bounded event queue checks passed\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
