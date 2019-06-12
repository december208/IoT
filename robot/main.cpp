#include <stdio.h>
#include <vector>
#include <string>
#include <algorithm>
#include <opencv2/opencv.hpp>
#include <arpa/inet.h> 
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>


using namespace cv;
using namespace std;

static void edge_CB(int, void*);
static void rect_CB(int, void*);
static void color_CB(int, void*);
void setupWindow();
bool compareArea(vector<Point> a, vector<Point> b);
Mat drawInfo(Mat src, vector<RotatedRect> minRects);
Mat preProcess(Mat input);
vector<RotatedRect> findObj( Mat input);
string classifyObj(RotatedRect minRect);
void readThreshhold();
void writeThreshhold();

int low_thres=100;
int up_thres=200;
int dilate_itera = 3;
int n_obj = 3;
int ratioErrTolerance = 50;
int redLowHSV[] = {177,0,0};
int redHighHSV[] = {179,255,255};
int greenLowHSV[] = {0,0,0};
int greenHighHSV[] = {0,0,0};
static int MAX_ITERATE = 20;
static int MAX_THRES = 300;
static int MAX_N_obj = 10;
char win_name[] = "Display Image";
char ctrl_win[] = "Control Panel";
Mat src,srcR,srcG,full_size_src,src_contoure;

int center_y = 0;
int center_x = 0;

int green_cubic_x = 0;
int green_cubic_y = 0;
int red_cubic_x = 0;
int red_cubic_y = 0;
int green_oval_x = 0;
int green_oval_y = 0;



int main(int argc, char** argv ){
	
	

	int mode=3;
	setupWindow();
	VideoCapture cap(0); // open the default camera
    while(1){
		if(!cap.isOpened()) cout<<"not yet\n";
		else break;
	}
	
    //socket的建立
    int sockfd = 0;
    sockfd = socket(AF_INET , SOCK_STREAM , 0);

    if (sockfd == -1){
        printf("Fail to create a socket.");
    }

    //socket的連線

    struct sockaddr_in info;
    bzero(&info,sizeof(info));
    info.sin_family = PF_INET;

    //localhost test
    info.sin_addr.s_addr = inet_addr("10.42.0.65");
    info.sin_port = htons(4000);


    int err = connect(sockfd,(struct sockaddr *)&info,sizeof(info));
    

    if(err==-1){
        cout<<"Connection error";
    }
    

	cap.set(CV_CAP_PROP_FRAME_WIDTH,960);
	cap.set(CV_CAP_PROP_FRAME_HEIGHT,720);
	
    Mat cameraMatrix, distCoeffs, map1, map2;
    FileStorage fs("../camMatrix.xml", FileStorage::READ);
    fs["CameraMatrix"]>> cameraMatrix;
    fs["DistortionCoeffient"]>> distCoeffs;
    fs.release(); 
    cap>>src;
	Size imageSize = src.size(); 
	initUndistortRectifyMap(cameraMatrix, distCoeffs, Mat(), Mat(), imageSize, CV_32F, map1, map2); 


    //Rect rect(0, 0, 600, 600);

	
	while(1){
	    //cap>>full_size_src;	
	    //src = full_size_src(Rect(660, 400, 600, 600));
	    cap>>src;
	    //remap(src, src, map1, map2, INTER_LINEAR);
        //src = src(rect);
	    switch(mode){
		    case 1:
			    color_CB(0,0);
			    edge_CB(0,0);
			    break;
		    case 2:
			    color_CB(0,0);
			    edge_CB(0,0);
			    rect_CB(0,0);
			    break;
		    case 3:
			    color_CB(0,0);
			    break;
		    
            case 4:
                string data = format("%i %i %i %i %i %i",  red_cubic_x, red_cubic_y,green_cubic_x, green_cubic_y, green_oval_x, green_oval_y);
                char message[data.size() + 1];
                strcpy(message, data.c_str()); 
                send(sockfd,message,sizeof(message),0);
			    mode = 2;
                break;
		    	
			

	    }
	    int pressedKey = waitKey(10);
	    if( pressedKey == 113) break;//press q key to quit
	    if( pressedKey == 'e') {edge_CB(0,0);mode =1;}//press e key to show edge
	    if( pressedKey == 'r') {rect_CB(0,0);mode=2;}//press r key to show bounding rectangle
	    if( pressedKey == 'w') {color_CB(0,0);mode=3;}
	    if( pressedKey == 'g') {mode=4;}//press g key to grab
        if( pressedKey == 'i') {readThreshhold();}
        if( pressedKey == 'o') {writeThreshhold();}
	}
    close(sockfd);
    return 0;
}


static void color_CB(int, void*){
    Mat srcHSV;
    cvtColor(src, srcHSV, COLOR_BGR2HSV);
    inRange(srcHSV, Scalar(greenLowHSV[0],greenLowHSV[1],greenLowHSV[2]), Scalar(greenHighHSV[0],greenHighHSV[1],greenHighHSV[2]), srcG);
    inRange(srcHSV, Scalar(redLowHSV[0],redLowHSV[1],redLowHSV[2]), Scalar(redHighHSV[0],redHighHSV[1],redHighHSV[2]), srcR);    
    dilate(srcR,srcR, Mat(), Point(-1,-1), dilate_itera);
    erode(srcG,srcG, Mat(), Point(-1,-1), dilate_itera);
    imshow(win_name, srcR+srcG);
}

static void edge_CB(int, void*){
    Mat edge;
    edge = preProcess(srcR+srcG);
    imshow(win_name, edge);
}

static void rect_CB(int, void*){
    Mat dst, edge;
    vector<RotatedRect> minRects(n_obj);
    edge = preProcess(srcR+srcG);
    minRects = findObj(edge);
    dst = drawInfo(src, minRects);
    imshow(win_name, dst);
	
}

bool compareArea(vector<Point> a, vector<Point> b){
    return contourArea(a) > contourArea(b);
}

Mat  preProcess(Mat input){
    Mat edge;
    Canny(srcG+srcR, edge, low_thres, up_thres, 3);
    dilate(edge, edge, Mat(),Point(-1,-1), dilate_itera);
    erode(edge, edge, Mat(),Point(-1,-1), dilate_itera);
    return edge;
}

vector<RotatedRect> findObj(Mat input){
    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;
    vector<RotatedRect> minRects;
    findContours(input, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);
    vector<vector<Point> > contours_poly(contours.size());
    sort(contours.begin(), contours.end(), compareArea);
    if(n_obj>contours.size()) return minRects;
    for(int i=0; i<n_obj; i++){
        minRects.push_back(minAreaRect(Mat(contours[i])));
    }
    return minRects;
}

Mat drawInfo(Mat src, vector<RotatedRect> minRects){
    if(minRects.size()<n_obj) return src;
    Mat output = src.clone();
    Point2f shift(30,-30);
    for(int i=0; i<minRects.size(); i++){
        Point2f rect_points[4];
        minRects[i].points( rect_points );
        for( int j = 0; j < 4; j++ ){
            line( output, rect_points[j], rect_points[(j+1)%4], Scalar(0,0,0), 1, 8 );
        }
        string objType = classifyObj(minRects[i]);
        center_x = (-1)*minRects[i].center.x;
        center_y = minRects[i].center.y;
        string coord = format("( %i, %i)", center_x, center_y);
        circle(output, minRects[i].center, 2, Scalar(0,0,0), CV_FILLED,8);
        putText(output, objType, minRects[i].center-shift, 0, 1, Scalar(0,0,0), 5, 8);
	    putText(output, coord, minRects[i].center-shift, 0, 1, Scalar(255,0,0), 1, 8);
    

        green_cubic_x = -1;
        green_cubic_y = -1;
        red_cubic_x = -1;
        red_cubic_y = -1;
        green_oval_x = -1;
        green_oval_y = -1;
        if(objType == "Green Cubic"){
            green_cubic_x = center_x;
            green_cubic_y = center_y;
        }
        if(objType == "Red Cubic"){
            red_cubic_x = center_x;
            red_cubic_y = center_y;
        }
        if(objType == "Green Oval"){
            green_oval_x = center_x;
            green_oval_y = center_y;
        }
    
    }
    return output;
}

string classifyObj(RotatedRect minRect){
    float error = (float)ratioErrTolerance/100.0;
    float sideA = minRect.size.width;
    float sideB = minRect.size.height;
    float ratio=0;
    Point center = minRect.center;
    if(sideA>sideB || sideA == sideB) ratio = sideA/sideB;
    else ratio = sideB/sideA;
    if(fabs(ratio-1.0) < error){
        if(srcR.at<uchar>(center.y,center.x) == 255){
            return "Red Cubic";
        } 
        else if(srcG.at<uchar>(center.y,center.x) == 255){
            return "Green Cubic";
        }
        else{
            return "UNKNOWN";
        }
    }
    else if(fabs(ratio-2.0) < error){
        if(srcR.at<uchar>(center.y,center.x) == 255){
            return "UNKNOWN";
        }
        else if(srcG.at<uchar>(center.y,center.x) == 255){
            return "Green Oval";
        }
        else{
            return "UNKNOWN";
        }
    }
    else return "UNKNOWN";
}

void setupWindow(){
	namedWindow(win_name,WINDOW_NORMAL);
    createTrackbar("Upper Threshold: ", win_name, &up_thres, MAX_THRES, edge_CB);
    createTrackbar("Lower Threshold: ", win_name, &low_thres, MAX_THRES, edge_CB);
    createTrackbar("Dilate Iterate: ", win_name, &dilate_itera, MAX_ITERATE, edge_CB);
    createTrackbar("Number of object: ", win_name, &n_obj, MAX_N_obj, rect_CB);
    namedWindow(ctrl_win,WINDOW_NORMAL);
    createTrackbar("Red High H", ctrl_win, &(redHighHSV[0]), 179, color_CB);
    createTrackbar("Red Low H", ctrl_win, &(redLowHSV[0]), 179, color_CB);
    createTrackbar("Red High S", ctrl_win, &(redHighHSV[1]), 255, color_CB);
    createTrackbar("Red Low S", ctrl_win, &(redLowHSV[1]), 255, color_CB);
    createTrackbar("Red High V", ctrl_win, &(redHighHSV[2]), 255, color_CB);
    createTrackbar("Red Low V", ctrl_win, &(redLowHSV[2]), 255, color_CB);
    createTrackbar("Green High H", ctrl_win, &(greenHighHSV[0]), 179, color_CB);
    createTrackbar("Green Low H", ctrl_win, &(greenLowHSV[0]), 179, color_CB);
    createTrackbar("Green High S", ctrl_win, &(greenHighHSV[1]), 255, color_CB);
    createTrackbar("Green Low S", ctrl_win, &(greenLowHSV[1]), 255, color_CB);
    createTrackbar("Green High V", ctrl_win, &(greenHighHSV[2]), 255, color_CB);
    createTrackbar("Green Low V", ctrl_win, &(greenLowHSV[2]), 255, color_CB);
    createTrackbar("Ratio Error Tolerance (0\%~100\%)", ctrl_win, &ratioErrTolerance, 100, rect_CB);
    return ;
}

void readThreshhold(){
    FileStorage fs;
    fs.open("Threshhold.xml", FileStorage::READ);
    fs["redLowH"]>>redLowHSV[0];
    fs["redLowS"]>>redLowHSV[1];
    fs["redLowV"]>>redLowHSV[2];
    fs["redHighH"]>>redHighHSV[0];
    fs["redHighS"]>>redHighHSV[1];
    fs["redHighV"]>>redHighHSV[2];
    fs["greenLowH"]>>greenLowHSV[0];
    fs["greenLowS"]>>greenLowHSV[1];
    fs["greenLowV"]>>greenLowHSV[2];
    fs["greenHighH"]>>greenHighHSV[0];
    fs["greenHighS"]>>greenHighHSV[1];
    fs["greenHighV"]>>greenHighHSV[2];
    fs.release();     

    setTrackbarPos("Red High H", ctrl_win, redHighHSV[0]);
    setTrackbarPos("Red Low H", ctrl_win, redLowHSV[0]);
    setTrackbarPos("Red High S", ctrl_win, redHighHSV[1]);
    setTrackbarPos("Red Low S", ctrl_win, redLowHSV[1]);
    setTrackbarPos("Red High V", ctrl_win, redHighHSV[2]);
    setTrackbarPos("Red Low V", ctrl_win, redLowHSV[2]);
    setTrackbarPos("Green High H", ctrl_win,greenHighHSV[0]);
    setTrackbarPos("Green Low H", ctrl_win, greenLowHSV[0]);
    setTrackbarPos("Green High S", ctrl_win, greenHighHSV[1]);
    setTrackbarPos("Green Low S", ctrl_win, greenLowHSV[1]);
    setTrackbarPos("Green High V", ctrl_win, greenHighHSV[2]);
    setTrackbarPos("Green Low V", ctrl_win, greenLowHSV[2]);
    
}

void writeThreshhold(){
    FileStorage fs;
    fs.open("Threshhold.xml", FileStorage::WRITE);
    fs<<"redLowH"<<redLowHSV[0];
    fs<<"redLowS"<<redLowHSV[1];
    fs<<"redLowV"<<redLowHSV[2];
    fs<<"redHighH"<<redHighHSV[0];
    fs<<"redHighS"<<redHighHSV[1];
    fs<<"redHighV"<<redHighHSV[2];
    fs<<"greenLowH"<<greenLowHSV[0];
    fs<<"greenLowS"<<greenLowHSV[1];
    fs<<"greenLowV"<<greenLowHSV[2];
    fs<<"greenHighH"<<greenHighHSV[0];
    fs<<"greenHighS"<<greenHighHSV[1];
    fs<<"greenHighV"<<greenHighHSV[2];
    fs.release();     
}