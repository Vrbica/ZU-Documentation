#pragma once

#include <string>
#include <functional>
#include <atomic>

#include "EDSDK.h"

class CanonController {
public:
    // Set these before connecting
    std::string savePath;
    std::string partNumber;

    // Callbacks — set before calling initialize()
    std::function<void(const std::string& path)> onImageSaved;
    std::function<void(const std::string& msg)>  onError;
    std::function<void(const std::string& msg)>  onStatus;

    CanonController();
    ~CanonController();

    bool initialize();          // Call once at startup
    void terminate();           // Call once at shutdown

    bool connectCamera();       // Finds first Canon camera on USB
    void disconnectCamera();

    bool isConnected()  const { return m_sessionOpen; }
    bool isCapturing()  const { return m_capturing;   }
    std::string cameraName() const { return m_cameraName; }
    int  imageCount()   const { return m_imageCounter; }

    void startCapture();
    void stopCapture();

    void pumpEvents();          // Call every ~50 ms in the main loop

private:
    EdsCameraRef        m_camera       = nullptr;
    bool                m_sessionOpen  = false;
    std::atomic<bool>   m_capturing    = false;
    std::string         m_cameraName;
    int                 m_imageCounter = 0;

    void downloadImage(EdsDirectoryItemRef dirItem);

    static EdsError EDSCALLBACK onObjectEvent(EdsUInt32 event, EdsBaseRef obj,   EdsVoid* ctx);
    static EdsError EDSCALLBACK onStateEvent (EdsUInt32 event, EdsUInt32 param,  EdsVoid* ctx);
};
