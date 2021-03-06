#include <iostream> // for standard I/O
#include <string>   // for strings
#include <iomanip>  // for controlling float print precision
#include <sstream>  // string to number conversion
#include <opencv2/core.hpp>     // Basic OpenCV structures (cv::Mat, Scalar)
#include <opencv2/imgproc.hpp>  // Gaussian Blur
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>  // OpenCV window I/O
#include "opencv2/video/tracking.hpp"

using namespace std;
using namespace cv;


static void drawHsv(const Mat& flow, Mat& bgr) {
	//extract x and y channels
	Mat xy[2]; //X,Y
	split(flow, xy);

	//calculate angle and magnitude
	Mat magnitude, angle, hsv;
	cartToPolar(xy[0], xy[1], magnitude, angle, true);

	//translate magnitude to range [0;1]
	double mag_max;
	minMaxLoc(magnitude, 0, &mag_max);
	magnitude.convertTo(
		magnitude,    // output matrix
		-1,           // type of the ouput matrix, if negative same type as input matrix
		1.0 / mag_max // scaling factor
	);


	//build hsv image
	Mat _hsv[3];
	_hsv[0] = angle;
	_hsv[1] = magnitude;
	_hsv[2] = cv::Mat::ones(angle.size(), CV_32F);

	merge(_hsv, 3, hsv);
	//convert to BGR and show
	cvtColor(hsv, bgr, COLOR_HSV2BGR);
}


static void drawOptFlowMap(const Mat& flow, Mat& cflowmap, double scale, int step, const Scalar& color)
{
	for (int y = 0; y < cflowmap.rows; y += step)
		for (int x = 0; x < cflowmap.cols; x += step)
		{
			const Point2f& fxy = flow.at<Point2f>(y, x) * scale;
			line(cflowmap, Point(x, y), Point(cvRound(x + fxy.x), cvRound(y + fxy.y)),
				color);
			circle(cflowmap, Point(x, y), 2, color, -1);
		}
}

int main(int argc, char** argv)
{
	VideoCapture cap(0);
	if (!cap.isOpened())
	{
		cout << "Could not open reference " << endl;
		return -1;
	}
	Mat flow, cflow, frame, gray, prevgray, img_bgr, img_hsv;
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy; 

	namedWindow("flow", 1);

	for (;;)
	{
		cap >> frame;
		cvtColor(frame, gray, COLOR_BGR2GRAY);

		if (!prevgray.empty())
		{
			calcOpticalFlowFarneback(prevgray, gray, flow, 0.5, 5, 16, 3, 5, 1.2, OPTFLOW_FARNEBACK_GAUSSIAN);
			// calculate dense optical flow
			/*calcOpticalFlowFarneback(
				prevgray,
				gray,
				flow, // computed flow image that has the same size as prev and type CV_32FC2
				0.5,  // image scale: < 1 to build pyramids for each image. 0.5 means a
					  // classical pyramid, where each next layer is twice smalller than the
					  // previous one
				5,    // number of pyramid layers
				15,   // averaging windows size. larger values increase the algorithm robustness
					  // to image noise and give more chances for fast motion detection, but
					  // yields more blurred motion field
				3,    // number of iterations for each pyramid level
				5,    // size of the pixel neighborhood used to find the polynomial expansion
					  // in each pixel
				1.1,  // standard deviation of the Gaussian used to smooth derivations
				OPTFLOW_FARNEBACK_GAUSSIAN     // flags
			);*/

			cvtColor(prevgray, cflow, COLOR_GRAY2BGR);
			drawOptFlowMap(flow, cflow, 1.5, 16, CV_RGB(0, 255, 0));
			imshow("flow", cflow);
			drawHsv(flow, img_bgr);
			Mat gray_bgr = Mat::zeros(frame.rows, frame.cols, CV_8UC1);
			cvtColor(img_bgr, gray_bgr, CV_BGR2GRAY);
			normalize(gray_bgr, gray_bgr, 0, 255, NORM_MINMAX, CV_8UC1);
			blur(gray_bgr, gray_bgr, Size(3, 3));
			imshow("gray", gray_bgr);

			/// Detect edges using Threshold
			Mat img_thresh = Mat::zeros(frame.rows, frame.cols, CV_8UC1);
			threshold(gray_bgr, img_thresh, 155, 255, THRESH_BINARY_INV);
			dilate(img_thresh, img_thresh, 0, Point(-1, -1), 2);
			imshow("tresh",img_thresh);
			findContours(img_thresh, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
			for (int i = 0; i< contours.size(); i++)
			{
				vector<vector<Point> > contours_poly(contours.size());
				approxPolyDP(Mat(contours[i]), contours_poly[i], 3, true);
				Rect box = boundingRect(Mat(contours_poly[i]));
				if (box.width > 50 && box.height > 50 && box.width < 900 && box.height < 680) {
					rectangle(frame,
						box.tl(), box.br(),
						Scalar(0, 255, 0), 4);
				}
			
			}
	

			/// Show in a window
			namedWindow("Contours", CV_WINDOW_AUTOSIZE);
			imshow("Contours", frame);
		}
		char c = (char)waitKey(5);
			if (c == 27) break;
		std::swap(prevgray, gray);

	}
	return 0;
}