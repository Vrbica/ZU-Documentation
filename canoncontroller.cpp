#include "canoncontroller.h"

#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
CanonController::CanonController() = default;

CanonController::~CanonController() {
    disconnectCamera();
    terminate();
}

// ---------------------------------------------------------------------------
bool CanonController::initialize() {
    EdsError err = EdsInitializeSDK();
    if (err != EDS_ERR_OK) {
        if (onError) onError("EdsInitializeSDK failed (code " + std::to_string(err) + ")");
        return false;
    }
    return true;
}

void CanonController::terminate() {
    EdsTerminateSDK();
}

// ---------------------------------------------------------------------------
bool CanonController::connectCamera() {
    EdsCameraListRef cameraList = nullptr;
    EdsError err = EdsGetCameraList(&cameraList);
    if (err != EDS_ERR_OK) {
        if (onError) onError("EdsGetCameraList failed (code " + std::to_string(err) + ")");
        return false;
    }

    EdsUInt32 count = 0;
    EdsGetChildCount(cameraList, &count);
    if (count == 0) {
        EdsRelease(cameraList);
        if (onError) onError("No Canon camera found — connect via USB and try again.");
        return false;
    }

    EdsGetChildAtIndex(cameraList, 0, &m_camera);
    EdsRelease(cameraList);

    EdsDeviceInfo info{};
    EdsGetDeviceInfo(m_camera, &info);
    m_cameraName = info.szDeviceDescription;

    err = EdsOpenSession(m_camera);
    if (err != EDS_ERR_OK) {
        EdsRelease(m_camera);
        m_camera = nullptr;
        if (onError) onError("EdsOpenSession failed (code " + std::to_string(err) + ")");
        return false;
    }

    m_sessionOpen = true;

    EdsSetObjectEventHandler   (m_camera, kEdsObjectEvent_All,  onObjectEvent, this);
    EdsSetCameraStateEventHandler(m_camera, kEdsStateEvent_All, onStateEvent,  this);

    if (onStatus) onStatus("Connected: " + m_cameraName);
    return true;
}

void CanonController::disconnectCamera() {
    if (!m_sessionOpen) return;
    stopCapture();
    EdsCloseSession(m_camera);
    EdsRelease(m_camera);
    m_camera      = nullptr;
    m_sessionOpen = false;
    m_cameraName.clear();
}

// ---------------------------------------------------------------------------
void CanonController::startCapture() {
    if (!m_sessionOpen) return;

    // Tell camera to save images to both the card and the PC
    EdsUInt32 saveTo = kEdsSaveTo_Both;
    EdsSetPropertyData(m_camera, kEdsPropID_SaveTo, 0, sizeof(saveTo), &saveTo);

    // Report "unlimited" space on the PC side
    EdsCapacity cap{ 0x7FFFFFFF, 0x1000, 1 };
    EdsSetCapacity(m_camera, cap);

    m_imageCounter = 0;
    m_capturing    = true;

    if (onStatus) onStatus("Capture started — take photos with the camera.");
}

void CanonController::stopCapture() {
    if (!m_capturing) return;
    m_capturing = false;
    if (onStatus)
        onStatus("Capture stopped. " + std::to_string(m_imageCounter) + " image(s) saved.");
}

bool CanonController::triggerCapture() {
    if (!m_sessionOpen || !m_capturing) return false;
    EdsError err = EdsSendCommand(m_camera, kEdsCameraCommand_TakePicture, 0);
    if (err != EDS_ERR_OK) {
        if (onError) onError("Shutter trigger failed (code " + std::to_string(err) + ")");
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
bool CanonController::startLiveView()
{
    if (!m_sessionOpen || m_liveViewActive) return false;

    // Enable EVF mode on the camera body
    EdsUInt32 evfMode = 1;
    EdsError err = EdsSetPropertyData(m_camera, kEdsPropID_Evf_Mode,
                                      0, sizeof(evfMode), &evfMode);
    if (err != EDS_ERR_OK) {
        if (onError) onError("Cannot enable Live View mode (code " + std::to_string(err) + ")");
        return false;
    }

    // Route the EVF stream to the PC (keep camera screen on as well)
    EdsUInt32 device = kEdsEvfOutputDevice_PC | kEdsEvfOutputDevice_TFT;
    err = EdsSetPropertyData(m_camera, kEdsPropID_Evf_OutputDevice,
                             0, sizeof(device), &device);
    if (err != EDS_ERR_OK) {
        if (onError) onError("Cannot set EVF output device (code " + std::to_string(err) + ")");
        return false;
    }

    m_liveViewActive = true;
    if (onStatus) onStatus("Live View started.");
    return true;
}

void CanonController::stopLiveView()
{
    if (!m_sessionOpen || !m_liveViewActive) return;

    // Route stream back to camera screen only
    EdsUInt32 device = kEdsEvfOutputDevice_TFT;
    EdsSetPropertyData(m_camera, kEdsPropID_Evf_OutputDevice,
                       0, sizeof(device), &device);

    // Disable EVF mode
    EdsUInt32 evfMode = 0;
    EdsSetPropertyData(m_camera, kEdsPropID_Evf_Mode,
                       0, sizeof(evfMode), &evfMode);

    m_liveViewActive = false;
    if (onStatus) onStatus("Live View stopped.");
}

void CanonController::grabLiveViewFrame()
{
    if (!m_sessionOpen || !m_liveViewActive) return;

    // Create an in-memory stream to receive the JPEG frame
    EdsStreamRef memStream = nullptr;
    EdsError err = EdsCreateMemoryStream(0, &memStream);
    if (err != EDS_ERR_OK) return;

    // Create EVF image ref and download
    EdsEvfImageRef evfImage = nullptr;
    err = EdsCreateEvfImageRef(memStream, &evfImage);
    if (err != EDS_ERR_OK) {
        EdsRelease(memStream);
        return;
    }

    err = EdsDownloadEvfImage(m_camera, evfImage);
    EdsRelease(evfImage);

    if (err == EDS_ERR_OK) {
        // Copy JPEG bytes out of the stream
        EdsUInt64 length = 0;
        EdsGetLength(memStream, &length);

        if (length > 0 && onLiveViewFrame) {
            EdsVoid* ptr = nullptr;
            EdsGetPointer(memStream, &ptr);
            std::vector<uint8_t> jpeg(
                reinterpret_cast<uint8_t*>(ptr),
                reinterpret_cast<uint8_t*>(ptr) + static_cast<size_t>(length));
            onLiveViewFrame(jpeg);
        }
    }

    EdsRelease(memStream);
}

// ---------------------------------------------------------------------------
void CanonController::pumpEvents() {
    EdsGetEvent();
}

// ---------------------------------------------------------------------------
void CanonController::downloadImage(EdsDirectoryItemRef dirItem) {
    EdsDirectoryItemInfo itemInfo{};
    EdsGetDirectoryItemInfo(dirItem, &itemInfo);

    // Build extension from original filename
    std::string origName(itemInfo.szFileName);
    std::string ext = ".JPG";
    auto dot = origName.rfind('.');
    if (dot != std::string::npos) {
        ext = origName.substr(dot);
        for (auto& c : ext) c = (char)toupper((unsigned char)c);
    }

    // Timestamp
    auto now = std::chrono::system_clock::now();
    auto t   = std::chrono::system_clock::to_time_t(now);
    auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(
                   now.time_since_epoch()) % 1000;
    std::tm tm{};
    localtime_s(&tm, &t);
    std::ostringstream ts;
    ts << std::put_time(&tm, "%Y%m%d_%H%M%S")
       << "_" << std::setfill('0') << std::setw(3) << ms.count();

    std::string filename = partNumber + "_" + ts.str() + ext;
    fs::path subDir = fs::path(savePath) / partNumber;
    std::string fullPath = (subDir / filename).string();

    fs::create_directories(subDir);

    EdsStreamRef stream = nullptr;
    EdsError err = EdsCreateFileStream(
        fullPath.c_str(),
        kEdsFileCreateDisposition_CreateAlways,
        kEdsAccess_ReadWrite,
        &stream);

    if (err != EDS_ERR_OK) {
        EdsRelease(dirItem);
        if (onError) onError("Cannot create file: " + fullPath);
        return;
    }

    err = EdsDownload(dirItem, itemInfo.size, stream);
    if (err != EDS_ERR_OK) {
        EdsDownloadCancel(dirItem);
        EdsRelease(stream);
        EdsRelease(dirItem);
        if (onError) onError("EdsDownload failed (code " + std::to_string(err) + ")");
        return;
    }

    EdsDownloadComplete(dirItem);
    EdsRelease(stream);
    EdsRelease(dirItem);

    ++m_imageCounter;
    if (onImageSaved) onImageSaved(fullPath);
}

// ---------------------------------------------------------------------------
EdsError EDSCALLBACK CanonController::onObjectEvent(
    EdsUInt32 event, EdsBaseRef obj, EdsVoid* ctx)
{
    auto* self = static_cast<CanonController*>(ctx);

    if (event == kEdsObjectEvent_DirItemCreated && self->m_capturing) {
        self->downloadImage(static_cast<EdsDirectoryItemRef>(obj));
    } else if (obj) {
        EdsRelease(obj);
    }
    return EDS_ERR_OK;
}

EdsError EDSCALLBACK CanonController::onStateEvent(
    EdsUInt32 event, EdsUInt32 /*param*/, EdsVoid* ctx)
{
    auto* self = static_cast<CanonController*>(ctx);
    if (event == kEdsStateEvent_Shutdown) {
        self->m_sessionOpen = false;
        self->m_capturing   = false;
        self->m_camera      = nullptr;
        if (self->onStatus) self->onStatus("Camera was disconnected.");
    }
    return EDS_ERR_OK;
}
