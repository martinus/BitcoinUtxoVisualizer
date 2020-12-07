#include <doctest.h>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

// renders some text
TEST_CASE("opencv_test" * doctest::skip()) {
    cv::Mat image = cv::Mat::zeros(480, 1200, CV_8UC3);
    auto org = cv::Point(10, 100);
    auto color = cv::Scalar(80, 150, 255);

    // big text
    cv::putText(image, "Hello world from BitcoinUtxoVisualizer!", org, cv::FONT_HERSHEY_SIMPLEX, 2, color, 2, cv::LINE_AA);

    // normal text
    cv::putText(image,
                "normal text!",
                org + cv::Point(0, 40),
                cv::FONT_HERSHEY_SIMPLEX,
                0.7,
                color,
                1,
                cv::LINE_AA);

    // small text
    cv::putText(image,
                "small text!",
                org + cv::Point(0, 80),
                cv::FONT_HERSHEY_SIMPLEX,
                0.4,
                color,
                1,
                cv::LINE_AA);

    cv::imshow("opencv_test", image);
    cv::waitKey();
}
