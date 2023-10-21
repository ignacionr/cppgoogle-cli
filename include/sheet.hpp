#pragma once
#include <functional>
#include <nlohmann/json.hpp>

namespace ignacionr
{
    class sheet
    {
    public:
        using loader_t = std::function<nlohmann::json()>;
        using saver_t = std::function<void(int, int, nlohmann::json)>;

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

            void include_row(int row)
            {
                if (min_row == -1 || row < min_row)
                    min_row = row;
                if (max_row == -1 || row > max_row)
                    max_row = row;
            }

            void include_col(int col)
            {
                if (min_col == -1 || col < min_col)
                    min_col = col;
                if (max_col == -1 || col > max_col)
                    max_col = col;
            }

            void include(int row, int col)
            {
                include_row(row);
                include_col(col);
            }
        };

        rectangle rectangle_diff() const
        {
            rectangle rect{-1, -1, -1, -1};

            int max_height = std::max(values_.size() * values_.is_array(),
                baseline_.size() * baseline_.is_array());
            auto max_width_of = [](const auto &table) -> int
            {
                int max_width = 0;
                for (const auto &row : table)
                {
                    if (row.size() > max_width)
                    {
                        max_width = row.size();
                    }
                }
                return max_width;
            };
            int max_width = std::max(max_width_of(values_) * values_.is_array(),
             max_width_of(baseline_) * baseline_.is_array());

            if (!(values_.is_array() && baseline_.is_array())) {
                rect.include(max_height - 1, max_width -1);
                return rect;
            }

            for (int row = 0; row < max_height; ++row)
            {
                if ((values_.size() <= row) || (baseline_.size() <= row) || !baseline_.is_array())
                {
                    rect.include_row(row);
                    rect.include_col(0);
                    rect.include_col(max_width - 1);
                }
                else
                {
                    auto &vr = values_[row];
                    auto &br = baseline_[row];
                    if (vr.is_array() != br.is_array())
                    {
                        rect.include_row(row);
                        rect.include_col(0);
                        rect.include_col(max_width - 1);
                    }
                    else if (vr.is_array())
                    {
                        for (int col = 0; col < max_width; ++col)
                        {
                            if (
                                ((vr.size() > col) != (br.size() > col)) ||
                                ((vr.size() > col) && (vr[col] != br[col])))
                            {
                                rect.include(row, col);
                            }
                        }
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
