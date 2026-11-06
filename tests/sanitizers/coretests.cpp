#include "core/connections/connectionencoding.h"
#include "core/connections/connectionstopstate.h"

#include <atomic>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace {
void require(bool condition, const char *message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void encoding()
{
    require(ConnectionEncoding::quote("") == "''", "Empty value must remain quoted");
    require(ConnectionEncoding::quote("my database") == "'my database'", "Spaces changed");
    require(ConnectionEncoding::quote("a'b\\c") == "'a\\'b\\\\c'",
            "Escaping quotes/backslashes failed");
    require(ConnectionEncoding::quote("Слон 🐘") == "'Слон 🐘'", "UTF-8 bytes changed");
    require(ConnectionEncoding::build("host", 1, "user", "db", "secret")
                    == "host='host' port=1 user='user' dbname='db' password='secret' "
                       "connect_timeout=10 application_name=Slonyara",
            "Connection parameters changed");
    require(ConnectionEncoding::build("::1", 65535, "", "", "'")
                    == "host='::1' port=65535 user='' dbname='' password='\\'' "
                       "connect_timeout=10 application_name=Slonyara",
            "Boundary values were encoded incorrectly");
    const std::string longValue(10000, '\\');
    require(ConnectionEncoding::quote(longValue) == "'" + std::string(20000, '\\') + "'",
            "Long values were truncated");
    // Exercise the same encoder concurrently, with independent input/output buffers.
    std::vector<std::thread> threads;
    for (int i = 0; i < 8; ++i) {
        threads.emplace_back([i] {
            for (int j = 0; j < 100; ++j) {
                const auto value = std::to_string(i) + "'\\" + std::to_string(j);
                require(ConnectionEncoding::quote(value)
                                == "'" + std::to_string(i) + "\\'\\\\" + std::to_string(j) + "'",
                        "Concurrent encoding corrupted data");
            }
        });
    }
    for (auto &thread : threads)
        thread.join();
}

void cancellation()
{
    ConnectionStopState cancelled;
    require(!cancelled.stopRequested(), "A new worker must not be cancelled");
    cancelled.requestStop();
    cancelled.requestStop();
    bool called = false;
    cancelled.waitUntilStopped([&] { called = true; });
    require(!called, "A cancelled worker must not announce readiness");

    for (int attempt = 0; attempt < 100; ++attempt) {
        ConnectionStopState state;
        std::atomic<bool> ready{ false };
        std::atomic<bool> returned{ false };
        std::thread waiter([&] {
            state.waitUntilStopped([&] { ready.store(true, std::memory_order_release); });
            returned.store(true, std::memory_order_release);
        });
        while (!ready.load(std::memory_order_acquire))
            std::this_thread::yield();
        require(!returned.load(std::memory_order_acquire), "Worker returned before cancellation");
        std::thread reader([&] {
            while (!state.stopRequested())
                std::this_thread::yield();
        });
        std::thread firstStopper([&] { state.requestStop(); });
        std::thread secondStopper([&] { state.requestStop(); });
        firstStopper.join();
        secondStopper.join();
        reader.join();
        waiter.join();
        require(returned.load(std::memory_order_acquire), "Lost cancellation notification");
    }
}
} // namespace

int main(int argc, char **argv)
{
    require(argc == 2, "Choose encoding or cancellation");
    const std::string suite = argv[1];
    if (suite == "encoding")
        encoding();
    else if (suite == "cancellation")
        cancellation();
    else
        require(false, "Unknown suite");
    std::cout << "PASS " << suite << '\n';
}
