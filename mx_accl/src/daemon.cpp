#ifndef DISABLE_DAEMON

#include <memx/accl/daemon.h>

// This constructor creates the instance of the IfmapSend class. RPC is started here and the ifmap writing is started.
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
    //Sending context and response to the rpc
    stub->async()->send_ifmap(&context_,response_,this);
    NextWrite();
    StartCall();
}

//Exceuted everytime a featuremap needs to be sent
void IfmapSend::OnWriteDone(bool ok) {
    if(!ok){
        StartWritesDone();
    }
    else{
        NextWrite();
    }
}

//Exceuted when all the featuremaps have been sent
void IfmapSend::OnDone(const grpc::Status& s ){
    std::unique_lock<std::mutex> l(mu_);
    status_ = s;
    done_ = true;
    //Notify the main thread that is waiting on this RPC to finish
    cv_.notify_one();
}

//Function that can be used to block the main thread until the RPC is done
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
    //Sending the featuremap based on current index
    StartWrite(ifmap_grpc_[idx_]);
    idx_++;
}

OfmapCq::OfmapCq(mxstream::MxService::Stub* stub, std::string uuid):
                            uuid_{uuid},
                            stub_{stub}{}

/**
 * Retrieves output feature maps (ofmaps) from the daemon using gRPC async completion queue.
 * 
 * @param port_list A reference to the list of ports from which ofmaps are to be received.
 * @param ofmap_grpc A reference to a vector where received ofmaps will be stored.
 * 
 * @return grpc::Status indicating the success or failure of the operation. 
 *         Returns grpc::StatusCode::NOT_FOUND if the context ID of the first ofmap is 10000.
 *         Returns grpc::Status::CANCELLED if the context ID of the first ofmap is 10001.
 */
grpc::Status OfmapCq::get_ofmaps( mxstream::OfPorts& port_list, std::vector<mxstream::MxData*>& ofmap_grpc) {
    grpc::ClientContext ctx;
    ctx.AddMetadata("uuid",uuid_);
    std::unique_ptr<grpc::CompletionQueue> cq_ = std::make_unique<grpc::CompletionQueue>();
    //Setting up the async reader
    std::unique_ptr<grpc::ClientAsyncReader<mxstream::MxData> > reader(stub_->Asyncrecevice_ofmap(&ctx, port_list, cq_.get(), nullptr));
    void* got_tag;
    bool ok = false;
    bool reading = true;
    int i = 0;
    grpc::Status status = grpc::Status::OK;
    while (reading) {
        if (cq_->Next(&got_tag, &ok)) {
            if (ok) {        
                //Reading the each ofmap till done
                reader->Read(ofmap_grpc[i],nullptr);
                i++;
            } else {
                reading = false;
            }
        }
    }
    reader->Finish(&status, nullptr);

    //Checking for any out of normal cases
    if(ofmap_grpc[0]->ctx_id()==10000){
        status = grpc::Status(grpc::StatusCode::NOT_FOUND, "Top of the queue not equal");
    }
    else if(ofmap_grpc[0]->ctx_id()==10001){
        status = grpc::Status::CANCELLED;
    }
    cq_->Shutdown();
    while (cq_->Next(&got_tag, &ok)) {
        // Draining the queue to avoid memory leaks
    }
    return status;
}

#endif
