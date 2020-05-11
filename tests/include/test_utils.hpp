#include <opencv2/opencv.hpp>
#include <string>

using namespace std;

#ifndef PROD_MODE
#ifdef USE_CVV
#include <opencv2/cvv/debug_mode.hpp>
#include <opencv2/cvv/dmatch.hpp>
#include <opencv2/cvv/filter.hpp>
#include <opencv2/cvv/final_show.hpp>
#include <opencv2/cvv/show_image.hpp>
#endif  // USE_CVV
#endif  // PROD_MODE

const cv::Mat addRectangleToImage(cv::Mat img, cv::Rect rectangle);
void display(cv::Mat img, const string title = "display");