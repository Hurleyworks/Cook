#pragma once

// Renderer is the main rendering engine for dog_core.
// Stub implementation matching engine_core/Renderer interface
// Manages pipeline selection, scene rendering, and resource coordination.

#include "../dog_core.h"
#include "RenderContext.h"
#include "DogShared.h"

using sabi::CameraHandle;

class PTXManager;     // forward declaration
class SkyDomeHandler; // forward declaration

class Renderer
{
public:
    // Constructor initializes the renderer with default settings
    Renderer();

    // Destructor ensures proper cleanup of rendering resources
    ~Renderer();

    void init(MessageService messengers, const PropertyService& properties);
    void initializeEngine(CameraHandle camera, ImageCacheHandlerPtr imageCache);
    void finalize();

    void render(const InputEvent& input, bool updateMotion, uint32_t frameNumber = 0);

    void addSkyDomeHDR(const std::filesystem::path& hdrPath);
    void addRenderableNode(RenderableWeakRef& weakNode);
    void removeRenderableNode(RenderableWeakRef& weakNode);
    void removeRenderableNodeByID(ItemID nodeID);

private:
    // Camera update methods
    void updateCameraBody(const InputEvent& input);
    void updateCameraSensor();
    
    MessageService messengers;
    PropertyService properties;

    // Render Context
    RenderContextPtr renderContext_;

    // Camera state tracking
    DogShared::PerspectiveCamera currentCamera_ = {};
    DogShared::PerspectiveCamera previousCamera_ = {};
    bool cameraChanged_ = false;
    bool restartAccumulation_ = false;
    
    // Frame tracking
    uint32_t accumulationFrame_ = 0;
    uint32_t maxAccumulationFrames_ = 1024;
    
    // Last input state for camera processing
    InputEvent lastInput_ = {};

    // Initialization state
    bool initialized_ = false;
    
   
};