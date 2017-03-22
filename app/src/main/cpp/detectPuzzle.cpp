#include <jni.h>
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
#include <time.h>
#include "detectPuzzle.h"
#include "puzzle.h"
#include "utils.h"

using namespace std;
using namespace cv;

int h = 0;

std::vector<Puzzle> puzzleInit() {
	std::vector<Puzzle> savedNumbers;

	for (int i = 0; i < 5; i++) {
		cv::Mat r;
		std::string path = "/sdcard/Documents/data/";
		std::string filename = numbers[i] + ".yaml";
		cv::FileStorage fs(path + filename, cv::FileStorage::READ);
		read(fs["image"], r);
		savedNumbers.push_back(Puzzle(r));
        fs.release();
	}
	return savedNumbers;
}

PuzzleJson puzzleDetect(cv::Mat& file) {
	PuzzleJson final_result;

	final_result.execute = false;
	Mat frame = file;
	if(frame.empty())
		return final_result;

	cvtColor(frame, frame, COLOR_BGRA2BGR);

	Mat hsvFrame;

	//flip a webcam frame (upside down)
	transpose(frame, frame);
	flip(frame, frame, -1);
	//VGA
	//Rect frame_roi(51, 143, abs(452 - 51), abs(143 - 479));
	//HR
	//Rect frame_roi(162, 462, abs(162 - 615), abs(462 - 681));
	resize(frame, frame, Size(720, 1280));
	Point center(frame.cols / 2, frame.rows / 2);

	double width = frame.cols / 2;
	double height = frame.rows / 2;

	//Rect roi = Rect(center.x - width / 1.5, center.y - height / 2.25, width * 1.25, height / 1.15);
	Point br(frame.cols / 4, frame.rows / 5);
	Point tl(frame.cols / 2 + (frame.cols / 4), frame.rows / 5 * 4);

	Rect roi = Rect(tl, br);

	if(h < 1){
		std::ostringstream os ;
		os << h ;
		os.str();
		bool hi = imwrite("/sdcard/Documents/" + os.str() + "test.jpg", frame);
		h++;
	}

	cvtColor(frame, hsvFrame, COLOR_BGR2HSV);

	frame = frame(roi);

	/*assume that only one big square is found
	* ##problem: what if there are more than two big squares?
	*/
	//for (size_t i = 0; i < squares.size(); i++) {
	int64 t0 = cv::getTickCount();
	vector<Puzzle> savedBlocks;
	vector<Puzzle> savedNumbers;

	savedNumbers = puzzleInit();

	LOGD("GET Puzzle");
	Mat puzzleTransformed = Utils().getPuzzle(frame);
	if (puzzleTransformed.empty()) {
		final_result.puzzleLines = 0;
		return final_result;
	}

	if (!puzzleTransformed.empty()) {
		vector<Puzzle> puzzles = Utils().getPuzzles(puzzleTransformed);
        if(puzzles.empty())
            return final_result;

        final_result.puzzleLines = puzzles.size();

		LOGD("GET BLOCK");
		vector<vector<Puzzle>> blocks = Utils().getBlocks(puzzles);
		if(blocks.empty())
			return final_result;

		LOGD("GET SYMBOLS");
		vector<vector<Puzzle>> symbols = Utils().getSymbols(blocks);
		if (symbols.empty())
			return final_result;

		LOGD("......ANAYLIZING SYMBOLS");
		srand((unsigned int) time(NULL));
		for (int i = 0; i < symbols.size(); i++) {
			PuzzleResult detected;
			for (int j = 0; j < symbols[i].size(); j++) {
				Mat symbolRoi = symbols[i][j].getElement();
				if (!symbolRoi.empty()) {
					LOGD("__FIRST SYMBOL");
					if (j == 0) {
						int symbolRoi_width = symbolRoi.cols;
						int symbolRoi_height = symbolRoi.rows;
						LOGE("width: %d  Height: %d", symbolRoi_width, symbolRoi_height);
						if (symbolRoi_width > 70 && symbolRoi_height > 70) {
							//WARNING: fixed value
							Point br = Point(symbolRoi_width / 4, symbolRoi_height / 2.25);
							Point tl = Point(10, 10);
							Point diff = br - tl;
							Rect roi = Rect(tl, br);
							int forR = 0, forB = 0, forY = 0;

							//rectangle(symbolRoi, roi, Scalar(255, 255, 255), 1);
							for (int n = 0; n < 100; n++) {
								int x = rand() % diff.x;
								int y = rand() % diff.y;

								Vec3b *ptr = symbolRoi.ptr<Vec3b>(y);
								Vec3b pixel = ptr[x];

								int r = pixel[0];
								int g = pixel[1];
								int b = pixel[2];
								if (r > g && r > b)
									forR++;
								else if (b > r && b > g)
									forB++;
								else if (b < r && b < g)
									forY++;
							}
							int result = Utils().largest(forR, forB, forY);
							if (result == -1) {
								LOGD("Action: Not Detected!!!");
								cout << "Ation: Not Detected!!!" << endl;
								detected.action = "None";
							}
							else {
								LOGD("Action: %d", result + 1);
								cout << "Action: " << actions[result] << endl;
								detected.action = actions[result];
							}

						}
					}
					LOGD("__SECOND SYMBOL");
					if (j == 1) {
						Mat grayTest;
						Mat thres;

						cvtColor(symbolRoi, grayTest, COLOR_BGR2GRAY);
						threshold(grayTest, thres, 50, 255, THRESH_BINARY | THRESH_OTSU);

						Point center(thres.cols / 2, thres.rows / 2);

						vector<Puzzle> seperated;
						seperated.push_back(
								thres(Rect(0, 0, symbolRoi.cols / 2, symbolRoi.rows / 2)));
						seperated.push_back(
								thres(Rect(thres.cols / 2, 0, thres.cols / 2, thres.rows / 2)));
						seperated.push_back(
								thres(Rect(thres.cols / 2, thres.rows / 2, thres.cols / 2,
										   thres.rows / 2)));
						seperated.push_back(
								thres(Rect(0, thres.rows / 2, thres.cols / 2, thres.rows / 2)));

						int result = -1;
						int max_nonzero = -1;
						for (int x = 0; x < seperated.size(); x++) {
							int n1 = countNonZero(seperated[x % 4].getElement());
							int n2 = countNonZero(seperated[(x + 1) % 4].getElement());
							if (n1 + n2 > max_nonzero) {
								max_nonzero = n1 + n2;
								result = x;
							}
						}
						if (result == -1) {
							LOGD("Direction: Not detected!!!");
							cout << "Direction: Not detected!!!" << endl;
							detected.direction = "None";
						}
						else {
							LOGD("Direction: %d", result + 1);
							cout << "Direction: " << direction[result] << endl;
							detected.direction = direction[result];
						}
					}
					LOGD("__THIRD SYMBOL");
					if (j == 2) {
						LOGD("____GET NUMBER");
						Mat target = Utils().getNumber(symbolRoi);
						LOGD("____FINISH NUMBER");
						if (target.empty()) {
							detected.number = -1;
							continue;
						}

						double prev_maxPercent = -1;
						double maxPercent = -1;
						int result = -1;
						bool broken = false;

						for (int x = 0; x < savedNumbers.size(); x++) {
							Mat savedNumber = savedNumbers[x].getElement();

							resize(target, target, savedNumber.size());

							Mat xor_result;
							bitwise_xor(savedNumber, target, xor_result);
							threshold(xor_result, xor_result, 100, 255, THRESH_BINARY);
							//morphologyEx(xor_result, xor_result, MORPH_OPEN, getStructuringElement(MORPH_RECT, Size(3, 3)));
							//morphologyEx(xor_result, xor_result, MORPH_CLOSE, getStructuringElement(MORPH_RECT, Size(2, 2)));

							int sampleCount = countNonZero(savedNumber);
							int xor_resultCount = countNonZero(xor_result);

							double percentage =
									((double) sampleCount / (double) xor_resultCount) * 100;
							/*if (x == 2) {
							cout << xor_result.size() << endl;
							cout << xor_result << endl;
							cout << savedNumber.size() << endl;
							cout << savedNumber << endl;
							}*/
							cout << x + 1 << " sample: " << sampleCount << " xor_result count: " <<
							xor_resultCount << endl;
							cout << "Result: " << percentage << endl << endl;
							maxPercent = max(maxPercent, percentage);
							if (maxPercent != prev_maxPercent) {
								if (maxPercent - prev_maxPercent < 20) {
									broken = true;
									break;
								}
								prev_maxPercent = maxPercent;
								result = x;
							}
						}

						if (broken) {
							cout << "broken" << endl;
							Mat divided[2];
							divided[0] = target(Rect(0, 0, target.cols, target.rows / 2));
							flip(divided[0], divided[0], 0);
							divided[1] = target(
									Rect(0, target.rows / 2, target.cols, target.rows / 2));

							Mat xor_result;
							bitwise_xor(divided[0], divided[1], xor_result);
						}
						if (result == -1) {
							LOGD("Number: Not detected!!!");
							cout << "Number: Not detected!!!" << endl;
							detected.number = -1;
						}
						else {
							LOGD("Number %d", result + 1);
							cout << "Number: " << numbers[result] << endl;
							detected.number = result + 1;
						}
					}
				}//symbolRoi emtpy()
			} //end of blocks j
			final_result.detected.push_back(detected);
		} //end of blocks i
	}

	LOGD("FINISH!!!!!!");
	int64 t1 = cv::getTickCount();
	double secs = (t1 - t0) / cv::getTickFrequency();
	std::cout << "time: " << secs << endl;

	return final_result;
}