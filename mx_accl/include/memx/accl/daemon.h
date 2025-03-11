#ifndef DAEMON_H
#define DAEMON_H

#include <grpcpp/grpcpp.h>
#include "mx_proc.grpc.pb.h"

#include <vector>
#include <condition_variable>

/**
 * gRPC callback API implementation for sending ifmap to the daemon
 */
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

/**
 * gRPC completion-queue API implementation for receivingm ofmap from the daemon
 */
class OfmapCq{
    public:
     explicit OfmapCq(mxstream::MxService::Stub* stub, std::string uuid);
     
     grpc::Status get_ofmaps(mxstream::OfPorts& port_list, std::vector<mxstream::MxData*>& ofmap_grpc);

    private:
     std::string uuid_;
     mxstream::MxService::Stub* stub_;
};

#endif
