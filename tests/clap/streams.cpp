#include "host.hpp"
#include <algorithm>
#include <cstring>

namespace clap_test {
std::string Host::Save() const {
  std::string data;
  const clap_ostream_t out{
      &data,
      [](const clap_ostream_t *stream, const void *buffer,
         uint64_t size) -> int64_t {
        const auto count =
            std::min<uint64_t>(size, 13); // Exercise partial writes.
        static_cast<std::string *>(stream->ctx)
            ->append(static_cast<const char *>(buffer), count);
        return static_cast<int64_t>(count);
      }};
  Require(state->save(plugin, &out), "save project state");
  return data;
}
bool Host::Load(const std::string &data) {
  struct Reader {
    const std::string &data;
    std::size_t offset{};
  } reader{data};
  const clap_istream_t in{
      &reader,
      [](const clap_istream_t *stream, void *buffer, uint64_t size) -> int64_t {
        auto &r = *static_cast<Reader *>(stream->ctx);
        const auto count =
            std::min<uint64_t>({size, 17, r.data.size() - r.offset});
        std::memcpy(buffer, r.data.data() + r.offset, count);
        r.offset += count;
        return static_cast<int64_t>(count);
      }};
  return state->load(plugin, &in);
}
} // namespace clap_test
