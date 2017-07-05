#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

template <typename T>
struct bgr_color
{
    bgr_color() : b_(T()), g_(T()), r_(T()) {}
    bgr_color(T b, T g, T r) : b_(b), g_(g), r_(r) {}
    T b_;
    T g_;
    T r_;
};

template <typename T>
double color_distance(const bgr_color<T>& a, const bgr_color<T>& b)
{
    const double db = static_cast<double>(b.b_) - static_cast<double>(a.b_);
    const double dg = static_cast<double>(b.g_) - static_cast<double>(a.g_);
    const double dr = static_cast<double>(b.r_) - static_cast<double>(a.r_);
    return std::sqrt(db*db + dg*dg + dr*dr);
}

template <typename T>
bool operator==(const bgr_color<T>& lhs, const bgr_color<T>& rhs)
{
    return
        lhs.b_ == rhs.b_ &&
        lhs.g_ == rhs.g_ &&
        lhs.r_ == rhs.r_;
}

template <typename T>
bool operator!=(const bgr_color<T>& lhs, const bgr_color<T>& rhs)
{
    return !(lhs == rhs);
}

template <typename T>
bgr_color<T> operator*(const bgr_color<T>& col, double x)
{
    return bgr_color<T>(
        static_cast<T>(col.b_ * x),
        static_cast<T>(col.g_ * x),
        static_cast<T>(col.r_ * x));
}

template <typename T>
bgr_color<T> operator+(const bgr_color<T>& lhs, const bgr_color<T>& rhs)
{
    return bgr_color<T>(
        static_cast<T>(lhs.b_ + rhs.b_),
        static_cast<T>(lhs.g_ + rhs.g_),
        static_cast<T>(lhs.r_ + rhs.r_));
}

template <typename T>
struct hsv_color
{
    hsv_color(T h, T s, T v) : h_(h), s_(s), v_(v) {}
    T h_;
    T s_;
    T v_;
};

template <typename T>
hsv_color<double> bgr_to_hsv(const bgr_color<T>& bgr)
{
    const T bc = bgr.b_;
    const T gc = bgr.g_;
    const T rc = bgr.r_;
    const double b = bc / 255.0;
    const double g = gc / 255.0;
    const double r = rc / 255.0;
    const auto maxc = std::max({rc, gc, bc});
    const double v = std::max({r, g, b});
    const double s = v == 0 ? 0 : (v - std::min({r, g, b}))/v;
    double h = 0;
    const double divisor = v - std::min({r, g, b});
    if (divisor == 0)
        return hsv_color<double>(0, 0, 0);
    if (maxc == rc)
        h = 60*(g-b)/divisor;
    else if (maxc == gc)
        h = 120 + 60*(b-r)/divisor;
    else if (maxc == bc)
        h = 240 + 60*(r-g)/divisor;
    else
        assert(false); // If we land here our v is incorrect.
    if (h < 0)
        h += 360;
    return hsv_color<double>(h, s, v);
}

struct img_pos
{
    img_pos(std::int32_t x, std::int32_t y) :
        x_(x), y_(y)
    {}
    std::int32_t x_;
    std::int32_t y_;
};

namespace std
{
    template<>
    struct hash<img_pos>
    {
        inline std::size_t operator()(const img_pos& pos) const
        {
            // see boost::hash_combine
            std::size_t lhs = static_cast<std::size_t>(pos.x_);
            std::size_t rhs = static_cast<std::size_t>(pos.y_);
            return lhs ^ (rhs + 0x9e3779b9 + (lhs << 6) + (lhs >> 2));
        }
    };
}

bool operator==(const img_pos& lhs, const img_pos& rhs)
{
    return lhs.x_ == rhs.x_ && lhs.y_ == rhs.y_;
}

struct size_2d
{
public:
    size_2d(std::int32_t width, std::int32_t height) :
        width_(width), height_(height)
    {}
    std::int32_t area() const { return width_ * height_; }
    std::int32_t width_;
    std::int32_t height_;
};

size_2d operator+(const size_2d& lhs, const size_2d& rhs)
{
    return size_2d(lhs.width_ + rhs.width_, lhs.height_ + rhs.height_);
}

size_2d operator*(std::int32_t x, const size_2d& size)
{
    return size_2d(x * size.width_, x * size.height_);
}

template <typename T>
class image
{
public:
    image(
        const size_2d& size,
        const T& fill_color = T()) :
            size_(size),
            data_(static_cast<std::size_t>(size_.area()) * 3, fill_color)
    {
    };
    const size_2d& size() const { return size_; }
    const T& pixel(const img_pos& pos) const
    {
        return data_[pixel_index(pos)];
    }
    T& pixel(const img_pos& pos)
    {
        return data_[pixel_index(pos)];
    }
private:
    std::size_t pixel_index(const img_pos& pos) const
    {
        return static_cast<std::size_t>(pos.y_ * size().width_ + pos.x_);
    }
    size_2d size_;
    std::vector<T> data_;
};

template <typename T>
bool save_image_ppm(const image<T>& img, const std::string& filepath)
{
    static_assert(std::is_same<T, bgr_color<std::uint8_t>>::value,
        "Only BGR byte images supported.");
    std::ofstream file(filepath, std::fstream::binary);
    if (file.bad())
        return false;
    file
        << "P6 "
        << std::to_string(img.size().width_)
        << " "
        << std::to_string(img.size().height_)
        << " "
        << "255"
        << "\n";
    for (std::int32_t y = 0; y < img.size().height_; ++y)
    {
        for (std::int32_t x = 0; x < img.size().width_; ++x)
        {
            const auto col = img.pixel(img_pos(x, y));
            file << col.r_;
            file << col.g_;
            file << col.b_;
        }
    }
    return true;
}

template <typename T, typename F>
image<T> filter_3x3(F f, const image<T>& img)
{
    static_assert(std::is_same<T, std::uint8_t>::value,
        "Only BGR byte images supported.");

    image<T> result(img.size());
    for (std::int32_t y = 1; y < result.size().height_ - 1; ++y)
    {
        for (std::int32_t x = 1; x < result.size().width_ - 1; ++x)
        {
            const img_pos pos(x, y);
            result.pixel(img_pos(x, y)) = f(
                img.pixel(img_pos(x - 1, y - 1)),
                img.pixel(img_pos(x    , y - 1)),
                img.pixel(img_pos(x + 1, y - 1)),
                img.pixel(img_pos(x - 1, y    )),
                img.pixel(img_pos(x    , y    )),
                img.pixel(img_pos(x + 1, y    )),
                img.pixel(img_pos(x - 1, y + 1)),
                img.pixel(img_pos(x    , y + 1)),
                img.pixel(img_pos(x + 1, y + 1)));
        }
    }
    return result;
}

template <typename T>
std::array<image<T>, 3> split_channels(
    const image<bgr_color<T>>& img)
{
    std::array<image<T>, 3> channels = {
        image<T>(img.size()),
        image<T>(img.size()),
        image<T>(img.size())};
    for (std::int32_t y = 1; y < img.size().height_ - 1; ++y)
    {
        for (std::int32_t x = 1; x < img.size().width_ - 1; ++x)
        {
            const img_pos pos(x, y);
            channels[0].pixel(pos) = img.pixel(pos).b_;
            channels[1].pixel(pos) = img.pixel(pos).g_;
            channels[2].pixel(pos) = img.pixel(pos).r_;
        }
    }
    return channels;
}

template <typename T>
image<bgr_color<T>> merge_channels(
    const std::array<image<T>, 3>& channels)
{
    image<bgr_color<T>> img(channels[0].size());
    for (std::int32_t y = 1; y < img.size().height_ - 1; ++y)
    {
        for (std::int32_t x = 1; x < img.size().width_ - 1; ++x)
        {
            const img_pos pos(x, y);
            img.pixel(pos).b_ = channels[0].pixel(pos);
            img.pixel(pos).g_ = channels[1].pixel(pos);
            img.pixel(pos).r_ = channels[2].pixel(pos);
        }
    }
    return img;
}

template <typename T>
image<T> dilate(const image<T>& img)
{
    const auto f = [](
        const T& x0y0, const T& x1y0, const T& x2y0,
        const T& x0y1, const T& x1y1, const T& x2y1,
        const T& x0y2, const T& x1y2, const T& x2y2) -> T
    {
        return std::max({
            x0y0, x1y0, x2y0,
            x0y1, x1y1, x2y1,
            x0y2, x1y2, x2y2});
    };
    return filter_3x3(f, img);
}

template <typename T>
image<T> median_blur(const image<T>& img)
{
    const auto f = [](
        const T& x0y0, const T& x1y0, const T& x2y0,
        const T& x0y1, const T& x1y1, const T& x2y1,
        const T& x0y2, const T& x1y2, const T& x2y2) -> T
    {
        std::array<T, 9> v{
            x0y0, x1y0, x2y0,
            x0y1, x1y1, x2y1,
            x0y2, x1y2, x2y2};
        std::nth_element(v.begin(), v.begin() + 4, v.end());
        return v[4];
    };
    return filter_3x3(f, img);
}

template <typename F, typename T>
image<bgr_color<T>> apply_to_all_channels(F f,
    const image<bgr_color<T>>& img )
{
    auto channels = split_channels(img);
    for (std::size_t i = 0; i < channels.size(); ++i)
    {
        channels[i] = f(channels[i]);
    }
    return merge_channels(channels);
}

template <typename T>
image<bgr_color<T>> dilate_bgr(const image<bgr_color<T>>& img)
{
    return apply_to_all_channels(dilate<T>, img);
}


template <typename T>
image<bgr_color<T>> median_blur_bgr(const image<bgr_color<T>>& img)
{
    return apply_to_all_channels(median_blur<T>, img);
}
