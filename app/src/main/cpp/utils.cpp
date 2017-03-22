#include <jni.h>
#include <opencv2/opencv.hpp>
#include "detectPuzzle.h"
#include "puzzle.h"
#include "utils.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"

using namespace std;
using namespace cv;
using namespace rapidjson;

template <typename T>
std::string to_string(T value)
{
	std::ostringstream os ;
	os << value ;
	return os.str() ;
}

struct compare {
	bool operator()(cv::Point a, cv::Point b) {
		return a.x + a.y < b.x + b.y;
	}

	bool operator()(cv::Point2f a, cv::Point2f b) {
		return a.x + a.y < b.x + b.y;
	}

	bool operator()(cv::Vec4i a, cv::Vec4i b) {
		return a[1] < b[1];
	}

	bool operator()(cv::RotatedRect a, cv::RotatedRect b) {
		int i = a.size.area();
		int j = b.size.area();
		return (i > j);
	}
} comparePoint;

int Utils::coordi_maxY(std::vector<cv::Vec4i> yCoordinates, int upper) {
	int temp = 0;
	int maxValue;

	for (int i = 0; i < yCoordinates.size(); i++) {
		maxValue = max(temp, yCoordinates[i][1]);
	}

	return maxValue + upper;
}

cv::Mat Utils::transform(cv::Mat &frame, std::vector<cv::Point> roi) {
	Mat before = frame.clone();
	Mat after = frame.clone();

	circle(before, roi[0], 1, Scalar(0, 0, 0), 2);
	circle(before, roi[1], 1, Scalar(255, 0, 0), 2);
	circle(before, roi[2], 1, Scalar(0, 255, 0), 2);
	circle(before, roi[3], 1, Scalar(0, 0, 255), 2);

	sort(roi.begin(), roi.end(), comparePoint);

	/*int max_y = max(roi[0].y, roi[2].y);
	roi[0].y = max_y;
	roi[2].y = max_y;
	int max_x = max(roi[2].x, roi[3].x);
	roi[2].x = max_x;
	roi[3].x = max_x;*/

	circle(after, roi[0], 1, Scalar(0, 0, 0), 2);
	circle(after, roi[1], 1, Scalar(255, 0, 0), 2);
	circle(after, roi[2], 1, Scalar(0, 255, 0), 2);
	circle(after, roi[3], 1, Scalar(0, 0, 255), 2);

	RotatedRect box = minAreaRect(Mat(roi));
	//rectangle(after, Rect(box.boundingRect()), Scalar(1, 1, 1), 1);

	Point2f inputPoint[4]; //upper-left, upper-right, lower-left, lower-right
	Point2f outputPoint[4];

	inputPoint[0] = roi[0];
	inputPoint[1] = roi[2];
	inputPoint[2] = roi[1];
	inputPoint[3] = roi[3];

	//wrap perspective for square.
	double widthA = sqrt(pow(roi[0].x - roi[2].x, 2) + pow(roi[0].y - roi[2].y, 2));
	double widthB = sqrt(pow(roi[1].x - roi[3].x, 2) + pow(roi[1].y - roi[3].y, 2));

	double heightA = sqrt(pow(roi[0].x - roi[1].x, 2) + pow(roi[0].y - roi[1].y, 2));
	double heightB = sqrt(pow(roi[2].x - roi[3].x, 2) + pow(roi[2].y - roi[3].y, 2));

	double maxWidth = max(widthA, widthB);
	double maxHeight = max(heightA, heightB);

	outputPoint[0] = Point2f(0, 0);
	outputPoint[1] = Point2f(maxWidth, 0);
	outputPoint[2] = Point2f(0, maxHeight);
	outputPoint[3] = Point2f(maxWidth, maxHeight);

	Mat M = getPerspectiveTransform(inputPoint, outputPoint);
	Mat result;

	warpPerspective(frame, result, M, Size(maxWidth, maxHeight));
	return result;
}

vector<float> Utils::coordi_meanStddev(vector<Vec4i> yCoordinates) {
	//get mean
	vector<float> meanStddev;

	float sum = 0.0;
	float mean;
	float n = 0.0;

	for (int i = 0; i < yCoordinates.size(); i++) {
		sum += yCoordinates[i][1];
	}

	mean = sum / (float) yCoordinates.size();
	meanStddev.push_back(mean);

	for (int i = 0; i < yCoordinates.size(); i++) {
		n += pow(yCoordinates[i][1] - mean, 2);
	}

	meanStddev.push_back(sqrt(n / (float) yCoordinates.size()));

	return meanStddev;
}

vector<int> Utils::getHorizontalBorder(Mat &frame, Mat &canny) {
	Mat tempResult = frame.clone();

	vector<Vec4i> lines;
	vector<Vec4i> linesWithSimilarY;
	vector<int> yCoordinates;

	HoughLinesP(canny, lines, 1, CV_PI / 180, 80, 30, 10);
	sort(lines.begin(), lines.end(), comparePoint); //sort by y coordinate value

	for (int i = 0; i < lines.size(); i++) {
		Vec4i l = lines[i];
		Point p1 = Point(l[0], l[1]);
		Point p2 = Point(l[2], l[3]);

		float x = lines[i][0], y = lines[i][1];
		float angle = atan2(p1.y - p2.y, p1.x - p2.x);
		float degree = angle * 180 / CV_PI;

		if (y <= canny.rows - 40 && abs(degree) > 150) {
			linesWithSimilarY.push_back(lines[i]);
			vector<float> mean_stddev = coordi_meanStddev(linesWithSimilarY);

			if (mean_stddev[1] > 3) {
				linesWithSimilarY.pop_back();
				vector<float> mean_stddev = coordi_meanStddev(linesWithSimilarY);

				if (lines[i - 1][1] < 40) {
					yCoordinates.push_back(coordi_maxY(linesWithSimilarY, 3));
				}
				else
					yCoordinates.push_back(cvRound(mean_stddev[0]));
				linesWithSimilarY.clear();
			}
			//line(tempResult, Point(lines[i][0], lines[i][1]), Point(lines[i][2], lines[i][3]), Scalar(0, 0, 255), 1, 8);
		}
	}

	if (!linesWithSimilarY.empty()) {
		vector<float> mean_stddev = coordi_meanStddev(linesWithSimilarY);
		yCoordinates.push_back(mean_stddev[0]);
	}

	//for (int i = 0; i < yCoordinates.size(); i++) {
	//	line(tempResult, Point(0, yCoordinates[i]), Point(tempResult.cols, yCoordinates[i]), Scalar(255, 255, 255), 1, 8);
	//}

	return yCoordinates;
}

vector<Point> Utils::findSquarePoints(cv::Mat &frame, double threshold1, double threshold2,
									  cv::Size elementSize, int findContoursMode,
									  double epsilonRatio, bool isSquare,
									  int minContourArea, int debug = 0) {
	Mat canny;

	Canny(frame, canny, threshold1, threshold2);
	morphologyEx(canny, canny, MORPH_CLOSE, getStructuringElement(MORPH_RECT, elementSize));

	//g_debugFrame = canny.clone();

	vector<vector<Point>> contours;
	vector<Vec4i> hierarchy;
	vector<Point> approx;
	vector<RotatedRect> rects;

	findContours(canny, contours, hierarchy, findContoursMode, CV_CHAIN_APPROX_SIMPLE);

	for (int i = 0; i < contours.size(); i++) {
		//used 'approxPolyDP' to find polygon(square). It only shows 4 points for square.
		rects.push_back(minAreaRect(contours[i]));
		//approxPolyDP(contours[i], approx, arcLength(contours[i], true) * epsilonRatio, true);
		//if (approx.size() >= 4 && contourArea(Mat(approx)) > minContourArea) {
		//	//rects.push_back(minAreaRect(approx));
		//}
		//drawContours(frame, contours, i, Scalar(255, 255, 255), 1, 8);
	}

	sort(rects.begin(), rects.end(), comparePoint);
	/*if (rects.size() > 1 || rects.size() < 1) {
		return vector<Point>();
	}*/
	if(rects.empty())
		return vector<Point>();

	Point2f rect_points[4];
	rects[0].points(rect_points);
	for (int x = 0; x < 4; x++) {
		//line(frame, Point(rect_points[x % 4]), Point(rect_points[(x + 1) % 4]), Scalar(0, 0, 255), 1);
	}
	vector<Point> square;
	square.push_back(rect_points[0]);
	square.push_back(rect_points[1]);
	square.push_back(rect_points[2]);
	square.push_back(rect_points[3]);

	return square;
}

int Utils::largest(int x, int y, int z) {
	if (x > y && x > z)
		return 0;
	else if (y > x && y > z)
		return 1;
	else if (z > x && z > y)
		return 2;
	else
		return -1;
}

string Utils::convertToJson(PuzzleJson result) {
	StringBuffer s;
	PrettyWriter<StringBuffer> writer(s);

	writer.StartObject();

	writer.Key("execute");
	writer.Bool(result.execute);

	writer.Key("puzzleLines");
	writer.Int(result.puzzleLines);

	writer.Key("puzzle");
	writer.StartArray();
	for (int i = 0; i < result.detected.size(); i++) {
		PuzzleResult detected = result.detected[i];
		writer.StartObject();

		writer.Key("action");
		writer.String((detected.action).c_str());
		writer.Key("direction");
		writer.String((detected.direction).c_str());
		writer.Key("repetition");
		writer.Int(detected.number);

		writer.EndObject();
	}
	writer.EndArray();
	writer.EndObject();

	return s.GetString();
}

vector<Point> Utils::getPuzzlePoints(Mat &frame) {
	vector<Point> square;
	square = findSquarePoints(frame, 40, 70, Size(11, 11), CV_RETR_EXTERNAL, 0.02, false, 9000);
	return square;
}

Mat Utils::getPuzzle(Mat &frame) {
	vector<Point> puzzlePoint = getPuzzlePoints(frame);

	Mat puzzle;
	if (puzzlePoint.empty()) {
		return Mat();
	}
	else
		puzzle = transform(frame, puzzlePoint);
	return puzzle;
}

vector<Puzzle> Utils::getPuzzles(Mat &puzzle) {
	vector<Puzzle> puzzles;
	Mat canny;
	Canny(puzzle, canny, 30, 100);
	morphologyEx(canny, canny, MORPH_CLOSE, getStructuringElement(MORPH_RECT, Size(3, 3)));

	vector<int> yCoordinates = getHorizontalBorder(puzzle, canny);

	if (yCoordinates.empty()) {
		puzzles.push_back(Puzzle(puzzle));
	}
	else {
		for (int i = 0; i < yCoordinates.size(); i++) {
			Rect roi;

			if (i == 0)
				roi = Rect(0, 0, puzzle.cols, yCoordinates[i]);
			else
				roi = Rect(0, yCoordinates[i - 1], puzzle.cols,
						   yCoordinates[i] - yCoordinates[i - 1]);

			if (i == yCoordinates.size() - 1) {
				if (roi.area() > 7000) //7000: minimum area of puzzle
					puzzles.push_back(Puzzle(puzzle(roi)));
				roi = Rect(0, yCoordinates[yCoordinates.size() - 1],
						   puzzle.cols, puzzle.rows - yCoordinates[yCoordinates.size() - 1]);
			}

			if (roi.area() > 7000) //7000: minimum area of puzzle
				puzzles.push_back(Puzzle(puzzle(roi)));
		}
	}
	cout << "how many puzzles: " << puzzles.size() << endl;
	if(puzzles.size() > 3)
		return vector<Puzzle>();
	return puzzles;
}

vector<vector<Puzzle>> Utils::getBlocks(vector<Puzzle> puzzles) {
	vector<vector<Puzzle>> blocks;

	//* WARNING: Border is fixed value
	//*/
	for (int i = 0; i < puzzles.size(); i++) {
		Mat canny;
		Mat puzzleElement = puzzles[i].getElement();
		Canny(puzzleElement, canny, 50, 150);
		//blocks[i] = canny.clone();

		float firstBorder = puzzleElement.cols * 1.2 / 3;
		float rest = puzzleElement.cols - (puzzleElement.cols * 1.2 / 3);
		float secondBorder = firstBorder + rest / 2;

		Rect first = Rect(0, 0, firstBorder, puzzleElement.rows);
		Rect second = Rect(firstBorder, 0, secondBorder - firstBorder, puzzleElement.rows);
		Rect thrid = Rect(secondBorder, 0, puzzleElement.cols - secondBorder, puzzleElement.rows);

		Mat firstBlock = puzzleElement(first);
		Mat secondBlock = puzzleElement(second);
		Mat thirdBlock = puzzleElement(thrid);
		if(firstBlock.empty() || secondBlock.empty() || thirdBlock.empty())
			return vector<vector<Puzzle>>();

		vector<Puzzle> block;
		block.push_back(firstBlock);
		block.push_back(secondBlock);
		block.push_back(thirdBlock);

		blocks.push_back(block);
	}
	return blocks;
}

std::vector<vector<Puzzle>> Utils::getSymbols(std::vector<std::vector<Puzzle>> blocks) {
	vector<vector<Puzzle>> symbols;

	for (int i = 0; i < blocks.size(); i++) {
		vector<Puzzle> symbol;
		for (int j = 0; j < blocks[i].size(); j++) {
			Puzzle temp;
			Mat blockRoi = blocks[i][j].getElement();
			Mat grayBlockRoi;
			cvtColor(blockRoi, grayBlockRoi, COLOR_BGR2GRAY);

			if (j == 0 || j == 2) {
				LOGD("FIRST AND THRID SYMBOL");
				temp = (Puzzle(blockRoi(Rect(0, 0, blocks[i][j].getElement().cols,
											 blocks[i][j].getElement().rows))));
			}
			else {
				LOGD("SECOND SYMBOL");
				////vector<vector<Point>> symbols = findSquarePoints(blockRoi, 10, 100, Size(1, 1), CV_RETR_CCOMP, 0.02, false, 20);
				vector<vector<Point>> symbolPoints;
				Mat canny;
				Canny(blocks[i][j].getElement(), canny, 40, 70);
				morphologyEx(canny, canny, MORPH_CLOSE,
							 getStructuringElement(MORPH_RECT, Size(2, 2)));

				vector<vector<Point>> contours;
				vector<Vec4i> hierarchy;
				vector<Point> approx;

				findContours(canny, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);
				for (int x = 0; x < contours.size(); x++) {
					approxPolyDP(contours[x], approx, arcLength(contours[x], true) * 0.04, true);
					if (approx.size() >= 4 && contourArea(Mat(approx)) > 50) {
						symbolPoints.push_back(approx);
						//drawContours(blockRoi, contours, x, Scalar(0, 255, 255), 1, 8);
					}
				}

				vector<Rect> candidates;
				Rect recommended;

				for (int y = 0; y < symbolPoints.size(); y++) {
					Point *p1 = &symbolPoints[y][0];
					vector<Point> symbol = symbolPoints[y];
					int n1 = (int) symbolPoints[y].size();

					RotatedRect symbolRoi = minAreaRect(Mat(symbol));
					Point2f rect_points[4];
					symbolRoi.points(rect_points);

					Rect roi(symbolRoi.boundingRect());
					//rectangle(blockRoi, roi, Scalar(255, 255, 255), 1);
					if (roi.br().x > blockRoi.size().width ||
						roi.br().y > blockRoi.size().height
						|| roi.tl().x < 0 || roi.tl().y < 0){
						temp = Puzzle(blockRoi);
						break;
					}

					Point2f center(blockRoi.cols / 2, blockRoi.rows / 2);
					Rect center_rect(center.x - 2.0, center.y - 2.0, 4, 4);
					//rectangle(blockRoi, Rect(15, 8, 28, 56), Scalar(255, 0, 0), 2);

					if (roi.x >= 0 && roi.y >= 0) {
						candidates.push_back(roi);
						int overlapped_area_size = (center_rect & roi).area();
						if (overlapped_area_size > 0 && roi.width < blockRoi.rows &&
							roi.height < blockRoi.cols) {
							if (recommended.area() <= 0)
								recommended = roi;
							else
								recommended = recommended | roi;
							candidates.pop_back();
						}
					}
					//circle(blockPieces[i][j], center, 1, Scalar(20, 10, 0), 1);
				}

				//for (int x = 0; x < candidates.size(); x++) {
				//	//rectangle(blockRoi, candidates[x], Scalar(255, 0, 0), 1);
				//	int size = (recommended & candidates[x]).area();
				//	if (size > 0) {
				//		//cout << blockRoi.rows << " " << blockRoi.cols << " " << i << j << " " << candidates[x].x << " " << candidates[x].y << " " << candidates[x].width << " " << candidates[x].height << endl;
				//		if (candidates[x].width < blockRoi.rows && candidates[x].height < blockRoi.cols)
				//			recommended = recommended | candidates[x];
				//	}
				//}

				if (recommended.area() > 0) {
					temp = Puzzle(blockRoi(recommended));
				}
			}
			symbol.push_back(temp);
		} //end of j
		symbols.push_back(symbol);
	} //end of i
	if(symbols.size() * 3 > blocks.size() * 3 || symbols.size() * 3 < blocks.size() * 3)
		return vector<vector<Puzzle>>();
	return symbols;
}

Mat Utils::getNumber(Mat &frame) {
	Mat gray;
	Mat thres;
	//Mat cropped_frame = frame(Rect(5, 5, frame.cols - 10, frame.rows - 10));
	Mat cropped_frame = frame;

	cvtColor(cropped_frame, gray, COLOR_BGR2GRAY);

	threshold(gray, thres, 140, 255, THRESH_BINARY | THRESH_OTSU);
	vector<vector<Point>> contours;
	vector<Vec4i> hierarchy;
	vector<Point> approx;
	vector<RotatedRect> rects;
	vector<Moments> mu;

	LOGD("______GET CONTOURS");
	findContours(thres, contours, hierarchy, RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));
	for (int x = 0; x < contours.size(); x++) {
		approxPolyDP(Mat(contours[x]), approx, 3, true);
		if (contourArea(Mat(approx)) > 80 && approx.size() >= 4) {
			mu.push_back(moments(contours[x], true));
			rects.push_back(minAreaRect(approx));
		}

		//drawContours(cropped_frame, contours, x, Scalar(255, 255, 255), 1, 8);
	}

	vector<Point2f> mc;
	for (int x = 0; x < mu.size(); x++) {
		mc.push_back(Point2f(mu[x].m10 / mu[x].m00, mu[x].m01 / mu[x].m00));
	}

	Point2f center(cropped_frame.cols / 2, cropped_frame.rows / 2);
	double current_distance = 99999;
	int final_index = 0;

	for (int x = 0; x < mc.size(); x++) {
		double distance = sqrt(pow(mc[x].x - center.x, 2) + pow(mc[x].y - center.y, 2));
		if (distance < current_distance) {
			current_distance = distance;
			final_index = x;
		}
	}

	//sort(rects.begin(), rects.end(), comparePoint);
	if(!rects.empty()) {
		LOGD("______GET ROI");
		Point2f rect_points[4];
		rects[final_index].points(rect_points);

		int maxWidth = cropped_frame.size().width;
		int maxHeight = cropped_frame.size().height;

		Rect roi2(rects[0].boundingRect());
		Point br = roi2.br();
		Point tl = roi2.tl();

		tl.x -= 6;
		tl.y -= 6;
		br.x += 6;
		br.y += 6;

		Rect new_roi(tl, br);
		if (new_roi.br().x > cropped_frame.size().width ||
			new_roi.br().y > cropped_frame.size().height
			|| new_roi.tl().x < 0 || new_roi.tl().y < 0) {
			for (int y = 0; y < 4; y++) {
				return Mat();
				/*if (rect_points[y].x < 0)
					rect_points[y].x = abs(rect_points[y].x);
				if (rect_points[y].y < 0)
					rect_points[y].y = abs(rect_points[y].y);
				if (rect_points[y].x > roi2.width)
					rect_points[y].x = roi2.width - rect_points[y].x;
				if (rect_points[y].y > roi2.height)
					rect_points[y].y = roi2.height - rect_points[y].y;

				new_roi = Rect(rects[0].boundingRect());*/
			}
		}
		//rectangle(cropped_frame, new_roi, Scalar(255, 255, 255), 1, 8, 0);
		Mat numbersRoi = cropped_frame(new_roi);

		Mat gray2;
		cvtColor(numbersRoi, gray2, COLOR_BGR2GRAY);

		Mat thres2;
		threshold(gray2, thres2, 140, 255, THRESH_BINARY | THRESH_OTSU);

		return thres2;
	}
	return Mat();
}