#include "image.h"

#include <algorithm>
#include <cassert>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <random>
#include <sstream>
#include <string>
#include <unordered_set>

typedef bgr_color<std::uint8_t> bgr_color_uint8;
typedef bgr_color<double> bgr_color_double;
typedef image<bgr_color_uint8> bgr_image_uint8_t;
typedef std::unordered_set<img_pos> img_position_set;

const bgr_color_uint8 invalid_color(0, 0, 0);
const std::int32_t spread = 1;

img_position_set get_free_neighbours(const bgr_image_uint8_t& img,
    const img_pos& pos)
{
    std::vector<img_pos> neighbours;
    neighbours.reserve(8);
    for (std::int32_t nx = pos.x_ - spread; nx <= pos.x_ + spread; ++nx)
        for (std::int32_t ny = pos.y_ - spread; ny <= pos.y_ + spread; ++ny)
            neighbours.push_back(img_pos(nx, ny));
    img_position_set result;
    std::copy_if(
        neighbours.begin(), neighbours.end(),
        std::inserter(result, result.begin()),
        [&img](img_pos pos_n) -> bool
    {
        if (pos_n.x_ < 0 || pos_n.x_ >= img.size().width_ ||
            pos_n.y_ < 0 || pos_n.y_ >= img.size().height_)
            return false;
        return img.pixel(pos_n) == invalid_color;
    });
    return result;
}

double color_pos_difference(const bgr_image_uint8_t& img,
    const img_pos& pos, const bgr_color_uint8& color)
{
    double diff = 0;
    int color_count = 0;
    for (std::int32_t nx = pos.x_ - spread; nx <= pos.x_ + spread; ++nx)
    {
        for (std::int32_t ny = pos.y_ - spread; ny <= pos.y_ + spread; ++ny)
        {
            if (nx < 0 || nx >= img.size().width_ ||
                ny < 0 || ny >= img.size().height_)
                continue;
            bgr_color_uint8 pixel_color = img.pixel(img_pos(nx, ny));
            if (pixel_color == invalid_color)
                continue;
            diff += color_distance(color, pixel_color);
            ++color_count;
        }
    }
    // Avoid division by zero.
    const double divisor = std::max(color_count, 1);
    // Square divisor to avoid coral like growing.
    // This also reduces the number of currently open border pixels.
    return diff/(divisor*divisor);
}

img_pos find_best_pos(const bgr_image_uint8_t& img,
    const img_position_set& next_positions,
    const bgr_color_uint8& color, std::mt19937& g)
{
    typedef std::pair<double, img_pos> rated_position;
    std::vector<rated_position> rated_positions;
    rated_positions.reserve(next_positions.size());
    std::transform(next_positions.begin(), next_positions.end(),
        std::back_inserter(rated_positions),
        [&img, &color](const img_pos& pos) -> rated_position
    {
        return std::make_pair(color_pos_difference(img, pos, color), pos);
    });
    std::shuffle(rated_positions.begin(), rated_positions.end(), g);
    return std::min_element(rated_positions.begin(), rated_positions.end(),
        [](const rated_position& rp1,
            const rated_position& rp2)
        -> bool
    {
        return rp1.first < rp2.first;
    })->second;
}

img_position_set init(int argc, char *argv[], const bgr_image_uint8_t& img)
{
    const std::vector<std::string> arguments( argv + 1, argv + argc );
    if ( arguments.size() != 1 )
    {
        std::cout << "Usage: all_colors [2/3/4]" << std::endl;
        return img_position_set();
    }

    img_position_set init_position;
    const auto add_relative_positions =
        [&init_position, &img](double fx, double fy)
    {
        init_position.insert(img_pos(
            static_cast<std::int32_t>(fx * img.size().width_),
            static_cast<std::int32_t>(fy * img.size().height_)));
    };
    if (arguments[0] == "1")
    {
        add_relative_positions(0.5, 0.5);
    }
    else if (arguments[0] == "2")
    {
        add_relative_positions(0.33, 0.5);
        add_relative_positions(0.67, 0.5);
    }
    else if (arguments[0] == "3")
    {
        add_relative_positions(0.33, 0.40);
        add_relative_positions(0.50, 0.69);
        add_relative_positions(0.67, 0.40);
    }
    else if (arguments[0] == "4")
    {
        add_relative_positions(0.33, 0.36);
        add_relative_positions(0.67, 0.36);
        add_relative_positions(0.33, 0.64);
        add_relative_positions(0.67, 0.64);
    }

    img_position_set next_positions;
    for (const auto& pos : init_position)
    {
        std::int32_t length = 5;
        for (std::int32_t nx = pos.x_ - length; nx <= pos.x_ + length; ++nx)
            next_positions.insert(img_pos(nx, pos.y_));
        for (std::int32_t ny = pos.y_ - length; ny <= pos.y_ + length; ++ny)
            next_positions.insert(img_pos(pos.x_, ny));
    };
    return next_positions;
}

bgr_image_uint8_t embellish(const bgr_image_uint8_t& img)
{
    const auto filtered = median_blur_bgr(dilate_bgr(img));

    bgr_image_uint8_t result = img;
    for (std::int32_t y = 0; y < result.size().height_; ++y)
    {
        for (std::int32_t x = 0; x < result.size().width_; ++x)
        {
            const img_pos pos(x, y);
            const auto image_col = img.pixel(pos);
            if (image_col != invalid_color)
            {
                continue;
            }
            const auto filtered_col = filtered.pixel(pos);
            auto col = image_col * 0.5 + filtered_col * 0.5;
            if (col == invalid_color)
            {
                col = bgr_color_uint8(127, 127, 127);
            }
            result.pixel(pos) = col;
        }
    }
    return result;
}

int main(int argc, char *argv[])
{
    bgr_image_uint8_t img(size_2d(1920, 1080), invalid_color);
    img_position_set next_positions = init(argc, argv, img);
    if (!img.size().height_)
        return 1;

    // walk RGB cube
    std::vector<bgr_color_uint8> colors;
    const int col_values = 64;
    const int col_mult = 4;
    for(int b = 1; b < col_values; ++b)
        for(int g = 1; g < 2 * col_values; ++g)
            for(int r = 1; r < 2 * col_values; ++r)
                colors.push_back(bgr_color_uint8(
                    static_cast<std::uint8_t>(col_mult*b),
                    static_cast<std::uint8_t>(col_mult*g/2),
                    static_cast<std::uint8_t>(col_mult*r/2)));

    std::random_device rd;
    std::mt19937 g(0);
    std::shuffle(colors.begin(), colors.end(), g);

    std::sort(colors.begin(), colors.end(),
        [](const bgr_color_uint8& bgr1, const bgr_color_uint8& bgr2) -> bool
    {
        // Other sortings instead of hue (saturation or value)
        // also yield nice results.
        return bgr_to_hsv(bgr1).h_ < bgr_to_hsv(bgr2).h_;
    });

    const std::size_t save_every_n_frames = 512;
    const std::size_t max_frames = colors.size();
    const std::size_t max_saves = max_frames / save_every_n_frames;
    unsigned long long img_num = 0;
    while (!colors.empty() && !next_positions.empty())
    {
        bgr_color_uint8 color = colors.back();
        colors.pop_back();
        img_pos pos = find_best_pos(img, next_positions, color, g);
        auto next_positions_it = next_positions.find(pos);
        assert(next_positions_it != next_positions.end());
        next_positions.erase(next_positions_it);
        img.pixel(pos) = color;
        img_position_set new_free_pos = get_free_neighbours(img, pos);
        next_positions.insert(new_free_pos.begin(), new_free_pos.end());
        if (colors.size() % save_every_n_frames == 0)
        {
            std::stringstream ss;
            std::cout << "image:" << img_num << "/" << max_saves << " "
                << "colors_left:" << colors.size() << " "
                << "border_positions:" << next_positions.size() << std::endl;
            ss << std::setw(4) << std::setfill('0') << ++img_num;
            const std::string img_path = "./output/image" + ss.str() + ".ppm";
            save_image_ppm(embellish(img), img_path);
        }
    }
}
