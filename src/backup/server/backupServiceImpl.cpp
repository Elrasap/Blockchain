#include "backupServiceImpl.hpp"
#include <iostream>
#include <chrono>

using ::grpc::Status;
using ::grpc::StatusCode;

Status BackupServiceImpl::RegisterDevice(::grpc::ServerContext*,
                                         const backup::RegisterDeviceRequest* request,
                                         backup::RegisterDeviceResponse* response) {
    const auto& info = request->info();
    std::cout << "[BackupService] RegisterDevice from "
              << info.name() << " (" << info.os() << "/" << info.arch() << ")\n";

    // TODO: hier echte Device-ID aus DB holen / erzeugen
    response->set_device_id("dev-" + info.name());
    response->set_message("Registered OK");
    return Status::OK;
}

Status BackupServiceImpl::Heartbeat(::grpc::ServerContext*,
                                    const backup::HeartbeatRequest* request,
                                    backup::HeartbeatResponse* response) {
    std::cout << "[BackupService] Heartbeat from device " << request->device_id()
              << " free=" << request->free_bytes()
              << " used=" << request->used_bytes() << "\n";

    response->set_ok(true);
    response->set_message("heartbeat ok");
    return Status::OK;
}

Status BackupServiceImpl::GetBackupConfig(::grpc::ServerContext*,
                                          const backup::GetBackupConfigRequest* request,
                                          backup::GetBackupConfigResponse* response) {
    std::cout << "[BackupService] GetBackupConfig for device "
              << request->device_id() << "\n";

    // TODO: später aus DB laden, jetzt nur Dummy:
    auto* set1 = response->add_sets();
    set1->set_path("/home/USER/projects");
    set1->set_recursive(true);

    auto* set2 = response->add_sets();
    set2->set_path("/home/USER/Documents");
    set2->set_recursive(true);

    return Status::OK;
}

Status BackupServiceImpl::CheckChunks(::grpc::ServerContext*,
                                      const backup::CheckChunksRequest* request,
                                      backup::CheckChunksResponse* response) {
    std::cout << "[BackupService] CheckChunks from device "
              << request->device_id()
              << " for path=" << request->backup_set_path()
              << " count=" << request->chunk_hashes_size() << "\n";

    // TODO: hier schaust du später in deinen Chunk-Store / DB
    // Jetzt tun wir so, als ob *alle* fehlen:
    for (const auto& h : request->chunk_hashes()) {
        auto* out = response->add_missing_chunk_hashes();
        *out = h;
    }
    return Status::OK;
}

Status BackupServiceImpl::UploadChunks(::grpc::ServerContext*,
                                       ::grpc::ServerReader<backup::UploadChunksRequest>* reader,
                                       backup::UploadChunksResponse* response) {
    backup::UploadChunksRequest req;
    uint64_t count = 0;

    while (reader->Read(&req)) {
        const auto& c = req.chunk();
        std::cout << "[BackupService] UploadChunk from device " << req.device_id()
                  << " hash_size=" << c.hash().size()
                  << " payload=" << c.payload().size() << " bytes\n";

        // TODO: Chunk persistent speichern (Filesystem / DB)
        ++count;
    }

    response->set_ok(true);
    response->set_message("stored chunks");
    response->set_chunks_stored(count);
    return Status::OK;
}

Status BackupServiceImpl::CommitSnapshot(::grpc::ServerContext*,
                                         const backup::CommitSnapshotRequest* request,
                                         backup::CommitSnapshotResponse* response) {
    std::cout << "[BackupService] CommitSnapshot from device "
              << request->device_id()
              << " path=" << request->backup_set_path()
              << " files=" << request->files_size() << "\n";

    // TODO: Snapshot + FileVersions in DB schreiben
    response->set_snapshot_id("snap-123");
    response->set_message("snapshot stored");
    return Status::OK;
}

Status BackupServiceImpl::ListSnapshots(::grpc::ServerContext*,
                                        const backup::ListSnapshotsRequest* request,
                                        backup::ListSnapshotsResponse* response) {
    std::cout << "[BackupService] ListSnapshots for device "
              << request->device_id()
              << " path=" << request->backup_set_path() << "\n";

    // TODO: echte Daten aus DB
    auto* s = response->add_snapshots();
    s->set_snapshot_id("snap-123");
    s->set_type(backup::SNAPSHOT_DAILY);
    s->set_total_bytes(123456);
    s->set_file_count(42);

    return Status::OK;
}

Status BackupServiceImpl::RequestRestore(::grpc::ServerContext*,
                                         const backup::RequestRestoreRequest* request,
                                         backup::RequestRestoreResponse* response) {
    std::cout << "[BackupService] RequestRestore for snapshot "
              << request->snapshot_id()
              << " to target=" << request->target_path() << "\n";

    // TODO: Restore-Job in Queue packen
    response->set_restore_id("rest-123");
    response->set_status(backup::RESTORE_PENDING);
    response->set_message("restore scheduled");
    return Status::OK;
}

Status BackupServiceImpl::GetRestoreStatus(::grpc::ServerContext*,
                                           const backup::GetRestoreStatusRequest* request,
                                           backup::GetRestoreStatusResponse* response) {
    std::cout << "[BackupService] GetRestoreStatus for "
              << request->restore_id() << "\n";

    // TODO: echten Status aus Job-Queue / DB
    response->set_restore_id(request->restore_id());
    response->set_status(backup::RESTORE_RUNNING);
    response->set_restored_bytes(1024);
    response->set_total_bytes(2048);
    response->set_error_message("");
    return Status::OK;
}

