#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <list>
#include <random>
#include <set>
#include <sstream>
#include <tuple>

#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

typedef int16_t Channel;
typedef int16_t PosComponent;

const int ImageType = CV_16SC3;

typedef Vec<Channel, 3> Color;
typedef pair<PosComponent, PosComponent> Pos;

Channel invalidColor = -1;

void SetPixel(Mat& image, PosComponent x, PosComponent y, const Color& color)
{
	image.at<Color>(y, x) = color;
}

Color GetPixel(const Mat& image, PosComponent x, PosComponent y)
{
	return image.at<Color>(y, x);
}

set<Pos> GetFreeNeighbours(const Mat& image, Pos pos)
{
	PosComponent x, y;
	tie(x, y) = pos;
	vector<Pos> neighbours;
	neighbours.reserve(8);
	for (PosComponent nx = x-1; nx <= x+1; ++nx)
		for (PosComponent ny = y-1; ny <= y+1; ++ny)
			neighbours.push_back(Pos(nx, ny));
	set<Pos> result;
	copy_if(neighbours.begin(), neighbours.end(), inserter(result, result.begin()), [&image](Pos pos) -> bool
	{
		PosComponent x, y;
		tie(x, y) = pos;
		if (x < 0 || x >= image.cols || y < 0 || y >= image.rows)
			return false;
		return GetPixel(image, x, y)[0] == invalidColor;
	});
	return result;
}

template<class T>
const T& min3(const T& a, const T& b, const T& c)
{
    return std::min(std::min(a, b), c);
}

template<class T>
const T& max3(const T& a, const T& b, const T& c)
{
    return std::max(std::max(a, b), c);
}

Color bgr2hsv(Color bgr)
{
	Channel b = bgr[0];
	Channel g = bgr[1];
	Channel r = bgr[2];
	Channel v = max3(r, g, b);
	Channel s = v == 0 ? 0 : (v - min3(r, g, b))/v;
	Channel h = 0;
	Channel divisor = v - min3(r,g,b);
	if (divisor == 0)
		return Color(0, 0, 0);
	if (v == r)
		h = 60*(g-b)/divisor;
	else if (v == g)
		h = 120 + 60*(b-r)/divisor;
	else if (v == b)
		h = 240 + 60*(r-g)/divisor;
	else
		assert(false); // If we land here our v is incorrect.
	if (h < 0)
		h += 360;
	return Color(h, s, v);
}

double ColorDiff(Color bgr1, Color bgr2)
{
	double db = bgr2[0] - bgr1[0];
	double dg = bgr2[1] - bgr1[1];
	double dr = bgr2[2] - bgr1[2];
	return sqrt(db*db + dg*dg + dr*dr);
}

double ColorPosDiff(const Mat& image, Pos pos, Color color)
{
	PosComponent x, y;
	tie(x, y) = pos;
	double result = 0;
	int colorCount = 0;
	for (PosComponent nx = x-1; nx <= x+1; ++nx)
	{
		for (PosComponent ny = y-1; ny <= y+1; ++ny)
		{
			if (nx < 0 || nx >= image.cols || ny < 0 || ny >= image.rows)
				continue;
			Color pixelColor = GetPixel(image, nx, ny);
			if (pixelColor[0] == invalidColor)
				continue;
			result += ColorDiff(color, pixelColor);
			++colorCount;
		}
	}
	// Avoid division by zero.
	colorCount = std::max(colorCount, 1);
	//return result/colorCount;
	// Square divisor to avoid coral like growing.
	// This also reduces the number of currently open border pixels.
	return result/(colorCount*colorCount);
}



Pos FindBestPos(const Mat& image, set<Pos> nextPositions, Color color)
{
	vector<pair<double, Pos>> ratedPositions;
	ratedPositions.reserve(nextPositions.size());
	transform(nextPositions.begin(), nextPositions.end(), back_inserter(ratedPositions), [&](Pos pos) -> pair<double, Pos>
	{
		return make_pair(ColorPosDiff(image, pos, color), pos);
	});
	random_shuffle(ratedPositions.begin(), ratedPositions.end());
	return min_element(ratedPositions.begin(), ratedPositions.end(),
		[](const pair<double, Pos>& ratedPos1, const pair<double, Pos>& ratedPos2)
	{
		return ratedPos1.first < ratedPos2.first;
	})->second;
}

set<Pos> NonBlackPositions(const Mat& img)
{
	set<Pos> result;
	for (int y = 0; y < img.rows; ++y)
		for (int x = 0; x < img.cols; ++x)
			if (img.at<unsigned char>(y, x) > 0)
				result.insert(Pos(x,y));
	return result;
}

pair<Mat, set<Pos>> Init(int argc, char *argv[])
{
	if (argc > 1)
	{
		Mat src = imread(argv[1], CV_LOAD_IMAGE_GRAYSCALE);
		Mat image = Mat(src.size(), ImageType, Scalar_<Channel>(invalidColor));
		return make_pair(image, NonBlackPositions(src));
	}

	Mat image = Mat(1080, 1920, ImageType, Scalar_<Channel>(invalidColor));
	set<Pos> nextPositions;
	nextPositions.insert(Pos(  image.cols/3, image.rows/2));
	nextPositions.insert(Pos(2*image.cols/3, image.rows/2));
	return make_pair(image, nextPositions);
}



int main(int argc, char *argv[])
{
	vector<Color> colors;
	Channel colValues = 64;
	Channel colMult = 4;
	for(Channel b = 0; b < colValues; ++b)
		for(Channel g = 0; g < 2*colValues; ++ g)
			for(Channel r = 0; r < 2*colValues; ++ r)
				colors.push_back(Color(colMult*b, colMult*g/2, colMult*r/2));

	{
		std::random_device rd;
		std::mt19937 g(rd());
		std::shuffle(colors.begin(), colors.end(), g);
	}

	sort(colors.begin(), colors.end(), [](Color bgr1, Color bgr2) -> bool
	{
		Color hsv1 = bgr2hsv(bgr1);
		Color hsv2 = bgr2hsv(bgr2);
		return hsv1[0] < hsv2[0];
	});

	Mat image;
	set<Pos> nextPositions;
	tie(image, nextPositions) = Init(argc, argv);

	const int saveEveryNFrames = 512;
	const int maxFrames = colors.size();
	const int maxSaves = maxFrames / saveEveryNFrames;
	unsigned long long imgNum = 0;
	while (!colors.empty() && !nextPositions.empty())
	{
		Color color = colors.back();
		colors.pop_back();
		Pos pos = FindBestPos(image, nextPositions, color);
		auto nextPositionsIt = nextPositions.find(pos);
		assert(nextPositionsIt != nextPositions.end());
		nextPositions.erase(nextPositionsIt);
		SetPixel(image, pos.first, pos.second, color);
		set<Pos> newFreePos = GetFreeNeighbours(image, pos);
		nextPositions.insert(newFreePos.begin(), newFreePos.end());
		if (colors.size() % saveEveryNFrames == 0)
		{
			stringstream ss;
			ss << setw(4) << setfill('0') << ++imgNum;

			cout << imgNum << "/" << maxSaves << " " << colors.size() << " " << nextPositions.size() << endl;
			Mat ucharImg;
			image.convertTo(ucharImg, CV_8UC3);

			Mat filtered;
			dilate(ucharImg, filtered, Mat(3, 3, CV_8UC1, Scalar(1)));
			medianBlur(filtered, filtered, 3);

			Mat ts;
			vector<Mat> imageChans(3, Mat());
			split(image, imageChans);
			threshold(imageChans[0], ts, invalidColor, 1, THRESH_BINARY_INV);
			Mat tu;
			ts.convertTo(tu, CV_8UC1);
			Mat tuchar;
			cvtColor(tu, tuchar, CV_GRAY2BGR);

			Mat m;
			multiply(filtered, tuchar, m);

			// fill black gaps, but only with half the median color.
			Mat mixed;
			addWeighted(ucharImg, 1.0, m, 0.5, 0.0, mixed);

			//ffmpeg -r 50 -i output/image%04d.png -vcodec libx264 -preset veryslow -qp 0 output/video.mp4
			imwrite("./output/image" + ss.str() + ".png", mixed);
		}
	}
}