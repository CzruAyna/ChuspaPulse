#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

extern std::atomic<bool> isRunning;
extern std::atomic<bool> g_shutdown_requested;
bool is_shutdown_requested();
void setupSignalHandler();

void require(bool value, const char *message) {
  if (!value) {
    throw std::runtime_error(message);
  }
}

void require_shutdown() {
  require(is_shutdown_requested(), "The shutdown getter must become true");
  require(g_shutdown_requested.load(),
          "The public atomic flag must become true");
  require(!isRunning.load(), "The existing application loop must stop");
}

void raise_signal(int signal) {
  require(std::raise(signal) == 0, "The signal could not be raised");
  require_shutdown();
}

void worker_observes(int signal) {
  std::atomic<bool> observed{false};
  std::thread worker([&observed] {
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (std::chrono::steady_clock::now() < deadline) {
      if (is_shutdown_requested()) {
        observed.store(true);
        return;
      }
      std::this_thread::yield();
    }
  });
  const int result = std::raise(signal);
  worker.join();
  require(result == 0, "The signal could not be raised");
  require(observed.load(), "A worker thread did not observe shutdown");
  require_shutdown();
}

int main(int argc, char **argv) {
  try {
    require(argc == 2, "Exactly one scenario is required");
    const std::string scenario{argv[1]};
    require(!g_shutdown_requested.load(), "Shutdown must initially be false");
    require(!is_shutdown_requested(), "The getter must initially return false");
    require(isRunning.load(), "The application must initially be running");
    if (scenario == "initial") {
      return 0;
    }

    setupSignalHandler();
    if (scenario == "setup") {
      setupSignalHandler();
      require(!is_shutdown_requested(),
              "Installing handlers must not request shutdown");
      require(isRunning.load(),
              "Installing handlers must not stop the application");
    } else if (scenario == "sigint") {
      raise_signal(SIGINT);
    } else if (scenario == "sigterm") {
      raise_signal(SIGTERM);
    } else if (scenario == "thread_sigint") {
      worker_observes(SIGINT);
    } else if (scenario == "thread_sigterm") {
      worker_observes(SIGTERM);
    } else if (scenario == "sticky") {
      raise_signal(SIGTERM);
      for (int i = 0; i < 100; ++i) {
        require_shutdown();
      }
      setupSignalHandler();
      require_shutdown();
    } else if (scenario == "repeated_sigint") {
      for (int i = 0; i < 100; ++i) {
        raise_signal(SIGINT);
      }
    } else if (scenario == "repeated_sigterm") {
      for (int i = 0; i < 100; ++i) {
        raise_signal(SIGTERM);
      }
    } else if (scenario == "mixed_signals") {
      for (int i = 0; i < 100; ++i) {
        raise_signal(i % 2 == 0 ? SIGINT : SIGTERM);
      }
    } else if (scenario == "external") {
      std::cout << "READY" << std::endl;
      const auto deadline =
          std::chrono::steady_clock::now() + std::chrono::seconds(5);
      while (isRunning.load() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
      require_shutdown();
      std::cout << "STOPPED_CLEANLY" << std::endl;
    } else {
      throw std::runtime_error("Unknown scenario");
    }
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << std::endl;
    return 1;
  }
}
