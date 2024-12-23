#include <memx/accl/daemon.h>

IfmapSend::IfmapSend(mxstream::MxService::Stub* stub, std::vector<mxstream::MxData*>& ifmap_grpc, mxstream::Ping* response, std::string uuid, int num_ports):
                                                                                                                    ifmap_grpc_{ifmap_grpc},
                                                                                                                    response_{response},
                                                                                                                    num_ports_{num_ports}
{
    idx_ = 0;
    context_.AddMetadata("uuid",uuid);
    auto deadline = std::chrono::system_clock::now() +
        std::chrono::milliseconds(10000);
    context_.set_deadline(deadline);
    stub->async()->send_ifmap(&context_,response_,this);
    NextWrite();
    StartCall();
}

void IfmapSend::OnWriteDone(bool ok) {
    if(!ok){
        StartWritesDone();
    }
    else{
        NextWrite();
    }
}

void IfmapSend::OnDone(const grpc::Status& s ){
    std::unique_lock<std::mutex> l(mu_);
    status_ = s;
    done_ = true;
    cv_.notify_one();
}

grpc::Status IfmapSend::Await() {
    std::unique_lock<std::mutex> l(mu_);
    cv_.wait(l, [this] { return done_; });
    return std::move(status_);
}

void IfmapSend::NextWrite() {
    if(idx_==num_ports_){
        StartWritesDone();
        return;
    }
    StartWrite(ifmap_grpc_[idx_]);
    idx_++;
}

OfmapRecv::OfmapRecv(mxstream::MxService::Stub* stub, mxstream::OfPorts& port_list, std::vector<mxstream::MxData*>& ofmap_grpc, std::string uuid):
                                                            ofmap_grpc_{ofmap_grpc}
{
    context_.AddMetadata("uuid",uuid);
    auto deadline = std::chrono::system_clock::now() +
        std::chrono::milliseconds(10000);
    context_.set_deadline(deadline);
    stub->async()->recevice_ofmap(&context_,&port_list,this);
    idx_=0;
    StartRead(ofmap_grpc_[idx_]);
    StartCall();
}

void OfmapRecv::OnReadDone(bool ok) {
    if (ok) {
        idx_++;
        StartRead(ofmap_grpc_[idx_]);
    }
}

void OfmapRecv::OnDone(const grpc::Status& s) {
    std::unique_lock<std::mutex> l(mu_);
    status_ = s;
    done_ = true;
    cv_.notify_one();
}

grpc::Status OfmapRecv::Await() {
    std::unique_lock<std::mutex> l(mu_);
    cv_.wait(l, [this] { return done_; });
    return std::move(status_);
}
