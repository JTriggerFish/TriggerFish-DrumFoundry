#include <asio.h>
#include <asiodrivers.h>
#include <asiosys.h>
#include <iasiodrv.h>
#include <windows.h>

extern IASIO *theAsioDriver;
extern AsioDrivers *asioDrivers;

int main() {
  // Reproduce RtAudio ownership without loading hardware. ASIOExit must not
  // dereference either the absent SDK owner or this opaque active-driver token;
  // RtAudio closes the actual driver using its own AsioDrivers afterwards.
  int token = 0;
  asioDrivers = nullptr;
  theAsioDriver = reinterpret_cast<IASIO *>(&token);
  if (ASIOExit() != ASE_OK || theAsioDriver != nullptr)
    return 1;
  return ASIOExit() == ASE_OK ? 0 : 2;
}
