#include "HttpClient.h"

#include <fmt/format.h>
#include <httplib.h>

#include <chrono>
#include <thread>

using namespace std::literals;

namespace util {

class HttpClientImpl : public HttpClient {
    httplib::Client mClient;

public:
    explicit HttpClientImpl(char const* schemeHostPort)
        : mClient(schemeHostPort) {}

    // NOLINTNEXTLINE(clang-analyzer-optin.cplusplus.VirtualCall)
    ~HttpClientImpl() override = default;

    HttpClientImpl(HttpClientImpl const&) = delete;
    HttpClientImpl(HttpClientImpl&&) = delete;
    auto operator=(HttpClientImpl const&) -> HttpClientImpl& = delete;
    auto operator=(HttpClientImpl&&) -> HttpClientImpl& = delete;

    [[nodiscard]] auto get(char const* path) -> std::string override {
        auto delay = 10ms;

        // try a few times times, with increasing delay
        for (size_t i = 0; i < 10; ++i) {
            auto res = mClient.Get(path);
            if (res && res->status == 200) {
                return std::move(res->body);
            }
            std::this_thread::sleep_for(delay);
            delay *= 2;
        }

        throw std::runtime_error(fmt::format("HttpClient: could not get '{}'", path));
    }
};

auto HttpClient::create(char const* schemeHostPort) -> std::unique_ptr<HttpClient> {
    return std::make_unique<HttpClientImpl>(schemeHostPort);
}

} // namespace util
