#include "editing/files.hpp"
#include "ui/analysis_view.hpp"
#include <chrono>
#include <iostream>
#include <thread>
int main(int argc, char **argv) {
  using namespace drumfoundry;
  if (argc != 2)
    return 1;
  analysis::Request request;
  request.document = editing::ReadFit(argv[1]);
  request.duration = 8;
  analysis::Worker worker;
  const auto start = std::chrono::steady_clock::now();
  worker.Submit(request);
  double first = -1;
  std::shared_ptr<const analysis::Result> result;
  while (!(result = worker.Take())) {
    worker.TakeContext();
    if (!worker.TakeChunks().empty() && first < 0)
      first = std::chrono::duration<double, std::milli>(
                  std::chrono::steady_clock::now() - start)
                  .count();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    if (std::chrono::steady_clock::now() - start > std::chrono::seconds(60))
      return 2;
  }
  if (!result->error.empty()) {
    std::cerr << result->error;
    return 3;
  }
  ui::AnalysisView view;
  view.setBounds(0, 0, 1200, 500);
  const auto drawStart = std::chrono::steady_clock::now();
  view.Set(result);
  const auto refresh = std::chrono::duration<double, std::milli>(
                           std::chrono::steady_clock::now() - drawStart)
                           .count();
  auto progressive = std::make_shared<analysis::Result>(*result);
  view.SetProgress(progressive, 1);
  const auto partialStart = std::chrono::steady_clock::now();
  view.SetProgress(progressive, 1.033);
  const auto partial = std::chrono::duration<double, std::milli>(
                           std::chrono::steady_clock::now() - partialStart)
                           .count();
  std::cout << "8 s preview: " << result->elapsedMs
            << " ms; first chunk: " << first
            << " ms; 1200x500 heatmap preparation: " << refresh
            << " ms; live stripe: " << partial << " ms\n";
}
