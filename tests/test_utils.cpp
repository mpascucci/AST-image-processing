#include <test_utils.hpp>

using namespace std;

const cv::Mat addRectangleToImage(cv::Mat img, cv::Rect rectangle)
{
    /* @Brief draws rectangle on a copy of img */
    cv::Mat out = img.clone();
    cv::Scalar color(std::rand() & 255, std::rand() & 255, std::rand() & 255);
    cv::rectangle(out, rectangle, color, 20);
    return out;
}

#ifdef USE_CVV
void display(cv::Mat img, const string title)
{
    assert(!img.empty());
    cvv::showImage(img, CVVISUAL_LOCATION, title);
}
#else
void display(cv::Mat img, const string title)
{
    assert(!img.empty());
    cv::imshow(title, img);
    cv::waitKey(0);
}
#endif //USE_CVV