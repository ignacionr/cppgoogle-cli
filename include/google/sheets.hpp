#pragma once

#include <exception>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include "auth.hpp"
#include "../sheet.hpp"

using json = nlohmann::json;

namespace ignacionr::google
{
    class sheets
    {
    public:
        static constexpr auto scope = "https://www.googleapis.com/auth/spreadsheets";
        sheets(auth &oauth)
            : baseUrl("https://sheets.googleapis.com/v4/spreadsheets/"), oauth_{oauth} {}

        auto load_sheet(std::string sheetId, std::string tab_name)
        {
            sheet result{[this, sheetId, tab_name]() -> nlohmann::json
                         {
                             auto result = FetchData(sheetId, tab_name);
                             return result["values"];
                         },
                         [this, sheetId, tab_name](int row, int col, nlohmann::json const &values)
                         {
                             std::string range = tab_name + "!" + getColumnName(col) + std::to_string(row + 1);
                             WriteData(sheetId, range, values);
                         }};
            result.load();
            return result;
        }

    private:
        static std::string getColumnName(int colNum)
        {
            std::string colName;
            ++colNum; // Switch to 1-based index
            while (colNum)
            {
                --colNum; // Adjust for 0-based character index
                char c = 'A' + (colNum % 26);
                colName = c + colName;
                colNum /= 26;
            }
            return colName;
        }

        nlohmann::json FetchData(const std::string &sheetId, const std::string &range)
        {
            std::string url = baseUrl + sheetId + "/values/" + range;
            cpr::Header headers{{"Authorization", "Bearer " + oauth_.token()}};

            auto response = cpr::Get(cpr::Url{url}, headers);
            if (response.status_code == 200)
            {
                auto j = json::parse(response.text);
                return j; // Parse and return as needed
            }
            else
            {
                throw std::runtime_error("Could not fetch.");
            }
        }

        void CreateNewSheet(const std::string &title, std::function<void(const std::string &)> withNewId)
        {
            json sheetData{
                {"properties", {{"title", title}}}};

            std::string url = baseUrl;
            cpr::Header headers{{"Authorization", "Bearer " + oauth_.token()}};
            auto response = cpr::Post(cpr::Url{url}, cpr::Body{sheetData.dump()}, headers, cpr::Header{{"Content-Type", "application/json"}});

            if (response.status_code == 200)
            {
                auto j = json::parse(response.text);
                std::string newSheetId = j["spreadsheetId"];
                withNewId(newSheetId);
            }
            else
            {
                // Handle error
                std::cerr << "Failed to create new sheet. Error: " << response.status_code << std::endl;
            }
        }

        void AddNewSheetToExistingSpreadsheet(const std::string &spreadsheetId, const std::string &sheetTitle, std::function<void(int)> doWithTabId)
        {
            // Create JSON payload
            json sheetData{
                {"requests", {{{"addSheet", {{"properties", {{"title", sheetTitle}}}}}}}}};

            std::string url = baseUrl + spreadsheetId + ":batchUpdate";
            cpr::Header headers{{"Authorization", "Bearer " + oauth_.token()}};
            auto response = cpr::Post(cpr::Url{url}, cpr::Body{sheetData.dump()}, headers, cpr::Header{{"Content-Type", "application/json"}});

            if (response.status_code == 200)
            {
                auto j = json::parse(response.text);
                int newTabId = j["replies"][0]["addSheet"]["properties"]["sheetId"];
                doWithTabId(newTabId);
            }
            else
            {
                std::cerr << "Failed to add new sheet to existing spreadsheet. Error: " << response.status_code << std::endl;
            }
        }

        void WriteData(const std::string &sheetId, const std::string &range, const json &values)
        {
            std::string url = baseUrl + sheetId + "/values/" + range + "?valueInputOption=RAW";
            cpr::Header headers{
                {"Authorization", "Bearer " + oauth_.token()},
                {"Content-Type", "application/json"}
                };

            json body{{"values", values}};
            auto response = cpr::Put(cpr::Url{url}, cpr::Body{body.dump()}, headers);

            if (response.status_code != 200)
            {
                std::cerr << "Failed to write data. Error: " << response.status_code << " >>>" << response.text << std::endl;
            }
        }

        std::string baseUrl;
        auth &oauth_;
    };
}