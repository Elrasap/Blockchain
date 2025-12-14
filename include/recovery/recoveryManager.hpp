#pragma once

class RecoveryManager {
public:
    void onStartupCheck();
    void recoverFromSnapshot();
    void replayWAL();
    void resyncWithLeader();
    void verifyState();
};
