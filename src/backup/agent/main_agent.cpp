#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include "backup.grpc.pb.h"

int main(int argc, char** argv) {
    std::string target = "localhost:50051";
    if (argc > 1) {
        target = argv[1];
    }

    auto channel = grpc::CreateChannel(target, grpc::InsecureChannelCredentials());
    std::unique_ptr<backup::BackupService::Stub> stub =
        backup::BackupService::NewStub(channel);

    // === RegisterDevice ===
    backup::RegisterDeviceRequest regReq;
    auto* info = regReq.mutable_info();
    info->set_name("test-device");
    info->set_os("linux");
    info->set_arch("x86_64");
    info->set_version("0.1");
    info->set_public_key("TODO");

    backup::RegisterDeviceResponse regResp;
    grpc::ClientContext ctx1;
    auto status = stub->RegisterDevice(&ctx1, regReq, &regResp);

    if (!status.ok()) {
        std::cerr << "RegisterDevice failed: " << status.error_message() << "\n";
        return 1;
    }
    std::cout << "[Agent] Registered as " << regResp.device_id()
              << " msg=" << regResp.message() << "\n";

    std::string deviceId = regResp.device_id();

    // === Heartbeat ===
    backup::HeartbeatRequest hbReq;
    hbReq.set_device_id(deviceId);
    hbReq.set_free_bytes(1024ull * 1024ull * 1024ull);
    hbReq.set_used_bytes(500ull * 1024ull * 1024ull);

    backup::HeartbeatResponse hbResp;
    grpc::ClientContext ctx2;
    status = stub->Heartbeat(&ctx2, hbReq, &hbResp);

    if (!status.ok()) {
        std::cerr << "Heartbeat failed: " << status.error_message() << "\n";
        return 1;
    }
    std::cout << "[Agent] Heartbeat OK: " << hbResp.message() << "\n";

    // === GetBackupConfig ===
    backup::GetBackupConfigRequest cfgReq;
    cfgReq.set_device_id(deviceId);

    backup::GetBackupConfigResponse cfgResp;
    grpc::ClientContext ctx3;
    status = stub->GetBackupConfig(&ctx3, cfgReq, &cfgResp);

    if (!status.ok()) {
        std::cerr << "GetBackupConfig failed: " << status.error_message() << "\n";
        return 1;
    }

    std::cout << "[Agent] Backup sets:\n";
    for (const auto& s : cfgResp.sets()) {
        std::cout << "  - " << s.path()
                  << " (recursive=" << (s.recursive() ? "true" : "false") << ")\n";
    }

    return 0;
}

