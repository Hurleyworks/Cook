#include "SceneHandler.h"
#include "../RenderContext.h"
#include <g3log/g3log.hpp>

namespace dog
{

SceneHandler::SceneHandler(RenderContextPtr ctx)
    : ctx_(ctx)
{
}

SceneHandler::~SceneHandler()
{
    finalize();
}

bool SceneHandler::initialize()
{
    if (initialized_)
    {
        LOG(WARNING) << "SceneHandler already initialized";
        return true;
    }

    if (!ctx_ || !ctx_->getCudaContext())
    {
        LOG(WARNING) << "SceneHandler: Invalid render context";
        return false;
    }

    try
    {
        // Initialize slot finders for resource allocation tracking
        material_slot_finder_.initialize(maxNumMaterials);
        geom_inst_slot_finder_.initialize(maxNumGeometryInstances);
        inst_slot_finder_.initialize(maxNumInstances);
        
        LOG(DBUG) << "SceneHandler slot finders initialized:";
        LOG(DBUG) << "  Max materials: " << maxNumMaterials;
        LOG(DBUG) << "  Max geometry instances: " << maxNumGeometryInstances;
        LOG(DBUG) << "  Max instances: " << maxNumInstances;
        
        // Initialize data buffers on device
        CUcontext cuContext = ctx_->getCudaContext();
        material_data_buffer_.initialize(cuContext, cudau::BufferType::Device, maxNumMaterials);
        geom_inst_data_buffer_.initialize(cuContext, cudau::BufferType::Device, maxNumGeometryInstances);
        inst_data_buffer_[0].initialize(cuContext, cudau::BufferType::Device, maxNumInstances);
        inst_data_buffer_[1].initialize(cuContext, cudau::BufferType::Device, maxNumInstances);
        
        LOG(DBUG) << "SceneHandler data buffers initialized";
        LOG(DBUG) << "  Material buffer size: " << maxNumMaterials;
        LOG(DBUG) << "  GeomInst buffer size: " << maxNumGeometryInstances;
        LOG(DBUG) << "  Instance buffers size: " << maxNumInstances << " (double buffered)";
        
        // Create Instance Acceleration Structure using the RenderContext's scene
        optixu::Scene optixScene = ctx_->getScene();
        ias_ = optixScene.createInstanceAccelerationStructure();
        
        // Configure IAS for fast build (since we're interactive)
        ias_.setConfiguration(
            optixu::ASTradeoff::PreferFastBuild,
            optixu::AllowUpdate::Yes,
            optixu::AllowCompaction::No);
        
        LOG(DBUG) << "SceneHandler IAS created and configured";
        
        // Initialize light distribution for importance sampling
        // Using probability buffer approach (not texture)
        light_inst_dist_.initialize(cuContext, cudau::BufferType::Device, nullptr, maxNumInstances);
        LOG(DBUG) << "Light distribution initialized for " << maxNumInstances << " instances";
        
        // In an empty scene, traversable handle can be 0
        // This is a valid state for OptiX
        traversable_handle_ = 0;
        has_geometry_ = false;

        initialized_ = true;
        LOG(INFO) << "SceneHandler initialized successfully";
        return true;
    }
    catch (const std::exception& ex)
    {
        LOG(WARNING) << "Failed to initialize scene handler: " << ex.what();
        finalize();
        return false;
    }
}

void SceneHandler::finalize()
{
    if (!initialized_)
    {
        return;
    }

    // Clean up IAS and associated memory
    if (as_scratch_mem_.isInitialized())
    {
        as_scratch_mem_.finalize();
    }
    if (ias_instance_buffer_.isInitialized())
    {
        ias_instance_buffer_.finalize();
    }
    if (ias_mem_.isInitialized())
    {
        ias_mem_.finalize();
    }
    
    // Destroy IAS
    if (ias_)
    {
        ias_.destroy();
    }
    
    // Clean up data buffers
    inst_data_buffer_[1].finalize();
    inst_data_buffer_[0].finalize();
    geom_inst_data_buffer_.finalize();
    material_data_buffer_.finalize();
    
    // Clean up light distribution
    light_inst_dist_.finalize();
    
    // Clean up slot finders
    inst_slot_finder_.finalize();
    geom_inst_slot_finder_.finalize();
    material_slot_finder_.finalize();

    traversable_handle_ = 0;
    has_geometry_ = false;
    initialized_ = false;

    LOG(DBUG) << "SceneHandler finalized";
}

bool SceneHandler::buildAccelerationStructures()
{
    if (!initialized_)
    {
        LOG(WARNING) << "SceneHandler not initialized";
        return false;
    }

    try
    {
        // Check if we have any instances
        uint32_t numInstances = ias_.getNumChildren();
        
        if (numInstances == 0)
        {
            // Empty scene is valid - traversable handle stays 0
            traversable_handle_ = 0;
            has_geometry_ = false;
            LOG(DBUG) << "Empty scene - traversable handle = 0";
            return true;
        }
        
        LOG(INFO) << "Building IAS with " << numInstances << " instances";
        
        // Generate shader binding table layout for the scene
        // This is required before building acceleration structures
        optixu::Scene optixScene = ctx_->getScene();
        size_t hitGroupSbtSize;
        optixScene.generateShaderBindingTableLayout(&hitGroupSbtSize);
        
        // Get build requirements
        OptixAccelBufferSizes bufferSizes;
        ias_.prepareForBuild(&bufferSizes);
        
        // Allocate or resize scratch memory
        if (as_scratch_mem_.isInitialized())
        {
            if (bufferSizes.tempSizeInBytes > as_scratch_mem_.sizeInBytes())
            {
                as_scratch_mem_.resize(bufferSizes.tempSizeInBytes, 1);
            }
        }
        else
        {
            as_scratch_mem_.initialize(ctx_->getCudaContext(), cudau::BufferType::Device,
                                      bufferSizes.tempSizeInBytes, 1);
        }
        
        // Allocate or resize IAS memory
        if (ias_mem_.isInitialized())
        {
            if (bufferSizes.outputSizeInBytes > ias_mem_.sizeInBytes())
            {
                ias_mem_.resize(bufferSizes.outputSizeInBytes, 1);
            }
        }
        else
        {
            ias_mem_.initialize(ctx_->getCudaContext(), cudau::BufferType::Device, 
                               bufferSizes.outputSizeInBytes, 1);
        }
        
        // Allocate or resize instance buffer
        if (ias_instance_buffer_.isInitialized())
        {
            if (numInstances > ias_instance_buffer_.numElements())
            {
                ias_instance_buffer_.resize(numInstances);
            }
        }
        else
        {
            ias_instance_buffer_.initialize(ctx_->getCudaContext(), cudau::BufferType::Device, numInstances);
        }
        
        // Build the IAS
        CUstream stream = ctx_->getCudaStream();
        ias_.rebuild(stream, ias_instance_buffer_, ias_mem_, as_scratch_mem_);
        
        // Get traversable handle
        traversable_handle_ = ias_.getHandle();
        has_geometry_ = true;
        ias_needs_rebuild_ = false;
        
        LOG(INFO) << "IAS built successfully with " << numInstances << " instances";
        LOG(DBUG) << "Traversable handle: " << traversable_handle_;
        
        return true;
    }
    catch (const std::exception& ex)
    {
        LOG(WARNING) << "Failed to build acceleration structures: " << ex.what();
        return false;
    }
}

void SceneHandler::update()
{
    if (!initialized_)
    {
        LOG(WARNING) << "SceneHandler not initialized";
        return;
    }

    // Future implementation will handle:
    // - Transform updates
    // - Animation updates
    // - Dynamic object updates
    // - Visibility updates
    
    LOG(DBUG) << "Scene updated";
}

bool SceneHandler::addRenderableNode(RenderableWeakRef weakNode)
{
    if (!initialized_)
    {
        LOG(WARNING) << "SceneHandler not initialized";
        return false;
    }

    // Try to lock the weak reference
    RenderableNode node = weakNode.lock();
    if (!node)
    {
        LOG(WARNING) << "Cannot add node - weak reference is expired";
        return false;
    }

    // Get the node's unique ID
    ItemID nodeID = node->getID();
    if (nodeID == INVALID_ID)
    {
        LOG(WARNING) << "Cannot add node - invalid ID";
        return false;
    }

    // Check if node already exists
    if (node_resources_.find(nodeID) != node_resources_.end())
    {
        LOG(DBUG) << "Node " << nodeID << " already exists in scene";
        return true; // Not an error, just already present
    }

    // Get the CgModel from the node
    CgModelPtr cgModel = node->getModel();
    if (!cgModel || !cgModel->isValid())
    {
        LOG(WARNING) << "Cannot add node " << nodeID << " - no valid CgModel";
        return false;
    }

    // Create resource tracking for this node
    NodeResources resources;
    resources.node = weakNode;
    
    // Allocate instance slot
    resources.instance_slot = inst_slot_finder_.getFirstAvailableSlot();
    if (resources.instance_slot >= maxNumInstances)
    {
        LOG(WARNING) << "Cannot add node " << nodeID << " - instance slots full";
        return false;
    }
    inst_slot_finder_.setInUse(resources.instance_slot);

    // Compute hash of the geometry for caching
    resources.geometry_hash = computeGeometryHash(cgModel);
    
    // Check if we already have this geometry cached
    auto geomIt = geometry_cache_.find(resources.geometry_hash);
    GeometryGroupResources* geomGroup = nullptr;
    
    if (geomIt != geometry_cache_.end())
    {
        // Reuse existing geometry group
        geomGroup = &geomIt->second;
        geomGroup->ref_count++;
        LOG(DBUG) << "Reusing cached geometry group (hash: " << resources.geometry_hash << ", refs: " << geomGroup->ref_count << ")";
    }
    else
    {
        // Create new geometry group
        GeometryGroupResources newGeomGroup;
        if (!createGeometryGroup(cgModel, resources.geometry_hash, newGeomGroup))
        {
            LOG(WARNING) << "Failed to create geometry group for node " << nodeID;
            inst_slot_finder_.setNotInUse(resources.instance_slot);
            return false;
        }
        
        geometry_cache_[resources.geometry_hash] = std::move(newGeomGroup);
        geomGroup = &geometry_cache_[resources.geometry_hash];
        geomGroup->ref_count = 1;
        LOG(INFO) << "Created new geometry group (hash: " << resources.geometry_hash << ")";
    }
    
    // Allocate geometry instance slot
    resources.geom_inst_slot = geom_inst_slot_finder_.getFirstAvailableSlot();
    if (resources.geom_inst_slot >= maxNumGeometryInstances)
    {
        LOG(WARNING) << "Cannot add node " << nodeID << " - geometry instance slots full";
        inst_slot_finder_.setNotInUse(resources.instance_slot);
        geomGroup->ref_count--;
        return false;
    }
    geom_inst_slot_finder_.setInUse(resources.geom_inst_slot);
    
    // Create OptiX instance for this node
    if (!createNodeInstance(resources, *geomGroup))
    {
        LOG(WARNING) << "Failed to create OptiX instance for node " << nodeID;
        inst_slot_finder_.setNotInUse(resources.instance_slot);
        geom_inst_slot_finder_.setNotInUse(resources.geom_inst_slot);
        geomGroup->ref_count--;
        return false;
    }
    
    // Mark node as stored in scene handler
    node->getState().state |= sabi::PRenderableState::StoredInSceneHandler;
    
    // Store the resources
    node_resources_[nodeID] = resources;
    
    // Mark that we need to rebuild acceleration structures
    ias_needs_rebuild_ = true;
    has_geometry_ = true;

    LOG(INFO) << "Added RenderableNode " << nodeID << " to scene (slot " << resources.instance_slot << ")";
    LOG(DBUG) << "Scene now contains " << node_resources_.size() << " nodes";
    
    return true;
}

bool SceneHandler::removeRenderableNode(RenderableWeakRef weakNode)
{
    if (!initialized_)
    {
        LOG(WARNING) << "SceneHandler not initialized";
        return false;
    }

    // Try to lock the weak reference
    RenderableNode node = weakNode.lock();
    if (!node)
    {
        // Node already deleted, but we might still have resources to clean up
        // This would require tracking by weak_ptr, which we'll add later
        LOG(DBUG) << "Cannot remove node - weak reference is expired";
        return false;
    }

    ItemID nodeID = node->getID();
    return removeRenderableNodeByID(nodeID);
}

bool SceneHandler::removeRenderableNodeByID(ItemID nodeID)
{
    if (!initialized_)
    {
        LOG(WARNING) << "SceneHandler not initialized";
        return false;
    }

    auto it = node_resources_.find(nodeID);
    if (it == node_resources_.end())
    {
        LOG(DBUG) << "Node " << nodeID << " not found in scene";
        return false;
    }

    // Get the resources
    const NodeResources& resources = it->second;
    
    // Free the instance slot
    if (resources.instance_slot != UINT32_MAX)
    {
        inst_slot_finder_.setNotInUse(resources.instance_slot);
    }
    
    // Free the geometry instance slot if allocated
    if (resources.geom_inst_slot != UINT32_MAX)
    {
        geom_inst_slot_finder_.setNotInUse(resources.geom_inst_slot);
    }
    
    // Clear the stored flag if node still exists
    if (RenderableNode node = resources.node.lock())
    {
        node->getState().state &= ~sabi::PRenderableState::StoredInSceneHandler;
    }
    
    // Remove from tracking
    node_resources_.erase(it);
    
    // Mark that we need to rebuild acceleration structures
    ias_needs_rebuild_ = true;
    
    // Check if scene is now empty
    if (node_resources_.empty())
    {
        has_geometry_ = false;
    }

    LOG(INFO) << "Removed RenderableNode " << nodeID << " from scene";
    LOG(DBUG) << "Scene now contains " << node_resources_.size() << " nodes";
    
    return true;
}

void SceneHandler::testNodeManagement()
{
    LOG(INFO) << "=== Testing SceneHandler Node Management ===";
    
    // Create a test node with minimal valid data
    RenderableNode testNode = sabi::WorldItem::create();
    testNode->setName("TestNode1");
    
    // Create a simple test CgModel
    CgModelPtr testModel = sabi::CgModel::create();
    testModel->V.resize(3, 3);  // 3 vertices
    testModel->V.col(0) = Eigen::Vector3f(0, 0, 0);
    testModel->V.col(1) = Eigen::Vector3f(1, 0, 0);
    testModel->V.col(2) = Eigen::Vector3f(0, 1, 0);
    
    testModel->N.resize(3, 3);  // 3 normals
    testModel->N.col(0) = Eigen::Vector3f(0, 0, 1);
    testModel->N.col(1) = Eigen::Vector3f(0, 0, 1);
    testModel->N.col(2) = Eigen::Vector3f(0, 0, 1);
    
    // Add a surface with one triangle
    sabi::CgModelSurface surface;
    surface.vertexCount = 3;
    surface.F.resize(3, 1);  // F is the triangle indices matrix (MatrixXu = unsigned)
    surface.F(0, 0) = 0;
    surface.F(1, 0) = 1;
    surface.F(2, 0) = 2;
    testModel->S.push_back(surface);
    testModel->triCount = 1;
    
    testNode->setModel(testModel);
    
    LOG(INFO) << "Test 1: Add node to scene";
    size_t initialCount = getNodeCount();
    bool addResult = addRenderableNode(testNode);
    LOG(INFO) << "  Add result: " << (addResult ? "SUCCESS" : "FAILED");
    LOG(INFO) << "  Node count: " << initialCount << " -> " << getNodeCount();
    
    LOG(INFO) << "Test 2: Try to add same node again";
    bool addAgainResult = addRenderableNode(testNode);
    LOG(INFO) << "  Add again result: " << (addAgainResult ? "SUCCESS (already exists)" : "FAILED");
    LOG(INFO) << "  Node count: " << getNodeCount();
    
    LOG(INFO) << "Test 3: Remove node from scene";
    bool removeResult = removeRenderableNode(testNode);
    LOG(INFO) << "  Remove result: " << (removeResult ? "SUCCESS" : "FAILED");
    LOG(INFO) << "  Node count: " << getNodeCount();
    
    LOG(INFO) << "Test 4: Try to remove non-existent node";
    bool removeAgainResult = removeRenderableNode(testNode);
    LOG(INFO) << "  Remove again result: " << (!removeAgainResult ? "SUCCESS (node not found)" : "FAILED");
    
    LOG(INFO) << "Test 5: Add multiple nodes";
    RenderableNode testNode2 = sabi::WorldItem::create();
    testNode2->setName("TestNode2");
    testNode2->setModel(testModel);
    
    RenderableNode testNode3 = sabi::WorldItem::create();
    testNode3->setName("TestNode3");
    testNode3->setModel(testModel);
    
    addRenderableNode(testNode);
    addRenderableNode(testNode2);
    addRenderableNode(testNode3);
    LOG(INFO) << "  Added 3 nodes, count: " << getNodeCount();
    
    LOG(INFO) << "Test 6: Remove by ID";
    ItemID node2ID = testNode2->getID();
    bool removeByIDResult = removeRenderableNodeByID(node2ID);
    LOG(INFO) << "  Remove by ID result: " << (removeByIDResult ? "SUCCESS" : "FAILED");
    LOG(INFO) << "  Node count: " << getNodeCount();
    
    LOG(INFO) << "Test 7: Clear all nodes";
    removeRenderableNode(testNode);
    removeRenderableNode(testNode3);
    LOG(INFO) << "  Final node count: " << getNodeCount();
    LOG(INFO) << "  Has geometry: " << (hasGeometry() ? "YES" : "NO");
    
    LOG(INFO) << "=== SceneHandler Node Management Tests Complete ===";
}

size_t SceneHandler::computeGeometryHash(CgModelPtr cgModel)
{
    // Simple hash based on vertex count, triangle count, and a sample of vertex positions
    std::hash<size_t> hasher;
    size_t hash = hasher(cgModel->vertexCount());
    hash ^= hasher(cgModel->triangleCount()) << 1;
    
    // Sample a few vertices for better uniqueness
    if (cgModel->V.cols() > 0)
    {
        hash ^= hasher(cgModel->V(0, 0)) << 2;
        if (cgModel->V.cols() > 1)
            hash ^= hasher(cgModel->V(0, cgModel->V.cols() - 1)) << 3;
    }
    
    return hash;
}

bool SceneHandler::createGeometryGroup(CgModelPtr cgModel, size_t hash, GeometryGroupResources& resources)
{
    if (!cgModel || !cgModel->isValid())
    {
        LOG(WARNING) << "Invalid CgModel for geometry creation";
        return false;
    }
    
    try
    {
        CUcontext cuContext = ctx_->getCudaContext();
        optixu::Scene optixScene = ctx_->getScene();
        
        // Convert CgModel vertices to shared::Vertex format (shared by all surfaces)
        std::vector<shared::Vertex> vertices;
        vertices.reserve(cgModel->V.cols());
        
        for (int i = 0; i < cgModel->V.cols(); ++i)
        {
            shared::Vertex v;
            v.position = Point3D(cgModel->V(0, i), cgModel->V(1, i), cgModel->V(2, i));
            
            // Use normals if available, otherwise default to up vector
            if (cgModel->N.cols() > i)
            {
                v.normal = Normal3D(cgModel->N(0, i), cgModel->N(1, i), cgModel->N(2, i));
                v.normal = normalize(v.normal);
            }
            else
            {
                v.normal = Normal3D(0, 1, 0);
            }
            
            // Calculate tangent from normal
            Vector3D tangent, bitangent;
            float sign = v.normal.z >= 0 ? 1.0f : -1.0f;
            const float a = -1 / (sign + v.normal.z);
            const float b = v.normal.x * v.normal.y * a;
            tangent = Vector3D(1 + sign * v.normal.x * v.normal.x * a, sign * b, -sign * v.normal.x);
            v.texCoord0Dir = normalize(tangent);
            
            // Use texture coordinates if available
            if (cgModel->UV0.cols() > i)
            {
                v.texCoord = Point2D(cgModel->UV0(0, i), cgModel->UV0(1, i));
            }
            else
            {
                v.texCoord = Point2D(0, 0);
            }
            
            vertices.push_back(v);
            
            // Update AABB
            resources.aabb.unify(v.position);
        }
        
        // Create shared vertex buffer for all surfaces
        resources.vertex_buffer.initialize(cuContext, cudau::BufferType::Device, vertices);
        
        // Create Geometry Acceleration Structure
        resources.gas = optixScene.createGeometryAccelerationStructure();
        
        // Create a GeometryInstance for each surface (following sample pattern)
        for (size_t surfIdx = 0; surfIdx < cgModel->S.size(); ++surfIdx)
        {
            const auto& surface = cgModel->S[surfIdx];
            
            // Convert surface triangles to shared::Triangle format
            std::vector<shared::Triangle> triangles;
            triangles.reserve(surface.F.cols());
            
            for (int i = 0; i < surface.F.cols(); ++i)
            {
                shared::Triangle tri;
                tri.index0 = surface.F(0, i);
                tri.index1 = surface.F(1, i);
                tri.index2 = surface.F(2, i);
                triangles.push_back(tri);
            }
            
            if (triangles.empty())
            {
                LOG(DBUG) << "Skipping surface " << surfIdx << " - no triangles";
                continue;
            }
            
            // Create per-surface resources
            GeometryInstanceResources geomInstRes;
            
            // Each surface gets its own triangle buffer
            geomInstRes.triangle_buffer.initialize(cuContext, cudau::BufferType::Device, triangles);
            
            // TODO: Allocate material slot based on surface.material or surface.cgMaterial
            geomInstRes.material_slot = 0;  // Default material for now
            
            // Create OptiX geometry instance for this surface
            geomInstRes.optix_geom_inst = optixScene.createGeometryInstance();
            geomInstRes.optix_geom_inst.setVertexBuffer(resources.vertex_buffer);  // Use shared vertex buffer
            geomInstRes.optix_geom_inst.setTriangleBuffer(geomInstRes.triangle_buffer);
            geomInstRes.optix_geom_inst.setNumMaterials(1, optixu::BufferView());
            geomInstRes.optix_geom_inst.setMaterial(0, 0, ctx_->getDefaultMaterial());  // Set default material
            geomInstRes.optix_geom_inst.setUserData(surfIdx);  // Store surface index
            
            // Add to GAS
            resources.gas.addChild(geomInstRes.optix_geom_inst);
            
            // Store in geometry group
            resources.geom_instances.push_back(std::move(geomInstRes));
            
            LOG(DBUG) << "Created GeometryInstance for surface " << surfIdx 
                      << " (" << triangles.size() << " triangles)";
        }
        
        if (resources.geom_instances.empty())
        {
            LOG(WARNING) << "No valid surfaces found in CgModel";
            return false;
        }
        
        LOG(INFO) << "Creating GeometryGroup with " << resources.geom_instances.size() 
                  << " surfaces, " << vertices.size() << " vertices total";
        
        // Configure and build GAS
        resources.gas.setNumMaterialSets(1);
        resources.gas.setNumRayTypes(0, 1);  // Single ray type for now
        resources.gas.setConfiguration(
            optixu::ASTradeoff::PreferFastBuild,
            optixu::AllowUpdate::No,
            optixu::AllowCompaction::No);
        
        // Prepare and build GAS
        OptixAccelBufferSizes gasSizes;
        resources.gas.prepareForBuild(&gasSizes);
        
        resources.gas_mem.initialize(cuContext, cudau::BufferType::Device, gasSizes.outputSizeInBytes, 1);
        
        cudau::Buffer gasScratch;
        gasScratch.initialize(cuContext, cudau::BufferType::Device, gasSizes.tempSizeInBytes, 1);
        
        CUstream stream = ctx_->getCudaStream();
        resources.gas.rebuild(stream, resources.gas_mem, gasScratch);
        
        gasScratch.finalize();
        
        LOG(INFO) << "Created GAS with handle: " << resources.gas.getHandle();
        
        return true;
    }
    catch (const std::exception& ex)
    {
        LOG(WARNING) << "Failed to create geometry resources: " << ex.what();
        return false;
    }
}

bool SceneHandler::createNodeInstance(NodeResources& nodeRes, const GeometryGroupResources& geomGroup)
{
    try
    {
        // Get node's transform
        RenderableNode node = nodeRes.node.lock();
        if (!node)
        {
            LOG(WARNING) << "Node expired while creating instance";
            return false;
        }
        
        // Get transform from SpaceTime
        const sabi::SpaceTime& spacetime = node->getSpaceTime();
        const Eigen::Matrix4f& worldTransform = spacetime.worldTransform.matrix();
        
        // Convert Eigen matrix to OptiX format (row-major float[12])
        float transform[12];
        for (int row = 0; row < 3; ++row)
        {
            for (int col = 0; col < 4; ++col)
            {
                transform[row * 4 + col] = worldTransform(row, col);
            }
        }
        
        // Create OptiX instance pointing to the GeometryGroup's GAS
        OptixInstance optixInstance = {};
        std::memcpy(optixInstance.transform, transform, sizeof(transform));
        optixInstance.instanceId = nodeRes.instance_slot;
        optixInstance.sbtOffset = 0;  // Will be set when we have multiple materials
        optixInstance.visibilityMask = 0xFF;  // Visible to all ray types
        optixInstance.flags = OPTIX_INSTANCE_FLAG_NONE;
        optixInstance.traversableHandle = geomGroup.gas.getHandle();
        
        // Add instance to IAS
        if (!ias_instance_buffer_.isInitialized() || 
            ias_instance_buffer_.numElements() <= ias_.getNumChildren())
        {
            // Resize instance buffer
            uint32_t newSize = std::max(16u, (uint32_t)(ias_.getNumChildren() * 2));
            if (ias_instance_buffer_.isInitialized())
            {
                ias_instance_buffer_.resize(newSize);
            }
            else
            {
                ias_instance_buffer_.initialize(ctx_->getCudaContext(), cudau::BufferType::Device, newSize);
            }
        }
        
        // Store instance in buffer
        nodeRes.optix_instance_index = ias_.getNumChildren();
        
        // Create optixu::Instance from the GAS handle and transform
        optixu::Scene optixScene = ctx_->getScene();
        optixu::Instance instance = optixScene.createInstance();
        instance.setChild(geomGroup.gas);
        instance.setTransform(transform);
        
        // Add to IAS
        ias_.addChild(instance);
        
        // Update geometry instance data buffer for each surface in the geometry group
        // For now, we'll use the first surface's buffers as representative
        // In the future, we may need to handle multiple geom instances per node
        if (!geomGroup.geom_instances.empty())
        {
            const auto& firstGeomInst = geomGroup.geom_instances[0];
            
            // Map the buffer to host memory
            geom_inst_data_buffer_.map();
            shared::GeometryInstanceData* geomInstDataHost = geom_inst_data_buffer_.getMappedPointer();
            shared::GeometryInstanceData& geomInstData = geomInstDataHost[nodeRes.geom_inst_slot];
            
            geomInstData.vertexBuffer = geomGroup.vertex_buffer.getROBuffer<shared::enableBufferOobCheck>();
            geomInstData.triangleBuffer = firstGeomInst.triangle_buffer.getROBuffer<shared::enableBufferOobCheck>();
            geomInstData.materialSlot = firstGeomInst.material_slot;
            geomInstData.geomInstSlot = nodeRes.geom_inst_slot;
            
            // Unmap the buffer
            geom_inst_data_buffer_.unmap();
        }
        
        // Update instance data buffer
        inst_data_buffer_[0].map();
        shared::InstanceData* instDataHost = inst_data_buffer_[0].getMappedPointer();
        shared::InstanceData& instData = instDataHost[nodeRes.instance_slot];
        
        instData.transform = Matrix4x4(
            Vector4D(transform[0], transform[1], transform[2], transform[3]),
            Vector4D(transform[4], transform[5], transform[6], transform[7]),
            Vector4D(transform[8], transform[9], transform[10], transform[11]),
            Vector4D(0, 0, 0, 1));
        instData.curToPrevTransform = instData.transform;  // No motion blur yet
        instData.normalMatrix = instData.transform.getUpperLeftMatrix().invert().transpose();
        instData.uniformScale = 1.0f;  // Assuming uniform scale for now
        
        // For now, single geometry instance per instance
        // In future, could support multiple geom instances per instance
        instData.isEmissive = nodeRes.is_emissive ? 1 : 0;
        instData.emissiveScale = 1.0f;
        
        inst_data_buffer_[0].unmap();
        
        LOG(DBUG) << "Created OptiX instance " << nodeRes.optix_instance_index << " for node (transform: " 
                  << transform[3] << ", " << transform[7] << ", " << transform[11] << ")";
        
        return true;
    }
    catch (const std::exception& ex)
    {
        LOG(WARNING) << "Failed to create node instance: " << ex.what();
        return false;
    }
}

} // namespace dog