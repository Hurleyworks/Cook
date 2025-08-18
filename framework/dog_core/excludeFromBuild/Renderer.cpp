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
        renderContext_->cleanup();
        renderContext_.reset();
    }

    initialized_ = false;
}

void Renderer::render (const InputEvent& input, bool updateMotion, uint32_t frameNumber)
{
    LOG (DBUG) << "Renderer::render - stub implementation (frame " << frameNumber << ")";

    // Stub implementation - would perform actual rendering here
    if (!initialized_)
    {
        LOG (WARNING) << "Renderer not initialized, cannot render";
        return;
    }

    // In full implementation would:
    // 1. Process input
    // 2. Update motion if needed
    // 3. Execute rendering pipeline
    // 4. Handle frame synchronization
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
