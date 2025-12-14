#pragma once

#include <grpcpp/grpcpp.h>
#include "backup.grpc.pb.h"




class BackupServiceImpl : public backup::BackupService::Service {
public:

    ::grpc::Status RegisterDevice(::grpc::ServerContext* context,
                                  const backup::RegisterDeviceRequest* request,
                                  backup::RegisterDeviceResponse* response) override;


    ::grpc::Status Heartbeat(::grpc::ServerContext* context,
                             const backup::HeartbeatRequest* request,
                             backup::HeartbeatResponse* response) override;


    ::grpc::Status GetBackupConfig(::grpc::ServerContext* context,
                                   const backup::GetBackupConfigRequest* request,
                                   backup::GetBackupConfigResponse* response) override;


    ::grpc::Status CheckChunks(::grpc::ServerContext* context,
                               const backup::CheckChunksRequest* request,
                               backup::CheckChunksResponse* response) override;


    ::grpc::Status UploadChunks(::grpc::ServerContext* context,
                                ::grpc::ServerReader<backup::UploadChunksRequest>* reader,
                                backup::UploadChunksResponse* response) override;


    ::grpc::Status CommitSnapshot(::grpc::ServerContext* context,
                                  const backup::CommitSnapshotRequest* request,
                                  backup::CommitSnapshotResponse* response) override;


    ::grpc::Status ListSnapshots(::grpc::ServerContext* context,
                                 const backup::ListSnapshotsRequest* request,
                                 backup::ListSnapshotsResponse* response) override;


    ::grpc::Status RequestRestore(::grpc::ServerContext* context,
                                  const backup::RequestRestoreRequest* request,
                                  backup::RequestRestoreResponse* response) override;


    ::grpc::Status GetRestoreStatus(::grpc::ServerContext* context,
                                    const backup::GetRestoreStatusRequest* request,
                                    backup::GetRestoreStatusResponse* response) override;
};

