#pragma once
#include <functional>
#include <ranges>
#include <nlohmann/json.hpp>

namespace ignacionr
{
    class sheet
    {
    public:
        using loader_t = std::function<nlohmann::json()>;
        using saver_t = std::function<void(int,int,nlohmann:json)>;

        sheet(loader_t loader, saver_t saver) : loader_{loader}, saver_{saver} {}
        void load()
        {
            baseline_ = values_ = loader_();
        }
        void save()
        {
            auto rect = rectangle_diff();
            if (!rect.empty())
            {
                saver_(rect.min_row, rect.min_col, rect.values(values_));
                baseline_ = values_;
            }
        }
        nlohmann::json &values()
        {
            return values_;
        }
        nlohmann::json const &values() const
        {
            return values_;
        }
    private:
        struct rectangle
        {
            int min_row, min_col, max_row, max_col;
            bool empty() const { return min_row == -1; }
            nlohmann::json values(nlohmann::json const &source) const
            {
                nlohmann::json result = nlohmann::json::array();
                if (!empty())
                {
                    for (int row = min_row; row <= max_row; ++row)
                    {
                        auto r = nlohmann::json::array();
                        if (source[row].is_array())
                        {
                            for (int col = min_col; col <= max_col; ++col)
                            {
                                r[col - min_col] = source[row][col];
                            }
                        }
                        result[row - min_row] = r;
                    }
                }
                return result;
            }
        };

        rectangle rectangle_diff() const
        {
            rectangle rect{-1, -1, -1, -1};

            int max_height = std::max(values_.size(), baseline_.size());
            auto max_width_of = [](auto const &table) -> int
            {
                auto transformed_table = table |
                                         std::views::transform(
                                             [](const auto &row)
                                             {
                                                 return row.is_array() ? row.size() : 0;
                                             });
                auto max_iter = std::ranges::max_element(transformed_table);
                return (max_iter != transformed_table.end()) ? *max_iter : 0;
            };

            int max_width = std::max(max_width_of(values_), max_width_of(baseline_));

            for (int row = 0; row < max_height; ++row)
            {
                for (int col = 0; col < max_width; ++col)
                {
                    auto &vr = values_[row];
                    auto &br = baseline_[row];
                    if ((vr.is_array() != br.is_array()) ||
                        (vr.is_array() && vr.size() > col && vr.size() > col && vr[col] != br[col]))
                    {
                        if (rect.min_row == -1 || row < rect.min_row)
                            rect.min_row = row;
                        if (rect.min_col == -1 || col < rect.min_col)
                            rect.min_col = col;
                        if (rect.max_row == -1 || row > rect.max_row)
                            rect.max_row = row;
                        if (rect.max_col == -1 || col > rect.max_col)
                            rect.max_col = col;
                    }
                }
            }

            return rect;
        }

        loader_t loader_;
        saver_t saver_;
        nlohmann::json values_;
        nlohmann::json baseline_;
    };
}
