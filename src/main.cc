#include <stdio.h>
#include <memory>
#include <future>
#include <omp.h>

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "rkYolov5s.hpp"
#include "rknnPool.hpp"


#include "bindCpu.h"

int main(int argc, char **argv)
{
    char *model_name = NULL;
    if (argc != 3)
    {
        printf("Usage: %s <rknn model> <jpg> \n", argv[0]);
        return -1;
    }
    // 参数二，模型所在路径/The path where the model is located
    model_name = (char *)argv[1];
    // 参数三, 视频/摄像头
    char *vedio_name = argv[2];

    // 初始化rknn线程池/Initialize the rknn thread pool
    int threadNum = 6;
    rknnPool<rkYolov5s, cv::Mat, frame_detect_result_t> testPool(model_name, threadNum);
    if (testPool.init() != 0)
    {
        printf("rknnPool init fail!\n");
        return -1;
    }

    cv::setNumThreads(4);
    cv::setUseOptimized(true);

    omp_set_num_threads(4);

    cv::VideoCapture capture;
    if (strlen(vedio_name) == 1)
        capture.open((int)(vedio_name[0] - '0'));
    else
        capture.open(vedio_name);


    auto cap_video_thread = std::async(std::launch::async, [&](){
        cv::Mat frame;
         while (capture.read(frame)){
            testPool.put(frame);
         }
    });

    auto get_result = std::async(std::launch::async, [&](){
        std::atomic<size_t> frames(0);
        std::atomic<uint64_t> time_sum(0);
        frame_detect_result_t frame_detect_result;
        size_t frame_count = 0;
        u_int64_t sum_t = 0;
        while (true){
            auto startTime = std::chrono::steady_clock::now();

            testPool.get(frame_detect_result);
            auto currentTime = std::chrono::steady_clock::now();
            auto elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - startTime).count();
            frame_count++;

            // auto group = frame_detect_result.group;
            // auto org_frame = frame_detect_result.org_frame;

            // if(group.count > 0){
            //     char text[256];
            //     for (size_t i = 0; i < group.count; i++){
            //         detect_result_t detect_result = group.results[i];
            //         sprintf(text, "%s %.1f%%", detect_result.name, detect_result.prop * 100);
            //         BOX_RECT box = detect_result.box;
            //         int x1 = box.left;
            //         int y1 = box.top;
            //         int x2 = box.right;
            //         int y2 = box.bottom;
            //         rectangle(org_frame, cv::Point(x1, y1), cv::Point(x2, y2), cv::Scalar(256, 0, 0, 256), 1);
            //         putText(org_frame, text, cv::Point(x1, y1 + 12), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255, 255, 255));
            //     }
            // } 
           
            sum_t+=elapsedTime;
            if(frame_count%240==0){
                printf("240 帧内平均帧率: %f fps\n",  frame_count*1.0 / (sum_t/1000.0/1000.0));
                frame_count=0;
                sum_t=0;
            }
            
        }
    });


    cap_video_thread.get();
    get_result.get();

    return 0;
}