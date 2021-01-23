#include <thread>
#include <mutex>
#include <atomic>

#include <vector>
#include <algorithm>
#include <iostream>

#include <opencv2/highgui/highgui.hpp>
//#include <opencv2/opencv.hpp>

using namespace cv;
//using namespace std;
using std::cout;
using std::cin;
using std::endl;
using std::thread;
using std::mutex;
using std::string;
using std::for_each;
using std::vector;
using std::atomic;
using std::unique_lock;
using std::ref;

mutex mtx_1;
atomic<int> atom{0};

vector<string> image_name=
{
	"b_bigbug0000_croppped.png",
	"b_bigbug0001_croppped.png",
	"b_bigbug0002_croppped.png",
	"b_bigbug0003_croppped.png",
	"b_bigbug0004_croppped.png",
	"b_bigbug0005_croppped.png",
	"b_bigbug0006_croppped.png",
	"b_bigbug0007_croppped.png",
	"b_bigbug0008_croppped.png",
	"b_bigbug0009_croppped.png",
	"b_bigbug0010_croppped.png",
	"b_bigbug0011_croppped.png",
	"b_bigbug0012_croppped.png"
};

short fSharpen[3][3]={{0,-1,0},{-1,5,-1},{0,-1,0}};
short fSharpen2[3][3]={{1,-2,1},{-2,5,-2},{1,-2,1}};
short fSharpen3[3][3]={{1,-3,1},{-3,9,-3},{1,-3,1}};
short fEdgeDetect[3][3]={{-1,-1,-1},{-1,8,-1},{-1,-1,-1}};
short fPrewitt0y[3][3]={{-1,-1,-1},{0,1,0},{1,1,1}};
short fPrewitt0x[3][3]={{-1,0,1},{-1,1,1},{-1,0,1}};


void histogram(Mat_<Vec3b> image)
{
	unsigned long histogram[256];
	for(auto i=0;i<256;i++) histogram[i]=0;

	for(auto i=0;i<image.rows;i++)
		for(auto j=0;j<image.cols;j++)
			for(auto k=0;k<3;k++)
			{
				histogram[image(i,j)[k]]++;
			}
	for(auto i=0;i<256;i++) cout<<i<<" "<<histogram[i]<<endl;
}


int filter_and_norm(Mat_<Vec3b>& input, Mat_<Vec3b>& output, short filter[3][3])
{
	int max=0;
	int min=255;
	for(auto i=1;i<input.rows-1;i++)
	{
		for(auto j=1;j<input.cols-1;j++)
		{
			for(auto k=0;k<3;k++)
			{
				auto sum=0;
				for(auto a=-1;a<2;a++)
				{
					for(auto b=-1;b<2;b++)
					{
						sum+=input(i+a,j+b)[k]*filter[1+a][1+b];
					}
				}
				if(sum>max) max=sum;
				if(sum<min) min=sum;
				output(i,j)[k]=sum;
			}
		}
	}
//	cout<<"max: "<<max<<" min: "<<min<<endl;
	int max_return=0, tmp;
	int min_return=255;
	int total_return=0;
	for(auto i=0;i<output.rows;i++)
		for(auto j=0;j<output.cols;j++)
		{
			for(auto k=0;k<3;k++)
			{
				tmp=256*(output(i,j)[k]-min)/(max-min);
				output(i,j)[k]=tmp;
				if(tmp>max_return) max_return=tmp;
				if(tmp<min_return) min_return=tmp;
			}
		}
//	cout<<"max: "<<max_return<<" min: "<<min_return<<endl;
	return min_return;
}


void add_image(Mat_<Vec3b>& result,Mat& gray_result,Mat& area, string name_of_next_image, int order)
{
	Mat_<Vec3b> next_image = imread(name_of_next_image);

	if(next_image.empty())
	{
		cout<<"Error: "<<name_of_next_image<<" open failed"<<endl;
		return;
	}
	Mat_<Vec3b> filtered_next1(next_image.rows,next_image.cols,Vec3b(0,0,0));

	int min=filter_and_norm(next_image,filtered_next1, fSharpen);
	int add=10;

	while(atom!=order);
	cout<<name_of_next_image<<" is processing."<<endl;

	for(auto i=0;i<result.rows;i++)
		for(auto j=0;j<result.cols;j++)
			{
				if((filtered_next1(i,j)[0]<=min+add)||(filtered_next1(i,j)[1]<=min+add)||(filtered_next1(i,j)[2]<=min+add))
				{	
					if(area.data[area.step*i+j]==0)
					{
						result(i,j)[0]=next_image(i,j)[0];
						result(i,j)[1]=next_image(i,j)[1];
						result(i,j)[2]=next_image(i,j)[2];
						area.data[area.step*i+j]=1;
						gray_result.data[gray_result.step*i+j]=255-order*17;
					}
				}			
			}
	atom++;
}

bool save_and_show(Mat& image, string name)
{
	bool is_success=imwrite(name,image);
	if(is_success=false)
	{
		cout<<"Saving failed"<<endl;
		cin.get();
		return false;
	}
	cout<<"The image is successfully saved"<<endl;
	namedWindow(name);
	imshow(name,image);
	waitKey(0);
	destroyWindow(name);
	return true;
}

void create_first_gray(Mat_<Vec3b>& input, Mat& output)
{
	unique_lock<mutex> lock(mtx_1);
	cout<<"First gray image is creating"<<endl;
	int add=10;
	Mat_<Vec3b> filtered_image(input.rows,input.cols,Vec3b(0,0,0));
	int min=filter_and_norm(input,filtered_image,fSharpen);
	for(auto i=0;i<input.rows;i++)
		for(auto j=0;j<input.cols;j++)
			if((filtered_image(i,j)[0]<=min+add)||(filtered_image(i,j)[1]<=min+add)||(filtered_image(i,j)[2]<=min+add))
				output.data[output.step*i+j]=255;
}


int main(int argc, char** argv)
{

	Mat_<Vec3b> image = imread(image_name[0]);
	if(image.empty()) cout<<"OpenCV errors"<<endl;
	if(!image_name.size()) return 0;

	Mat area(image.rows,image.cols,CV_8U);
	Mat gray_image(image.rows,image.cols,CV_8U);

	vector<thread> workers;
	workers.emplace_back(create_first_gray,ref(image),ref(gray_image));
	atom++;
	for(auto i=1;i<image_name.size();i++) workers.emplace_back(add_image,ref(image),ref(gray_image),ref(area),image_name[i],(int)i);
	for_each(workers.begin(),workers.end(),[](thread& t){t.join();});

	save_and_show(image,"my_color_results.png");
	save_and_show(gray_image,"my_gray_results.png");
	return 0;
}
