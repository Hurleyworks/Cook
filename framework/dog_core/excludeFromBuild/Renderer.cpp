#include "Renderer.h"
#include "tools/PTXManager.h"
#include "handlers/SceneHandler.h"
#include "nvcc/CudaCompiler.h"

Renderer::Renderer()
{
    LOG (DBUG) << "Renderer constructor";
}

Renderer::~Renderer()
{
    LOG (DBUG) << "Renderer destructor";
    finalize();
}

void Renderer::init (MessageService messengers, const PropertyService& properties)
{
    LOG (INFO) << "Renderer::init - stub implementation";

    this->messengers = messengers;
    this->properties = properties;
}

void Renderer::initializeEngine (CameraHandle camera, ImageCacheHandlerPtr imageCache)
{
    LOG (INFO) << "Renderer::initializeEngine - stub implementation";



    std::filesystem::path resourceFolder = properties.renderProps->getVal<std::string>(RenderKey::ResourceFolder);
    std::filesystem::path repoFolder = properties.renderProps->getVal<std::string>(RenderKey::RepoFolder);

    // Check build configuration for CUDA kernel compilation strategy
    bool softwareReleaseMode = properties.renderProps->getVal<bool>(RenderKey::SoftwareReleaseMode);
    bool embeddedPTX = properties.renderProps->getVal<bool>(RenderKey::UseEmbeddedPTX);

    // NB: Determine whether to compile CUDA kernels at runtime
    // Development workflow:
    // 1. Change compileCuda to true to force recompilation when CUDA source changes
    // 2. Run application once to compile fresh PTX files
    // 3. Run ptx_embed.bat to update generated/embedded_ptx.h with new binaries
    // 4. Restore original condition and rebuild application
    // 5. Application will then use the embedded PTX files

    bool compileCuda = true;
    std::string engineFilter = "all";  // Can be "all", "milo", or "shocker"

    if (compileCuda)
    {
        CudaCompiler nvcc;

        // Define the architectures to compile for
        std::vector<std::string> targetArchitectures;

        // Try to load from properties if available
        try
        {
            std::string archList = properties.renderProps->getVal<std::string>(RenderKey::CudaTargetArchitectures);
            if (!archList.empty())
            {
                // Parse comma-separated list of architectures
                size_t pos = 0;
                while ((pos = archList.find(',')) != std::string::npos)
                {
                    targetArchitectures.push_back(archList.substr(0, pos));
                    archList.erase(0, pos + 1);
                }
                if (!archList.empty())
                {
                    targetArchitectures.push_back(archList);
                }
            }
        }
        catch (...)
        {
            // Property not found, using defaults
        }

        // If no architectures specified in properties, use defaults
        if (targetArchitectures.empty())
        {
            targetArchitectures = { "sm_60", "sm_75", "sm_80", "sm_86", "sm_90" };
        }

        // Log which architectures we're compiling for
        LOG(DBUG) << "Compiling CUDA kernels for the following architectures:";
        for (const auto& arch : targetArchitectures)
        {
            LOG(DBUG) << "  - " << arch;
        }

        // Compile for all target architectures with engine filter
        nvcc.compile(resourceFolder, repoFolder, targetArchitectures);
    }

    // Create render context
    renderContext_ = RenderContext::create();
    if (renderContext_)
    {
        renderContext_->setCamera (camera);

        // Initialize with default device 0 and pass the image cache
        if (renderContext_->initialize (0, imageCache))
        {
            initialized_ = true;
            LOG (INFO) << "Renderer initialized with RenderContext";
        }
        else
        {
            LOG (WARNING) << "Failed to initialize RenderContext";
            renderContext_.reset();
        }
    }
    else
    {
        LOG (WARNING) << "Cannot initialize Renderer - RenderContext is null";
    }
}

void Renderer::finalize()
{
    LOG (INFO) << "Renderer::finalize";

    if (renderContext_)
    {
        // Wait for all GPU work to complete before cleanup
        renderContext_->waitAllStreamsComplete();
        LOG (DBUG) << "All streams synchronized for shutdown";
        
        renderContext_->cleanup();
        renderContext_.reset();
    }

    initialized_ = false;
}

void Renderer::render (const InputEvent& input, bool updateMotion, uint32_t frameNumber)
{
    LOG (DBUG) << "Renderer::render - frame " << frameNumber;

    if (!initialized_ || !renderContext_)
    {
        LOG (WARNING) << "Renderer not initialized, cannot render";
        return;
    }

    // Get current stream from the StreamChain (waits for previous frame if needed)
    CUstream currentStream = renderContext_->getCurrentStream();
    
    // Determine buffer index for double buffering
    uint32_t bufferIndex = frameNumber % 2;
    
    // Phase 1: Process input and update camera
    updateCameraBody(input);
    
    // Check if we need to restart accumulation due to camera changes
    if (cameraChanged_ || restartAccumulation_)
    {
        accumulationFrame_ = 0;
        cameraChanged_ = false;
        restartAccumulation_ = false;
        LOG (DBUG) << "  Restarting accumulation due to camera change";
    }
    else if (accumulationFrame_ < maxAccumulationFrames_)
    {
        accumulationFrame_++;
    }
    
    // Phase 2: Scene update (if motion is enabled)
    if (updateMotion)
    {
        // Reset accumulation when scene is animating
        accumulationFrame_ = 0;
        
        // TODO: Update animated objects
        // TODO: Rebuild acceleration structures if needed
        LOG (DBUG) << "  Updating motion for frame " << frameNumber;
    }
    
    // Phase 3: Update pipeline parameters
    // TODO: Update per-frame uniforms with camera data
    // TODO: Set accumulation count
    // Example of what will be done:
    // per_frame_params.camera = currentCamera_;
    // per_frame_params.prevCamera = previousCamera_;
    // per_frame_params.accumFrame = accumulationFrame_;
    // per_frame_params.bufferIndex = bufferIndex;
    
    // Phase 4: Rendering stages (all on current stream)
    // TODO: G-buffer generation
    // TODO: Path tracing kernel launch
    // TODO: Post-processing
    
    LOG (DBUG) << "  Rendering on stream for buffer " << bufferIndex 
               << ", accumulation frame " << accumulationFrame_;
    
    // Phase 5: Update camera sensor (if applicable, e.g., LightWave integration)
    updateCameraSensor();
    
    // Phase 6: Frame finalization
    // Swap streams for next frame (records end event on current stream)
    renderContext_->swapStreams();
    
    // Note: No need to synchronize here - next frame will wait automatically
    // Only synchronize when absolutely necessary (e.g., screenshots, shutdown)
}

void Renderer::addSkyDomeHDR (const std::filesystem::path& hdrPath)
{
    LOG (INFO) << "Renderer::addSkyDomeHDR - stub implementation: " << hdrPath.string();

    // Stub implementation - would load HDR and set up sky dome lighting
}

void Renderer::addRenderableNode (RenderableWeakRef& weakNode)
{
    LOG (DBUG) << "Renderer::addRenderableNode";

    if (!initialized_ || !renderContext_)
    {
        LOG (WARNING) << "Renderer not initialized, cannot add node";
        return;
    }

    // Get the handlers from render context
    dog::Handlers* handlers = renderContext_->getHandlers();
    if (!handlers || !handlers->scene)
    {
        LOG (WARNING) << "SceneHandler not available";
        return;
    }

    // Try to lock the weak reference to check node info
    if (RenderableNode node = weakNode.lock())
    {
        LOG (INFO) << "Adding RenderableNode: " << node->getName() << " (ID: " << node->getID() << ")";
        
        // Check if node has valid geometry
        CgModelPtr cgModel = node->getModel();
        if (cgModel)
        {
            LOG (DBUG) << "  Vertices: " << cgModel->vertexCount() << ", Triangles: " << cgModel->triangleCount();
        }
        else
        {
            LOG (WARNING) << "  Node has no CgModel geometry";
        }
    }

    // Add the node to the scene handler
    bool success = handlers->scene->addRenderableNode(weakNode);
    
    if (success)
    {
        LOG (INFO) << "Node successfully added to SceneHandler";
        LOG (INFO) << "Scene now contains " << handlers->scene->getNodeCount() << " nodes";
        
        // Rebuild acceleration structures if needed
        if (handlers->scene->hasGeometry())
        {
            LOG (DBUG) << "Building acceleration structures...";
            handlers->scene->buildAccelerationStructures();
        }
    }
    else
    {
        LOG (WARNING) << "Failed to add node to SceneHandler";
    }
}

void Renderer::removeRenderableNode (RenderableWeakRef& weakNode)
{
    LOG (DBUG) << "Renderer::removeRenderableNode";

    if (!initialized_ || !renderContext_)
    {
        LOG (WARNING) << "Renderer not initialized, cannot remove node";
        return;
    }

    // Get the handlers from render context
    dog::Handlers* handlers = renderContext_->getHandlers();
    if (!handlers || !handlers->scene)
    {
        LOG (WARNING) << "SceneHandler not available";
        return;
    }

    // Try to lock the weak reference to check node info
    if (RenderableNode node = weakNode.lock())
    {
        LOG (INFO) << "Removing RenderableNode: " << node->getName() << " (ID: " << node->getID() << ")";
    }

    // Remove the node from the scene handler
    bool success = handlers->scene->removeRenderableNode(weakNode);
    
    if (success)
    {
        LOG (INFO) << "Node successfully removed from SceneHandler";
        LOG (INFO) << "Scene now contains " << handlers->scene->getNodeCount() << " nodes";
        
        // Rebuild acceleration structures if still have geometry
        if (handlers->scene->hasGeometry())
        {
            LOG (DBUG) << "Rebuilding acceleration structures...";
            handlers->scene->buildAccelerationStructures();
        }
    }
    else
    {
        LOG (DBUG) << "Node was not in SceneHandler";
    }
}

void Renderer::removeRenderableNodeByID (ItemID nodeID)
{
    LOG (DBUG) << "Renderer::removeRenderableNodeByID: " << nodeID;

    if (!initialized_ || !renderContext_)
    {
        LOG (WARNING) << "Renderer not initialized, cannot remove node";
        return;
    }

    // Get the handlers from render context
    dog::Handlers* handlers = renderContext_->getHandlers();
    if (!handlers || !handlers->scene)
    {
        LOG (WARNING) << "SceneHandler not available";
        return;
    }

    // Remove the node from the scene handler
    bool success = handlers->scene->removeRenderableNodeByID(nodeID);
    
    if (success)
    {
        LOG (INFO) << "Node " << nodeID << " successfully removed from SceneHandler";
        LOG (INFO) << "Scene now contains " << handlers->scene->getNodeCount() << " nodes";
        
        // Rebuild acceleration structures if still have geometry
        if (handlers->scene->hasGeometry())
        {
            LOG (DBUG) << "Rebuilding acceleration structures...";
            handlers->scene->buildAccelerationStructures();
        }
    }
    else
    {
        LOG (DBUG) << "Node " << nodeID << " was not in SceneHandler";
    }
}

void Renderer::updateCameraBody (const InputEvent& input)
{
    if (!renderContext_)
    {
        return;
    }

    // Store last input
    lastInput_ = input;

    // Save previous camera for temporal effects
    previousCamera_ = currentCamera_;

    // Get camera from render context
    auto camera = renderContext_->getCamera();
    if (!camera)
    {
        LOG (WARNING) << "No camera available";
        return;
    }

    // Check if camera has changed
    if (camera->isDirty() || camera->hasSettingsChanged())
    {
        cameraChanged_ = true;
        restartAccumulation_ = true;

        // Update camera parameters
        sabi::CameraSensor* sensor = camera->getSensor();
        if (sensor)
        {
            currentCamera_.aspect = sensor->getPixelAspectRatio();
        }
        else
        {
            // Use render dimensions from context
            int renderWidth = renderContext_->getRenderWidth();
            int renderHeight = renderContext_->getRenderHeight();
            currentCamera_.aspect = static_cast<float> (renderWidth) / static_cast<float> (renderHeight);
        }
        currentCamera_.fovY = camera->getVerticalFOVradians();

        // Get camera position
        Eigen::Vector3f eyePoint = camera->getEyePoint();
        currentCamera_.position = Point3D (eyePoint.x(), eyePoint.y(), eyePoint.z());

        // Build camera orientation matrix from camera vectors
        Eigen::Vector3f right = camera->getRight();
        const Eigen::Vector3f& up = camera->getUp();
        const Eigen::Vector3f& forward = camera->getFoward();

        // Fix for standalone applications - negate right vector to correct trackball rotation
        // (not needed in LightWave but required in standalone)
        right *= -1.0f;

        // Convert to shared types
        Vector3D camRight (right.x(), right.y(), right.z());
        Vector3D camUp (up.x(), up.y(), up.z());
        Vector3D camForward (forward.x(), forward.y(), forward.z());

        // Build orientation matrix from camera basis vectors
        // Using the same constructor as production code: Matrix3x3(right, up, forward)
        currentCamera_.orientation = Matrix3x3 (camRight, camUp, camForward);

        // Set lens parameters (depth of field)
        if (properties.renderProps)
        {
            // Note: These would be used for depth of field effects
            // currentCamera_.lensSize = properties.renderProps->getValOr<float> (RenderKey::Aperture, 0.0f);
            // currentCamera_.focusDistance = properties.renderProps->getValOr<float> (RenderKey::FocalLength, 5.0f);
        }

        // Mark camera as not dirty after processing
        camera->setDirty (false);
        
        LOG (DBUG) << "Camera updated - position: (" 
                   << currentCamera_.position.x << ", " 
                   << currentCamera_.position.y << ", " 
                   << currentCamera_.position.z << ")";
    }
}

void Renderer::updateCameraSensor()
{
    // Get camera from render context
    if (!renderContext_)
    {
        LOG (WARNING) << "No render context available for camera sensor update";
        return;
    }

    auto camera = renderContext_->getCamera();
    if (!camera)
    {
        LOG (WARNING) << "No camera available for sensor update";
        return;
    }

    // TODO: When we have actual render buffers, we'll update the sensor here
    // For now this is a stub that will be filled in when we have:
    // 1. Linear beauty buffer from rendering
    // 2. Render handler with buffer management
    
    // Placeholder for future implementation:
    /*
    // Get the linear beauty buffer from render handler
    if (!renderHandler_ || !renderHandler_->isInitialized())
    {
        LOG (WARNING) << "RenderHandler not available or not initialized";
        return;
    }

    auto& linearBeautyBuffer = renderHandler_->getLinearBeautyBuffer();
    
    // Copy device buffer to host
    int renderWidth = renderContext_->getRenderWidth();
    int renderHeight = renderContext_->getRenderHeight();
    std::vector<float4> hostPixels (renderWidth * renderHeight);
    linearBeautyBuffer.read (hostPixels.data(), renderWidth * renderHeight);

    // Update the camera sensor with the rendered image
    bool previewMode = false; // Full quality display
    uint32_t renderScale = 1; // No scaling

    Eigen::Vector2i renderSize (renderWidth, renderHeight);
    bool success = camera->getSensor()->updateImage (hostPixels.data(), renderSize, previewMode, renderScale);

    if (!success)
    {
        LOG (WARNING) << "Failed to update camera sensor with rendered image";
    }
    */
}
