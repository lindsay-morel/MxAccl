#ifndef DAEMON_H
#define DAEMON_H

#include <grpcpp/grpcpp.h>
#include "mx_proc.grpc.pb.h"

#include <vector>
#include <condition_variable>

class IfmapSend : public grpc::ClientWriteReactor<mxstream::MxData> {
    public:
        IfmapSend(mxstream::MxService::Stub* stub, std::vector<mxstream::MxData*>& ifmap_grpc, mxstream::Ping* response, std::string uuid, int num_ports);
        
        void OnWriteDone(bool ok) override;
        
        void OnDone(const grpc::Status& s ) override;

        grpc::Status Await();

    private:
        grpc::ClientContext context_;
        std::vector<mxstream::MxData*> ifmap_grpc_;
        mxstream::Ping* response_;
        int idx_;
        int num_ports_;
        grpc::Status status_;
        bool done_ = false;
        std::mutex mu_;
        std::condition_variable cv_;
        void NextWrite();
};

class OfmapRecv : public grpc::ClientReadReactor<mxstream::MxData> {
    public:
        OfmapRecv(mxstream::MxService::Stub* stub, mxstream::OfPorts& port_list, std::vector<mxstream::MxData*>& ofmap_grpc, std::string uuid);

        void OnReadDone(bool ok) override;

        void OnDone(const grpc::Status& s) override; 

        grpc::Status Await();

    private:
        grpc::ClientContext context_;
        std::vector<mxstream::MxData*> ofmap_grpc_;
        int idx_;
        std::mutex mu_;
        std::condition_variable cv_;
        grpc::Status status_;
        bool done_ = false;
        mxstream::MxData dummy;
};

#endif