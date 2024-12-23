#include <gtest/gtest.h>
#include "memx/accl/MxAcclMT.h"
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

void tf_send(MX::Runtime::MxAcclMT* accl, int streamidx, int model_idx){

    int i =0;
    std::vector<float> input;
    while(++i <= NUM_TEST_FRAMES){
        if(streamidx ==0){
            input = {1,1,2,3,4,4,5,2,2,1,3,6};
            sent_num_frames_1++;
        }
        else{
            input = {5,5,1,1,7,2,2,6,4,0,3,2};
            sent_num_frames_2++;
        }
        std::vector<float*> input_data;
        input_data.push_back(input.data());
        accl->send_input(input_data, model_idx, streamidx, false);
    }
}

void tflite_receive(MX::Runtime::MxAcclMT* accl, int streamidx, int model_idx){
    int i = 0;
    while(++i <= NUM_TEST_FRAMES){
        float* fmap = new float[2];
        std::vector<float*> ofmap;
        int recvinf_label;
        ofmap.push_back(fmap);
        if(!accl->receive_output(ofmap, model_idx, streamidx, false)){
            delete[] fmap;
            break;
        }
        
        if(streamidx == 0){

            EXPECT_EQ(95.0,ofmap[0][0]);
            EXPECT_EQ(116.0,ofmap[0][1]);
            recv_num_frames_1++;
        }
        else{
            EXPECT_EQ(104.0,ofmap[0][0]);
            EXPECT_EQ(102.0,ofmap[0][1]);
            recv_num_frames_2++;
        }
        delete[] fmap;
    }
}

void normal_child_inference(model_paths& path, bool multi=false)
{
    MX::Runtime::MxAcclMT* accl = new  MX::Runtime::MxAcclMT(true);
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
    std::thread* send_thread;
    std::thread* recv_thread;
    std::thread* send_thread_1;
    std::thread* recv_thread_1;
    if(multi){
        send_thread =   new std::thread(tf_send, accl,0,0);
        recv_thread =   new std::thread(tflite_receive,accl,0,0);
        send_thread_1 = new std::thread(tf_send, accl, 1,1);
        recv_thread_1 = new std::thread(tflite_receive, accl,1,1);
    }
    else{
        send_thread =   new std::thread(tf_send, accl,0,0);
        recv_thread =   new std::thread(tflite_receive,accl,0,0);
        send_thread_1 = new std::thread(tf_send, accl, 1,0);
        recv_thread_1 = new std::thread(tflite_receive, accl,1,0);
    }
    if(send_thread->joinable() && send_thread_1->joinable()){
        send_thread->join();
        send_thread_1->join();
    }
    if(recv_thread->joinable() && recv_thread_1->joinable()){
        recv_thread->join();
        recv_thread_1->join();
    }
    test_num_frames();
    delete accl;
    delete send_thread;
    delete recv_thread;
    delete send_thread_1;
    delete recv_thread_1;
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