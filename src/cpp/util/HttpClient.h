#pragma once

#include <fmt/format.h>

#include <memory>
#include <string>

namespace util {

// all hidden in cpp because compile time of httplib is abmyssal
class HttpClient {
public:
    // Actually creates the HttpClient
    static auto create(char const* schemeHostPort) -> std::unique_ptr<HttpClient>;

    // Fetches the formatted path. Throws an exception on error, otherwise returns the content as std::string.
    template <typename... Args>
    [[nodiscard]] auto get(char const* pathFormat, Args&&... args) {
        return get(fmt::format(pathFormat, std::forward<Args>(args)...).c_str());
    }

    // Fetches the path. Throws an exception on error, otherwise returns the content as std::string.
    [[nodiscard]] virtual auto get(char const* path) -> std::string = 0;

    virtual ~HttpClient() = default;
    HttpClient() = default;

    HttpClient(HttpClient const&) = delete;
    HttpClient(HttpClient&&) = delete;
    auto operator=(HttpClient const&) -> HttpClient& = delete;
    auto operator=(HttpClient &&) -> HttpClient& = delete;
};

} // namespace util
