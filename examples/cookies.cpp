#include <httc/headers.h>
#include <httc/request.h>
#include <httc/response.h>
#include <httc/router.h>
#include <httc/server.h>
#include <httc/status.h>
#include <asio.hpp>
#include <print>
#include <string>

using namespace httc;

// Simple manual cookie parser helper
std::optional<std::string> get_cookie(const Request& req, std::string_view name) {
    auto cookie_header = req.header("Cookie");
    if (!cookie_header) {
        return std::nullopt;
    }

    std::string_view cookies = *cookie_header;
    std::string search_key = std::string(name) + "=";

    size_t start = 0;
    while (start < cookies.length()) {
        while (start < cookies.length() && cookies[start] == ' ') {
            start++;
        }

        size_t end = cookies.find(';', start);
        if (end == std::string_view::npos) {
            end = cookies.length();
        }

        std::string_view cookie = cookies.substr(start, end - start);
        if (cookie.starts_with(search_key)) {
            return std::string(cookie.substr(search_key.length()));
        }

        start = end + 1;
    }

    return std::nullopt;
}

asio::awaitable<void> home_handler(const Request& req, Response& res) {
    auto session_id = get_cookie(req, "session_id");
    res.headers.set("Content-Type", "text/html");

    if (session_id) {
        res.set_body(
            "<html><body style='font-family: sans-serif; text-align: center; padding-top: 50px;'>"
            "<h1 style='color: green;'>Status: Logged In</h1>"
            "<p>Welcome back! You are authenticated as: <b>"
            + *session_id
            + "</b></p>"
              "<div style='margin-top: 20px;'>"
              "  <a href='/dashboard' style='margin-right: 15px;'>Go to Dashboard</a>"
              "  <a href='/logout' style='color: red;'>Logout</a>"
              "</div>"
              "</body></html>"
        );
    } else {
        res.set_body(
            "<html><body style='font-family: sans-serif; text-align: center; padding-top: 50px;'>"
            "<h1 style='color: gray;'>Status: Not Logged In</h1>"
            "<p>You are browsing as a guest.</p>"
            "<div style='margin-top: 20px;'>"
            "  <a href='/dashboard' style='margin-right: 15px;'>Go to Dashboard</a>"
            "  <a href='/login'>Login</a>"
            "</div>"
            "</body></html>"
        );
    }
    co_return;
}

asio::awaitable<void> login_handler(const Request& req, Response& res) {
    // If already logged in, redirect to home
    if (get_cookie(req, "session_id")) {
        res.status = StatusCode::SEE_OTHER;
        res.headers.set("Location", "/");
        co_return;
    }

    if (req.method == "GET") {
        res.headers.set("Content-Type", "text/html");
        res.set_body(
            "<html><body style='font-family: sans-serif; text-align: center; padding-top: 50px;'>"
            "<h1>Login</h1>"
            "<form method='POST' action='/login'>"
            "  <button type='submit' style='padding: 10px 20px; cursor: pointer;'>Login as Test User</button>"
            "</form>"
            "<br><a href='/'>Back Home</a>"
            "</body></html>"
        );
    } else if (req.method == "POST") {
        res.add_cookie("session_id=user_12345; HttpOnly; Path=/; Max-Age=3600");

        res.status = StatusCode::SEE_OTHER;
        res.headers.set("Location", "/");
    }
    co_return;
}

asio::awaitable<void> dashboard_handler(const Request& req, Response& res) {
    auto session_id = get_cookie(req, "session_id");

    if (session_id) {
        res.headers.set("Content-Type", "text/html");
        res.set_body(
            "<html><body style='font-family: sans-serif; padding: 20px;'>"
            "<h1>Dashboard</h1>"
            "<p>This is a protected area.</p>"
            "<ul>"
            "  <li>User ID: "
            + *session_id
            + "</li>"
              "  <li>Secret Data: 42</li>"
              "</ul>"
              "<a href='/'>Back to Home</a> | <a href='/logout'>Logout</a>"
              "</body></html>"
        );
    } else {
        // Unauthorized access to dashboard
        res.status = StatusCode::UNAUTHORIZED;
        res.headers.set("Content-Type", "text/html");
        res.set_body(
            "<html><body style='font-family: sans-serif; text-align: center; padding-top: 50px;'>"
            "<h1 style='color: red;'>401 Unauthorized</h1>"
            "<p>You must be logged in to access the dashboard.</p>"
            "<a href='/login'>Login Here</a>"
            "</body></html>"
        );
    }
    co_return;
}

asio::awaitable<void> logout_handler(const Request&, Response& res) {
    // Clear the session cookie
    res.add_cookie("session_id=; HttpOnly; Path=/; Max-Age=0");

    res.status = StatusCode::SEE_OTHER;
    res.headers.set("Location", "/");
    co_return;
}

int main() {
    asio::io_context io_ctx;
    auto router = std::make_shared<Router>();

    router->route("/", methods::get(home_handler));
    router->route("/login", MethodWrapper<"GET", "POST">(login_handler));
    router->route("/dashboard", methods::get(dashboard_handler));
    router->route("/logout", methods::get(logout_handler));

    bind_and_listen("127.0.0.1", 8080, router, io_ctx);

    std::println("Server running on http://127.0.0.1:8080");
    io_ctx.run();

    return 0;
}
