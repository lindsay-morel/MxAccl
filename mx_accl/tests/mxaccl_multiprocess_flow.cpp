#include <gtest/gtest.h>
#include "memx/accl/MxAccl.h"
#include <sys/wait.h>

using namespace std;
namespace fs = std::filesystem;
#define NUM_TEST_FRAMES 100

fs::path mx_accl_path = MX::Utils::mx_get_accl_dir();
fs::path models_path = mx_accl_path/"tests"/"models";

atomic_int  sent_num_frames_1 = 0;
atomic_int  recv_num_frames_1 = 0;
atomic_int  sent_num_frames_2 = 0;
atomic_int  recv_num_frames_2 = 0;

void init_num_frames(){
    sent_num_frames_1 = 0;
    recv_num_frames_1 = 0;
    sent_num_frames_2 = 0;
    recv_num_frames_2 = 0;
}


void test_num_frames(){
    EXPECT_EQ(sent_num_frames_1.load(),recv_num_frames_1.load());
    EXPECT_EQ(sent_num_frames_2.load(),recv_num_frames_2.load());
    EXPECT_EQ(recv_num_frames_1.load(),NUM_TEST_FRAMES);
    EXPECT_EQ(recv_num_frames_2.load(),NUM_TEST_FRAMES);
}

struct model_paths{
    fs::path  dfp_path; 
    fs::path  pre_path; 
    fs::path  post_path;
};

bool input_callback_tf1(vector<const MX::Types::FeatureMap<float>*> dst, int){
    if(sent_num_frames_1.load()>=NUM_TEST_FRAMES)
    return false;
    std::vector<float> input = {1,1,2,3,4,4,5,2,2,1,3,6};
    dst[0]->set_data(input.data());
    sent_num_frames_1++;
    return true;
}

bool input_callback_tf2(vector<const MX::Types::FeatureMap<float>*> dst, int){
    if(sent_num_frames_2.load()>=NUM_TEST_FRAMES)
    return false;
    std::vector<float> input = {5,5,1,1,7,2,2,6,4,0,3,2};
    dst[0]->set_data(input.data());
    sent_num_frames_2++;
    return true;
}

bool output_callback_tflite1(vector<const MX::Types::FeatureMap<float>*> src, int){
    std::vector<float> output = {0,0};
    src[0]->get_data(output.data());
    EXPECT_EQ(95.0,output[0]);
    EXPECT_EQ(116.0,output[1]);
    recv_num_frames_1++;
    return true;
}

bool output_callback_tflite2(vector<const MX::Types::FeatureMap<float>*> src, int){
    std::vector<float> output = {0,0};
    src[0]->get_data(output.data());
    EXPECT_EQ(104.0,output[0]);
    EXPECT_EQ(102.0,output[1]);
    recv_num_frames_2++;
    return true;
}

void normal_child_inference(model_paths& path, bool multi=false)
{
    MX::Runtime::MxAccl* accl = new  MX::Runtime::MxAccl(true);
    int dfp_id = -1;
    try {
        dfp_id = accl->connect_dfp(path.dfp_path);
    } catch (const std::exception &e) {
        delete accl;
        FAIL()<<e.what();
    }
    ASSERT_EQ(dfp_id,0);
    accl->connect_pre_model(path.pre_path);
    accl->connect_post_model(path.post_path);
    if(multi){
        accl->connect_stream(input_callback_tf1,output_callback_tflite1,0);
        accl->connect_stream(input_callback_tf2,output_callback_tflite2,1,1);
    }
    else{
        accl->connect_stream(input_callback_tf1,output_callback_tflite1,0);
        accl->connect_stream(input_callback_tf2,output_callback_tflite2,1);        
    }
    accl->start();
    accl->wait();
    accl->stop();
    test_num_frames();
    delete accl;
    exit(0); 
}

void parent_status_check(pid_t child_pid_list[],int i)
{
    int status;
    waitpid(child_pid_list[i], &status, 0);
    if (WIFEXITED(status)) {
        int exit_status = WEXITSTATUS(status);
        ASSERT_EQ(exit_status,0);
    } else if (WIFSIGNALED(status)) {
        FAIL()<<"Child PID exited abruptly with status: "<<status;
    }
}

TEST(accl_multiprocess_flow,single_model){
    model_paths path;
    path.dfp_path = models_path/"cascadePlus"/"prepost_tflite.dfp";
    path.pre_path = models_path/"plugin_models"/"tflite"/"prepost_pre.tflite";
    path.post_path = models_path/"plugin_models"/"tflite"/"prepost_post.tflite";
    pid_t child_pid_list[2];
    for (int i = 0; i < 2; i++) {
        std::this_thread::sleep_for(100ms);
        pid_t pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            exit(1);
        } else if (pid == 0) {
            normal_child_inference(path);
        }
        else{
            child_pid_list[i] = pid;
        }
    }
    for (int i = 0; i < 2; i++) {
        parent_status_check(child_pid_list,i);
    }
}

TEST(accl_multiprocess_flow,multi_model){
    model_paths path;
    path.dfp_path = models_path/"cascadePlus"/"prepost_tflite_multimodel.dfp";
    path.pre_path = models_path/"plugin_models"/"tflite"/"prepost_pre.tflite";
    path.post_path = models_path/"plugin_models"/"tflite"/"prepost_post.tflite";
    pid_t child_pid_list[2] = {0,0};
    for (int i = 0; i < 2; i++) {
        std::this_thread::sleep_for(100ms);
        pid_t pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            exit(1);
        } else if (pid == 0) {
            normal_child_inference(path);
        }
        else{
            child_pid_list[i] = pid;
        }
    }
    for (int i = 0; i < 2; i++) {
        parent_status_check(child_pid_list,i);
    }
    ASSERT_NE(child_pid_list[0],0);
    ASSERT_NE(child_pid_list[1],0);
    ASSERT_NE(child_pid_list[0],child_pid_list[1]);
}