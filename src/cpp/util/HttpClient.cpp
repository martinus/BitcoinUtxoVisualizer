#include "HttpClient.h"

#include <fmt/format.h>
#include <httplib.h>

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
        auto res = mClient.Get(path);
        if (res->status != 200) {
            throw std::runtime_error(fmt::format("HttpClient: could not get '{}', got status {}", path, res->status));
        }
        return std::move(res->body);
    }
};

auto HttpClient::create(char const* schemeHostPort) -> std::unique_ptr<HttpClient> {
    return std::make_unique<HttpClientImpl>(schemeHostPort);
}

} // namespace util
