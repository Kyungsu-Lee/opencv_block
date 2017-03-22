#ifndef _DETECTPUZZLE_H_INCLUDED_
#define _DETECTPUZZLE_H_INCLUDED_
#include <jni.h>
#include <android/log.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <vector>
#include <string>

class Puzzle;

extern "C" {

#define LOG_TAG "cpp: "
#define LOGV(...) ((void)__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__))
#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))

typedef struct puzzleResult {
	std::string action;
	std::string direction;
	int number;
} PuzzleResult;

typedef struct puzzleJson {
	bool execute;
	int puzzleLines;
	std::vector<PuzzleResult> detected;
} PuzzleJson;

const std::string actions[4] = {"walk", "jump", "destroy", "walk_test"};
const std::string direction[4] = {"up", "right", "down", "left"};
const std::string numbers[5] = {"1", "2", "3", "4", "5"};

PuzzleJson puzzleDetect(cv::Mat& file);
std::vector<Puzzle> puzzleInit();
}
#endif
