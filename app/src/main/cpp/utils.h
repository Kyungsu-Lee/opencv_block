#ifndef _UTILS_H_INCLUDED_
#define _UTILS_H_INCLUDED_
#include <jni.h>
#include <android/log.h>
#include "detectPuzzle.h"

using namespace std;

extern "C" {
#define LOG_TAG "cpp: "
#define LOGV(...) ((void)__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__))
#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))

class Utils {
private:
	int coordi_maxY(std::vector<cv::Vec4i> yCoordinates, int upper);

	cv::Mat transform(cv::Mat &frame, std::vector<cv::Point> roi);

	std::vector<float> coordi_meanStddev(std::vector<cv::Vec4i> yCoordinates);

	std::vector<int> getHorizontalBorder(cv::Mat &frame, cv::Mat &canny);

	std::vector<cv::Point> findSquarePoints(cv::Mat &frame, double threshold1, double threshold2,
											cv::Size elementSize, int findContoursMode,
											double epsilonRatio, bool isSquare,
											int minContourArea, int debug);

public:
	Utils() { }

	int largest(int x, int y, int z);

	std::string convertToJson(PuzzleJson result);

	std::vector<cv::Point> getPuzzlePoints(cv::Mat &frame);

	cv::Mat getPuzzle(cv::Mat &frame);

	std::vector<Puzzle> getPuzzles(cv::Mat &puzzle);

	std::vector<vector<Puzzle>> getBlocks(std::vector<Puzzle> puzzles);

	std::vector<vector<Puzzle>> getSymbols(std::vector<std::vector<Puzzle>> blocks);

	cv::Mat getNumber(cv::Mat &frame);
};
}
#endif