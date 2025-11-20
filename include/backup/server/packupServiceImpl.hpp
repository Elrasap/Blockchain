#pragma once

#include <grpcpp/grpcpp.h>
#include "backup.grpc.pb.h"    // generiert aus backup.proto

// Ein einfacher, stub-basierter Start der BackupService-Implementierung.
// Später füllst du den mit echter Logik (DB-Anbindung, Chunk-Store, etc.)

class BackupServiceImpl : public backup::BackupService::Service {
public:
    // Registrierung eines neuen Geräts
    ::grpc::Status RegisterDevice(::grpc::ServerContext* context,
                                  const backup::RegisterDeviceRequest* request,
                                  backup::RegisterDeviceResponse* response) override;

    // Heartbeat – Agent meldet sich regelmäßig
    ::grpc::Status Heartbeat(::grpc::ServerContext* context,
                             const backup::HeartbeatRequest* request,
                             backup::HeartbeatResponse* response) override;

    // Aktuelle Backup-Konfiguration für ein Gerät
    ::grpc::Status GetBackupConfig(::grpc::ServerContext* context,
                                   const backup::GetBackupConfigRequest* request,
                                   backup::GetBackupConfigResponse* response) override;

    // Agent fragt: welche Chunks fehlen?
    ::grpc::Status CheckChunks(::grpc::ServerContext* context,
                               const backup::CheckChunksRequest* request,
                               backup::CheckChunksResponse* response) override;

    // Stream: Agent lädt Chunks hoch
    ::grpc::Status UploadChunks(::grpc::ServerContext* context,
                                ::grpc::ServerReader<backup::UploadChunksRequest>* reader,
                                backup::UploadChunksResponse* response) override;

    // Snapshot-Metadaten committen
    ::grpc::Status CommitSnapshot(::grpc::ServerContext* context,
                                  const backup::CommitSnapshotRequest* request,
                                  backup::CommitSnapshotResponse* response) override;

    // Liste der Snapshots
    ::grpc::Status ListSnapshots(::grpc::ServerContext* context,
                                 const backup::ListSnapshotsRequest* request,
                                 backup::ListSnapshotsResponse* response) override;

    // Restore anstoßen
    ::grpc::Status RequestRestore(::grpc::ServerContext* context,
                                  const backup::RequestRestoreRequest* request,
                                  backup::RequestRestoreResponse* response) override;

    // Restore-Status
    ::grpc::Status GetRestoreStatus(::grpc::ServerContext* context,
                                    const backup::GetRestoreStatusRequest* request,
                                    backup::GetRestoreStatusResponse* response) override;
};

