#ifndef _PUZZLE_H_INCLUDED_
#define _PUZZLE_H_INCLUDED_
#include <jni.h>
#include <opencv2/core/mat.hpp>

class Puzzle {
private:
	cv::Mat element;
public:
	Puzzle() { }

	Puzzle(cv::Mat element) { this->element = element; }

	void setElement(cv::Mat element) { this->element = element; }

	cv::Mat getElement() { return element; }
};
#endif
