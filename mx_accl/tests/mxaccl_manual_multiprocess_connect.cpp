#include <gtest/gtest.h>
#include "memx/accl/MxAcclMT.h"
#include <sys/wait.h>

using namespace std;
namespace fs = std::filesystem;

fs::path mx_home_path = MX::Utils::mx_get_home_dir();
fs::path samples_path = mx_home_path/"samples";

void normal_child_inference(fs::path model_path, bool local=false)
{
        MX::Runtime::MxAcclMT* accl;
        if(local) accl = new  MX::Runtime::MxAcclMT;
        else accl = new  MX::Runtime::MxAcclMT(true);
        int dfp_id = -1;
        try {
            dfp_id = accl->connect_dfp(model_path);
        } catch (const std::exception &e) {
            delete accl;
            FAIL()<<e.what();
        }
        ASSERT_EQ(dfp_id,0);
        std::this_thread::sleep_for(1000ms);
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

TEST(accl_manual_multiprocess_test,connect_dfp_same_dfp){
    fs::path model_path = samples_path/"models"/"CenterNet"/"centernet_onnx.dfp";
    pid_t child_pid_list[2];
    for (int i = 0; i < 2; i++) {
        std::this_thread::sleep_for(100ms);
        pid_t pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            exit(1);
        } else if (pid == 0) {
            normal_child_inference(model_path);
        }
        else{
            child_pid_list[i] = pid;
        }
    }
    for (int i = 0; i < 2; i++) {
        parent_status_check(child_pid_list,i);
    }
}

TEST(accl_manual_multiprocess_test,connect_dfp_different_dfp){
    fs::path model_path[2] = {samples_path/"models"/"CenterNet"/"centernet_onnx.dfp", samples_path/"models"/"yolov8s"/"yolov8s_tflite"/"yolov8s.dfp"};
    pid_t child_pid_list[2];
    for (int i = 0; i < 2; i++) {
        std::this_thread::sleep_for(100ms);
        pid_t pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            exit(1);
        } else if (pid == 0) {
            MX::Runtime::MxAcclMT* accl = new  MX::Runtime::MxAcclMT(true);
            int dfp_id = -1;
            try {
                dfp_id = accl->connect_dfp(model_path[i]);
                if(i==1){
                    delete accl;
                    FAIL()<<"different dfp used but didn't fail";
                }
            } catch (const std::exception &e) {
                if(i==1){
                    ASSERT_STREQ(e.what(),"A process with a different dfp is active on group: 0. Must use same dfp in multiple processes or wait for the other processes to end first");
                }
            }
            if(i==0) ASSERT_EQ(dfp_id,0);
            std::this_thread::sleep_for(1000ms);
            delete accl;
            exit(0);
        }
        else{
            child_pid_list[i] = pid;
        }
    }
    parent_status_check(child_pid_list,0);
}

TEST(accl_manual_multiprocess_test,connect_dfp_different_after_exit){
    fs::path model_path = samples_path/"models"/"CenterNet"/"centernet_onnx.dfp";
    pid_t child_pid_list[2];

    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed");
        exit(1);
    } else if (pid == 0) {
        normal_child_inference(model_path);
    }
    else{
        child_pid_list[0] = pid;
    }
    parent_status_check(child_pid_list,0);

    model_path = samples_path/"models"/"yolov8s"/"yolov8s_tflite"/"yolov8s.dfp";
    pid = fork();
    if (pid < 0) {
        perror("Fork failed");
        exit(1);
    } else if (pid == 0) {
        normal_child_inference(model_path);
    }
    else{
        child_pid_list[1] = pid;
    }
    parent_status_check(child_pid_list,1);
}

TEST(accl_manual_multiprocess_test,connect_dfp_remote_after_local){
    fs::path model_path = samples_path/"models"/"CenterNet"/"centernet_onnx.dfp";
    pid_t child_pid_list[2];
    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed");
        exit(1);
    } else if (pid == 0) {
        normal_child_inference(model_path);
    }
    else{
        child_pid_list[0] = pid;
    }
    parent_status_check(child_pid_list,0);

    pid = fork();
    if (pid < 0) {
        perror("Fork failed");
        exit(1);
    } else if (pid == 0) {
        normal_child_inference(model_path);
    }
    else{
        child_pid_list[1] = pid;
    }
    parent_status_check(child_pid_list,1);
}

TEST(accl_manual_multiprocess_test,connect_dfp_local_after_remote){
    fs::path model_path = samples_path/"models"/"CenterNet"/"centernet_onnx.dfp";
    pid_t child_pid_list[2];

    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed");
        exit(1);
    } else if (pid == 0) {
        normal_child_inference(model_path);
    }
    else{
        child_pid_list[0] = pid;
    }
    parent_status_check(child_pid_list,0);

    pid = fork();
    if (pid < 0) {
        perror("Fork failed");
        exit(1);
    } else if (pid == 0) {
        normal_child_inference(model_path);
    }
    else{
        child_pid_list[1] = pid;
    }
    parent_status_check(child_pid_list,1);
}

TEST(accl_manual_multiprocess_test,connect_dfp_remote_and_local){
    fs::path model_path = samples_path/"models"/"CenterNet"/"centernet_onnx.dfp";
    pid_t child_pid_list[2];
    for (int i = 0; i < 2; i++) {
        std::this_thread::sleep_for(100ms);
        pid_t pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            exit(1);
        } else if (pid == 0) {
            MX::Runtime::MxAcclMT* accl;
            if(i==0) accl = new  MX::Runtime::MxAcclMT(true);
            else accl = new  MX::Runtime::MxAcclMT;

            int dfp_id = -1;
            try {
                dfp_id = accl->connect_dfp(model_path);
                if(i==1){
                    delete accl;
                    FAIL()<<"local connected while remote is processing but didn't fail";
                }
            } catch (const std::exception &e) {
                if(i==1){
                    ASSERT_STREQ(e.what(),"Device 0 is not available to use");
                }
            }
            if(i==0)  ASSERT_EQ(dfp_id,0);
            std::this_thread::sleep_for(1000ms);
            delete accl;
            exit(0);
        }
        else{
            child_pid_list[i] = pid;
        }
    }
    parent_status_check(child_pid_list,0);
}

TEST(accl_manual_multiprocess_test,connect_dfp_local_and_remote){
    fs::path model_path = samples_path/"models"/"CenterNet"/"centernet_onnx.dfp";
    pid_t child_pid_list[2];
    for (int i = 0; i < 2; i++) {
        std::this_thread::sleep_for(100ms);
        pid_t pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            exit(1);
        } else if (pid == 0) {
            MX::Runtime::MxAcclMT* accl;
            if(i==1) accl = new  MX::Runtime::MxAcclMT(true);
            else accl = new  MX::Runtime::MxAcclMT;

            int dfp_id = -1;
            try {
                dfp_id = accl->connect_dfp(model_path);
                if(i==1){
                    delete accl;
                    FAIL()<<"remote connected while local is processing but didn't fail";
                }
            } catch (const std::exception &e) {
                if(i==1){
                    ASSERT_STREQ(e.what(),"Couldn't acquire lock on device 0");
                }
            }
            if(i==0) ASSERT_EQ(dfp_id,0);
            std::this_thread::sleep_for(1000ms);
            delete accl;
            exit(0);
        }
        else{
            child_pid_list[i] = pid;
        }
    }
    parent_status_check(child_pid_list,0);
}