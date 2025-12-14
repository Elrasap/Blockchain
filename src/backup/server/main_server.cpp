#include <iostream>
#include <string>

#include <grpcpp/grpcpp.h>
#include "backupServiceImpl.hpp"

int main(int argc, char** argv) {
    std::string address("0.0.0.0:50051");

    BackupServiceImpl service;

    grpc::ServerBuilder builder;
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "[BackupServer] Listening on " << address << std::endl;

    server->Wait();
    return 0;
}

