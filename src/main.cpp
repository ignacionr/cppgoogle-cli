#include "../include/google/auth.hpp"
#include "../include/google/sheets.hpp"

int main() {
    auto json = nlohmann::json::parse(std::getenv("GOOGLE_SERVICE_ACCOUNT"));
    ignacionr::google::auth authClient(json, {ignacionr::google::sheets::scope});
    
    ignacionr::google::sheets sheets{authClient};
    auto result = sheets.load_sheet("<your sheet id>", "Test234");
    std::cout << result.values() << std::endl;
    result.values()[5][0] = "this is 5,0";
    result.save();
    std::cout << result.values() << std::endl;
    return 0;
}
