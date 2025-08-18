#include "Renderer.h"
#include <g3log/g3log.hpp>
#include "tools/PTXManager.h"
#include "handlers/SceneHandler.h"

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
