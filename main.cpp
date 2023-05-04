#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>

using namespace cv;
using namespace std;


int hmin = 0, smin = 0, vmin = 0;
int hmax = 125, smax = 85, vmax = 255;
int diameter = 80;
int CannythreshA = 35, CannythreshB = 255;

Mat img_copy,img_copy2,  imgProcessedDefect,imgProcessedDiameter, imgDefect, imgDiameter;

Mat imgHSV, mask;


Mat preProcessDefect(Mat image) {
	Mat imgGray, imgBlur, imgCanny, imgFilterd, imgEro, imgDil;
	// Apply Gaussian Blur and convert image to HSV
	GaussianBlur(image, imgBlur, Size(3, 3), 3);
	cvtColor(imgBlur, imgHSV, COLOR_BGR2HSV);

	// Apply color filter to find defects
	Scalar lower(hmin, smin, vmin);
	Scalar upper(hmax, smax, vmax);
	inRange(imgHSV, lower, upper, mask);
	Mat kernel1 = getStructuringElement(MORPH_ELLIPSE, Size(5, 5));
	Mat kernel2 = getStructuringElement(MORPH_ELLIPSE, Size(13, 13));
	erode(mask, imgEro, kernel1);
	dilate(imgEro, imgDil, kernel2);

	return (imgDil);
}


Mat preProcessDiameter(Mat image) {

	Mat imgGray, imgBlur, imgCanny, imgFilterd, imgEro, imgDil;
	cvtColor(image, imgGray, COLOR_BGR2GRAY);


	// Apply Gaussian Blur
	GaussianBlur(imgGray, imgBlur, Size(3, 3), 0);
	GaussianBlur(imgBlur, imgBlur, Size(3, 3), 3);

	/* Used trackbars to find suitable values for the thresholds
	namedWindow("trackbars", (640, 200));
	createTrackbar("cannyA", "trackbars", &cannyA, 255);
	createTrackbar("cannyB", "trackbars", &cannyB, 255);
	createTrackbar("threshA", "trackbars", &threshA, 255);
	createTrackbar("threshB", "trackbars", &threshB, 255);*/

	// Apply Canny filter
	// Canny(imgBlur, imgCanny, cannyA, cannyB);

	// Apply binary threshold
	threshold(imgGray, imgFilterd, CannythreshA, CannythreshB, THRESH_BINARY);

	// Apply Erosion to remove noise
	Mat kernel = getStructuringElement(MORPH_RECT, Size(5, 5));
	erode(imgFilterd, imgEro, kernel);
	dilate(imgEro, imgDil, kernel);
	return (imgDil);
}


Mat findDefect(Mat image) {
	vector<vector<Point>> contours;
	vector<Point> contour;
	vector<Vec4i> hierarchy;
	findContours(image, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);


	for (int j = 0; j < contours.size(); j++)
	{
		int area = contourArea(contours[j]);
		if (area > 200) {

			// Find defects and their centers
			Rect bbox = boundingRect(contours[j]);
			Point center(bbox.x + bbox.width / 2, bbox.y + bbox.height / 2);
			Size axes(bbox.width / 2, bbox.height / 2);

			// Draw the ellipse around the defect
			ellipse(img_copy, center, axes, 0, 0, 360, Scalar(255, 0, 0), 2);

			// Classification based on the shape
			if (bbox.width > 0.8 * diameter) {
				string text = "Cut";
				putText(img_copy, text, Point(center.x + 100, center.y), FONT_HERSHEY_PLAIN, 2, Scalar(255, 255, 255), 2);
			}
			else if (bbox.width < 0.8 * diameter && bbox.height < 0.8 * diameter) {
				string text = "Pin Hole";
				putText(img_copy, text, Point(center.x + 100, center.y), FONT_HERSHEY_PLAIN, 2, Scalar(255, 255, 255), 2);
			}
			else {
				string text = "Scratches";
				putText(img_copy, text, Point(center.x + 100, center.y), FONT_HERSHEY_PLAIN, 2, Scalar(255, 255, 255), 2);
			}
		}
	}
	return (img_copy);

}

Mat findDiameter(Mat image) {

	vector<vector<Point>> contours;
	vector<Point> contour;
	vector<Vec4i> hierarchy;
	findContours(image, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

	// Draw the contour around the entire wire
	for (int j = 0; j < contours.size(); j++)
	{
		int area = contourArea(contours[j]);
		if (area > 1000) {
			drawContours(image, contours, j, Scalar(0, 0, 255), 2);
			contour = contours[j];
			break;
		}
	}

	// Count contour points on the left edge

	int n_leftEdge = 0;
	for (int j = 0; j < contour.size() - 1; j++) {
		if (contour[j + 1].y > contour[j].y) {
			n_leftEdge++;
		}
	}

	RNG rng;

	int k = 0;
	while (k < 3) {

		// Randomly select 3 points on the left edge of the wire
		int random_index = rng.uniform((k + 1) * n_leftEdge / 5, (k + 2) * n_leftEdge / 5);
		int y = contour[random_index].y;
		int x;
		for (int j = 0; j < contour.size() - 1; j++) {

			// Dind the closest contour point on the right edge of the wire
			if (j != random_index && contour[j].y >= y && contour[j + 1].y <= y) {
				if (contour[j].y == y) {
					x = contour[j].x - contour[random_index].x;
				}
				else if (contour[j + 1].y == y) {
					x = contour[j + 1].x - contour[random_index].x;
				}
				else {
					x = contour[j + 1].x + (contour[j + 1].x - contour[j].x) / (contour[j + 1].y - contour[j].y) - contour[random_index].x;
				}

				// Calculate the distance between left and right point
				string text = "Diameter: " + to_string(x);

				// Draw a red line and label the width
				line(img_copy2, Point(contour[random_index].x, y), Point(x + contour[random_index].x, y), Scalar(0, 0, 255), 2);
				putText(img_copy2, text, Point(contour[j].x + 50, y + 10), FONT_HERSHEY_PLAIN, 2, Scalar(255, 255, 255), 2);
			}
		}
		k++;
	}
	return(img_copy2);
}



int main() {

	String folderpath = "Input Images/*.bmp";
	vector<String> filenames;
	glob(folderpath, filenames);

	for (int i = 0; i < filenames.size(); i++) {
		//for (int i = 0; i < 1; i++) {

		Mat img = imread(filenames[i]);
		img_copy = img.clone();
		img_copy2 = img_copy;
		
		imgProcessedDiameter = preProcessDiameter(img_copy);
		imgProcessedDefect = preProcessDefect(img_copy);

		imgDefect = findDefect(imgProcessedDefect);
		imgDiameter = findDiameter(imgProcessedDiameter);

		string txt1 = "output/Defect" + to_string(i + 1) + ".bmp";
		string txt2 = "output/Diameter" + to_string(i + 1) + ".bmp";

		namedWindow(txt1, WINDOW_NORMAL);
		namedWindow(txt2, WINDOW_NORMAL);

		moveWindow(txt1, 0, 0);
		moveWindow(txt2, imgDefect.cols, 0);

		imshow(txt1, imgDefect);
		imshow(txt2, imgDiameter);

		waitKey(0);
		destroyAllWindows();

	}
}

