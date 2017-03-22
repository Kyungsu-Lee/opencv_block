#include <jni.h>
#include <android/log.h>
#include <string>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "puzzle.h"
#include "utils.h"
#include "detectPuzzle.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"

using namespace cv;
using namespace std;
using namespace rapidjson;

extern "C" {

#define LOG_TAG "cpp: "
#define LOGV(...) ((void)__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__))
#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))

int process(Mat img_input, Mat &img_result) {
    cvtColor(img_input, img_result, CV_RGBA2GRAY);

    return (0);
}

JNIEXPORT jint JNICALL Java_com_example_young_myapplication_MainActivity_convertNativeLib(JNIEnv *env, jobject instance,
                                                                   jlong matAddrInput,
                                                                   jlong matAddrResult) {

    Mat &img_input = *(Mat *) matAddrInput;
    Mat &img_result = *(Mat *) matAddrResult;

    int conv = process(img_input, img_result);
    int ret = (jint) conv;

    return ret;
}
JNIEXPORT jstring JNICALL
Java_com_example_young_myapplication_MainActivity_puzzleDetect(JNIEnv *env, jobject instance,
                                                               jlong matAddrRgba) {
    Mat& frame = *(Mat*)matAddrRgba;

    /**
     * draw guideline
     */
    Point center(frame.cols / 2, frame.rows / 2);

    double width = frame.cols / 2;
    double height = frame.rows / 2;

    Point br(frame.cols / 4, frame.rows / 5);
    Point tl(frame.cols / 2 + (frame.cols / 4), frame.rows / 5 * 4);

    Rect roi = Rect(tl, br);
    rectangle(frame, roi, Scalar(0, 0, 230), 2);


    bool failed = true;
    string jsonResult = "{ \"result\" : \"failed\" ";
    PuzzleJson result = puzzleDetect(frame);
    jsonResult = Utils().convertToJson(result);
    LOGI("%s", jsonResult.c_str());
    for (int i = 0; i < result.detected.size(); i++) {
        PuzzleResult pResult = result.detected[i];
        if (pResult.action == "" || pResult.direction == "" || pResult.number == -1) {
            failed = true;
        }
        else
            failed = false;
    }
    if (!failed) {
        putText(frame, "success", Point(10, 100), FONT_HERSHEY_SIMPLEX, 2, Scalar(255, 255, 255), 3);
    }

    else if(failed) {
        StringBuffer s;
        PrettyWriter<StringBuffer> writer(s);

        writer.StartObject();
        writer.Key("result");
        writer.String("failed");
        writer.EndObject();

        jsonResult = s.GetString();
        putText(frame, "None", Point(10, 100), FONT_HERSHEY_SIMPLEX, 2, Scalar(255, 255, 255), 3);
    }

    return env->NewStringUTF(jsonResult.c_str());
}

}

