#pragma once

#include <string>
#include <chrono>
#include <vector>
#include <stdexcept>
#include <nlohmann/json.hpp>
#include <cpr/cpr.h>
#include <jwt-cpp/jwt.h>

namespace ignacionr::google {

class auth {
public:
    auth(const nlohmann::json& service_account_json, std::vector<std::string> scopes)
        : service_account_json_(service_account_json), scopes_(scopes) {}

    std::string token() {
        auto now = std::chrono::system_clock::now();
        if (now + std::chrono::minutes{1} < token_expiry_) {
            return current_token_;
        }

        const std::string& private_key = service_account_json_["private_key"];
        const std::string& client_email = service_account_json_["client_email"];

        auto token = jwt::create()
            .set_issuer(client_email)
            .set_type("JWT")
            .set_audience("https://oauth2.googleapis.com/token");

        for (const auto& scope: scopes_) {
            token.set_payload_claim("scope", jwt::claim(scope));
        }

        auto jwt_token = token
            .set_issued_at(std::chrono::system_clock::now())
            .set_expires_at(std::chrono::system_clock::now() + std::chrono::hours{1})
            .sign(jwt::algorithm::rs256("", private_key));

        auto payload = nlohmann::json{
            {"grant_type", "urn:ietf:params:oauth:grant-type:jwt-bearer"},
            {"assertion", jwt_token}
        };

        auto response = cpr::Post(
            cpr::Url{"https://oauth2.googleapis.com/token"},
            cpr::Body{payload.dump()},
            cpr::Header{{"Content-Type", "application/json"}}
        );

        if (response.status_code == 200) {
            auto json_response = nlohmann::json::parse(response.text);
            current_token_ = json_response["access_token"];
            token_expiry_ = std::chrono::system_clock::now() + std::chrono::seconds{json_response["expires_in"]};
            return current_token_;
        } else {
            std::string errorMsg = "Failed to get token. Status code: " + std::to_string(response.status_code) + ". Response: " + response.text;
            throw std::runtime_error(errorMsg);
        }
    }

private:
    nlohmann::json service_account_json_;
    std::vector<std::string> scopes_;
    std::string current_token_;
    std::chrono::system_clock::time_point token_expiry_;
};

} // namespace ignacionr
