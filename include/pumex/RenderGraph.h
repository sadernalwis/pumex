//
// Copyright(c) 2017-2018 Paweł Księżopolski ( pumexx )
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#pragma once
#include <vector>
#include <set>
#include <map>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#if defined(GLM_ENABLE_EXPERIMENTAL) // hack around redundant GLM_ENABLE_EXPERIMENTAL defined in type.hpp
  #undef GLM_ENABLE_EXPERIMENTAL
  #define GLM_ENABLE_EXPERIMENTAL_HACK
#endif
#include <gli/texture.hpp>
#if defined(GLM_ENABLE_EXPERIMENTAL_HACK)
  #define GLM_ENABLE_EXPERIMENTAL
  #undef GLM_ENABLE_EXPERIMENTAL_HACK
#endif
#include <pumex/Export.h>
#include <pumex/ResourceRange.h>

namespace pumex
{

class Node;

struct PUMEX_EXPORT LoadOp
{
  enum Type { Load, Clear, DontCare };
  LoadOp()
    : loadType{ DontCare }, clearColor{}
  {
  }
  LoadOp(Type lType, const glm::vec4& color)
    : loadType{ lType }, clearColor{ color }
  {
  }

  Type loadType;
  glm::vec4 clearColor;
};

inline LoadOp loadOpLoad();
inline LoadOp loadOpClear(const glm::vec2& color = glm::vec2(1.0f, 0.0f));
inline LoadOp loadOpClear(const glm::vec4& color = glm::vec4(0.0f));
inline LoadOp loadOpDontCare();

struct PUMEX_EXPORT StoreOp
{
  enum Type { Store, DontCare };
  StoreOp()
    : storeType{ DontCare }
  {
  }
  StoreOp(Type sType)
    : storeType{ sType }
  {
  }
  Type storeType;
};

inline StoreOp storeOpStore();
inline StoreOp storeOpDontCare();

enum AttachmentType   { atUndefined, atColor, atDepth, atDepthStencil, atStencil };
enum ResourceMetaType { rmtUndefined, rmtImage, rmtBuffer };
enum OperationType    { opGraphics = VK_QUEUE_GRAPHICS_BIT, opCompute = VK_QUEUE_COMPUTE_BIT };
enum OperationEntryType
{
  opeAttachmentInput         = 1,
  opeAttachmentOutput        = 2,
  opeAttachmentResolveOutput = 4,
  opeAttachmentDepthOutput   = 8,
  opeAttachmentDepthInput    = 16,
  opeBufferInput             = 32,
  opeBufferOutput            = 64,
  opeImageInput              = 128,
  opeImageOutput             = 256
};
typedef VkFlags OperationEntryTypeFlags;

const OperationEntryTypeFlags opeAllAttachments       = opeAttachmentInput | opeAttachmentOutput | opeAttachmentResolveOutput | opeAttachmentDepthInput | opeAttachmentDepthOutput;
const OperationEntryTypeFlags opeAllImages            = opeImageInput | opeImageOutput;
const OperationEntryTypeFlags opeAllBuffers           = opeBufferInput | opeBufferOutput;
const OperationEntryTypeFlags opeAllAttachmentInputs  = opeAttachmentInput | opeAttachmentDepthInput;
const OperationEntryTypeFlags opeAllAttachmentOutputs = opeAttachmentOutput | opeAttachmentResolveOutput | opeAttachmentDepthOutput;
const OperationEntryTypeFlags opeAllInputs            = opeAttachmentInput | opeAttachmentDepthInput | opeBufferInput | opeImageInput;
const OperationEntryTypeFlags opeAllOutputs           = opeAttachmentOutput | opeAttachmentResolveOutput | opeAttachmentDepthOutput | opeBufferOutput | opeImageOutput;

const OperationEntryTypeFlags opeAllInputsOutputs     = opeAllInputs | opeAllOutputs;

class PUMEX_EXPORT AttachmentDefinition
{
public:
  AttachmentDefinition();
  AttachmentDefinition(VkFormat format, const ImageSize& attachmentSize, AttachmentType attachmentType, const gli::swizzles& swizzles = gli::swizzles(gli::swizzle::SWIZZLE_RED, gli::swizzle::SWIZZLE_GREEN, gli::swizzle::SWIZZLE_BLUE, gli::swizzle::SWIZZLE_ALPHA));

  VkFormat              format;
  ImageSize             attachmentSize;
  AttachmentType        attachmentType;
  gli::swizzles         swizzles;
};

inline bool operator==(const AttachmentDefinition& lhs, const AttachmentDefinition& rhs);
inline bool operator!=(const AttachmentDefinition& lhs, const AttachmentDefinition& rhs);

class PUMEX_EXPORT ResourceDefinition
{
public:
  ResourceDefinition();
  // constructor for images and attachments
  ResourceDefinition(VkFormat format, const ImageSize& attachmentSize, AttachmentType attachmentType, const std::string& name = std::string(), const gli::swizzles& sw = gli::swizzles(gli::swizzle::SWIZZLE_RED, gli::swizzle::SWIZZLE_GREEN, gli::swizzle::SWIZZLE_BLUE, gli::swizzle::SWIZZLE_ALPHA));
  // constructor for buffers
  ResourceDefinition(const std::string& name);

  ResourceMetaType      metaType;
  AttachmentDefinition  attachment;
  std::string           name; // external resources must have a name
};

inline bool operator==(const ResourceDefinition& lhs, const ResourceDefinition& rhs);

// swapchain attachment must have name==SWAPCHAIN_NAME
// its definition must be SWAPCHAIN_DEFINITION(VkFormat,arrayLayers) : for example SWAPCHAIN_DEFINITION(VK_FORMAT_B8G8R8A8_UNORM,1)
const std::string               SWAPCHAIN_NAME = "SWAPCHAIN";
PUMEX_EXPORT ResourceDefinition SWAPCHAIN_DEFINITION(VkFormat format, uint32_t arrayLayers = 1 );

class PUMEX_EXPORT RenderOperationEntry
{
public:
  RenderOperationEntry() = default;
  explicit RenderOperationEntry(OperationEntryType entryType, const ResourceDefinition& resourceDefinition, const LoadOp& loadOp, const ImageSubresourceRange& imageRange, VkImageLayout layout, VkImageUsageFlags imageUsage, VkImageCreateFlags imageCreate, VkImageViewType imageViewType, const std::string& resolveSourceEntryName, bool storeAttachment);
  explicit RenderOperationEntry(OperationEntryType entryType, const ResourceDefinition& resourceDefinition, const BufferSubresourceRange& bufferRange, VkPipelineStageFlags pipelineStage, VkAccessFlags accessFlags, VkFormat bufferFormat = VK_FORMAT_UNDEFINED);

  OperationEntryType     entryType;
  ResourceDefinition     resourceDefinition;
  LoadOp                 loadOp;                                               // for images and attachments
  std::string            resolveSourceEntryName;                               // for resolve attachments
  bool                   storeAttachment = false;                              // ensure that output attachment is stored

  ImageSubresourceRange  imageRange;                                           // used by images
  VkImageLayout          layout          = VK_IMAGE_LAYOUT_UNDEFINED;          // used by attachments and images ( attachments have this value set automaticaly )
  VkImageUsageFlags      imageUsage      = 0;                                  // addidtional imageUsage for image inputs/outputs
  VkImageCreateFlags     imageCreate     = 0;                                  // additional image creation flags
  VkImageViewType        imageViewType   = VK_IMAGE_VIEW_TYPE_MAX_ENUM;        // manual imageViewType. Will be ignored and chosen automatically if == VK_IMAGE_VIEW_TYPE_MAX_ENUM

  BufferSubresourceRange bufferRange;                                          // used by buffers
  VkPipelineStageFlags   pipelineStage   = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT; // used by buffers
  VkAccessFlags          accessFlags     = VK_ACCESS_MEMORY_READ_BIT;          // used by buffers
  VkFormat               bufferFormat    = VK_FORMAT_UNDEFINED;                // used by texel buffers probably
};

class PUMEX_EXPORT RenderOperation
{
public:
  RenderOperation();
  RenderOperation(const std::string& name, OperationType operationType = opGraphics, const ImageSize& attachmentSize = ImageSize{ isSurfaceDependent, glm::vec2(1.0f,1.0f), 1, 1, 1 }, uint32_t multiViewMask = 0x0);

  void                  addAttachmentInput(const std::string& entryName, const ResourceDefinition& resourceDefinition, const LoadOp& loadOp = loadOpDontCare(), const ImageSubresourceRange& imageRange = ImageSubresourceRange(), VkImageCreateFlags imageCreate = 0);
  void                  addAttachmentOutput(const std::string& entryName, const ResourceDefinition& resourceDefinition, const LoadOp& loadOp = loadOpDontCare(), const ImageSubresourceRange& imageRange = ImageSubresourceRange(), VkImageCreateFlags imageCreate = 0, bool storeAttachment = false);
  void                  addAttachmentResolveOutput(const std::string& entryName, const ResourceDefinition& resourceDefinition, const LoadOp& loadOp = loadOpDontCare(), const ImageSubresourceRange& imageRange = ImageSubresourceRange(), VkImageCreateFlags imageCreate = 0, bool storeAttachment = false, const std::string& sourceEntryName = "");
  void                  setAttachmentDepthInput(const std::string& entryName, const ResourceDefinition& resourceDefinition, const LoadOp& loadOp = loadOpDontCare(), const ImageSubresourceRange& imageRange = ImageSubresourceRange(VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1), VkImageCreateFlags imageCreate = 0);
  void                  setAttachmentDepthOutput(const std::string& entryName, const ResourceDefinition& resourceDefinition, const LoadOp& loadOp = loadOpDontCare(), const ImageSubresourceRange& imageRange = ImageSubresourceRange(VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1), VkImageCreateFlags imageCreate = 0, bool storeAttachment = false);

  void                  addImageInput(const std::string& entryName, const ResourceDefinition& resourceDefinition, const LoadOp& loadOp = loadOpDontCare(), const ImageSubresourceRange& imageRange = ImageSubresourceRange(), VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED, VkImageUsageFlags imageUsage = 0, VkImageCreateFlags imageCreate = 0, VkImageViewType imageViewType = VK_IMAGE_VIEW_TYPE_MAX_ENUM);
  void                  addImageOutput(const std::string& entryName, const ResourceDefinition& resourceDefinition, const LoadOp& loadOp = loadOpDontCare(), const ImageSubresourceRange& imageRange = ImageSubresourceRange(), VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED, VkImageUsageFlags imageUsage = 0, VkImageCreateFlags imageCreate = 0, VkImageViewType imageViewType = VK_IMAGE_VIEW_TYPE_MAX_ENUM);

  void                  addBufferInput(const std::string& entryName, const ResourceDefinition& resourceDefinition, const BufferSubresourceRange& bufferRange = BufferSubresourceRange(), VkPipelineStageFlags pipelineStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VkAccessFlags accessFlags = VK_ACCESS_MEMORY_READ_BIT);
  void                  addBufferOutput(const std::string& entryName, const ResourceDefinition& resourceDefinition, const BufferSubresourceRange& bufferRange = BufferSubresourceRange(), VkPipelineStageFlags pipelineStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VkAccessFlags accessFlags = VK_ACCESS_MEMORY_READ_BIT);

  void                  setRenderOperationNode(std::shared_ptr<Node> node);
  std::shared_ptr<Node> getRenderOperationNode();

  std::vector<std::reference_wrapper<const RenderOperationEntry>> getEntries(OperationEntryTypeFlags entryTypes) const;

  std::string           name;
  OperationType         operationType   = opGraphics;
  ImageSize             attachmentSize;
  uint32_t              multiViewMask   = 0;
  bool                  enabled         = true;

  std::map<std::string, RenderOperationEntry> entries;
  std::shared_ptr<Node>                       node;
};

inline bool operator<(const RenderOperation& lhs, const RenderOperation& rhs);

class PUMEX_EXPORT ResourceTransition
{
public:
  ResourceTransition() = delete;
  explicit ResourceTransition(uint32_t rteid, uint32_t tid, const std::map<std::string, RenderOperation>::const_iterator& operation, const std::map<std::string, RenderOperationEntry>::const_iterator& entry, const std::string& externalMemoryObjectName);

  inline uint32_t                    rteid() const; // identifies specific ResourceTransition connected to specific entry
  inline uint32_t                    tid() const;  // identifies potential resource ( amny ResourceTransitions may have the same tid
  inline const RenderOperation&      operation() const;
  inline const RenderOperationEntry& entry() const;
  inline const std::string&          operationName() const;
  inline const std::string&          entryName() const;
  inline const std::string&          externalMemoryObjectName() const;
  inline const std::map<std::string, RenderOperation>::const_iterator      operationIter() const;
  inline const std::map<std::string, RenderOperationEntry>::const_iterator entryIter() const;
  inline void                        setExternalMemoryObjectName(const std::string& externalMemoryObjectName);

protected:
  uint32_t                                                    rteid_;
  uint32_t                                                    tid_;
  std::map<std::string, RenderOperation>::const_iterator      operation_;
  std::map<std::string, RenderOperationEntry>::const_iterator entry_;
  std::string                                                 externalMemoryObjectName_;
};

class PUMEX_EXPORT ResourceTransitionDescription
{
public:
  ResourceTransitionDescription(const std::string& generatingOperation, const std::string& generatingEntry, const std::string& consumingOperation, const std::string& consumingEntry);

  std::string generatingOperation;
  std::string generatingEntry;
  std::string consumingOperation;
  std::string consumingEntry;
};

class PUMEX_EXPORT RenderGraph : public std::enable_shared_from_this<RenderGraph>
{
public:
  RenderGraph()                                 = delete;
  explicit RenderGraph(const std::string& name);
  RenderGraph(const RenderGraph&)            = delete;
  RenderGraph& operator=(const RenderGraph&) = delete;
  RenderGraph(RenderGraph&&)                 = delete;
  RenderGraph& operator=(RenderGraph&&)      = delete;
  ~RenderGraph();

  void                                                          addRenderOperation(const RenderOperation& op);
  // add resource transition between two operations
  void                                                          addResourceTransition(const ResourceTransitionDescription& resTran, const std::string& externalMemoryObjectName = std::string());
  // handy function for adding resource transition between two operations
  void                                                          addResourceTransition(const std::string& generatingOperation, const std::string& generatingEntry, const std::string& consumingOperation, const std::string& consumingEntry, const std::string& externalMemoryObjectName = std::string() );
  // add one transition that has many generating operations, but only one resource will be used ( all generating transitions must have disjunctive subresource ranges )
  void                                                          addResourceTransition(const std::vector<ResourceTransitionDescription>& resTrans, const std::string& externalMemoryObjectName = std::string());
  // add resource transition between operation and external memory object ( if memory object is not defined, then this is "empty" transition )
  void                                                          addResourceTransition(const std::string& opName, const std::string& entryName, const std::string& externalMemoryObjectName = std::string());
  // add missing resource transitions ("empty"). MUST be called before graph compilation
  void                                                          addMissingResourceTransitions();

  std::vector<std::string>                                      getRenderOperationNames() const;
  const RenderOperation&                                        getRenderOperation(const std::string& opName) const;
  RenderOperation&                                              getRenderOperation(const std::string& opName);

  void                                                          setRenderOperationNode(const std::string& opName, std::shared_ptr<Node> node);
  std::shared_ptr<Node>                                         getRenderOperationNode(const std::string& opName);

  std::vector<std::reference_wrapper<const ResourceTransition>> getOperationIO(const std::string& opName, OperationEntryTypeFlags entryTypes) const;
  std::vector<std::reference_wrapper<const ResourceTransition>> getTransitionIO(uint32_t transitionID, OperationEntryTypeFlags entryTypes) const;
  std::reference_wrapper<const ResourceTransition>              getTransition(uint32_t rteid) const;

  std::string                                                   name;
  std::map<std::string, RenderOperation>                        operations;
  std::vector<ResourceTransition>                               transitions;
protected:
  uint32_t                                                      generateTransitionID();
  uint32_t                                                      nextTransitionID = 1;
  uint32_t                                                      generateTransitionEntryID();
  uint32_t                                                      nextTransitionEntryID = 1;

  bool                                                          valid = false;
};

using RenderOperationSet = std::set<std::reference_wrapper<const RenderOperation>>;

PUMEX_EXPORT RenderOperationSet getInitialOperations(const RenderGraph& renderGraph);
PUMEX_EXPORT RenderOperationSet getFinalOperations(const RenderGraph& renderGraph);
PUMEX_EXPORT RenderOperationSet getPreviousOperations(const RenderGraph& renderGraph, const std::string& opName);
PUMEX_EXPORT RenderOperationSet getNextOperations(const RenderGraph& renderGraph, const std::string& opName);
PUMEX_EXPORT RenderOperationSet getAllPreviousOperations(const RenderGraph& renderGraph, const std::string& opName);
PUMEX_EXPORT RenderOperationSet getAllNextOperations(const RenderGraph& renderGraph, const std::string& opName);

// inlines

LoadOp  loadOpLoad()                        { return LoadOp(LoadOp::Load, glm::vec4(0.0f)); }
LoadOp  loadOpClear(const glm::vec2& color) { return LoadOp(LoadOp::Clear, glm::vec4(color.x, color.y, 0.0f, 0.0f)); }
LoadOp  loadOpClear(const glm::vec4& color) { return LoadOp(LoadOp::Clear, color); }
LoadOp  loadOpDontCare()                    { return LoadOp(LoadOp::DontCare, glm::vec4(0.0f)); }
StoreOp storeOpStore()                      { return StoreOp(StoreOp::Store); }
StoreOp storeOpDontCare()                   { return StoreOp(StoreOp::DontCare); }

bool operator==(const AttachmentDefinition& lhs, const AttachmentDefinition& rhs)
{
  return lhs.format == rhs.format && lhs.attachmentType == rhs.attachmentType && lhs.attachmentSize == rhs.attachmentSize && lhs.swizzles == rhs.swizzles;
}

bool operator!=(const AttachmentDefinition& lhs, const AttachmentDefinition& rhs)
{
  return lhs.format != rhs.format || lhs.attachmentType != rhs.attachmentType || lhs.attachmentSize != rhs.attachmentSize || lhs.swizzles != rhs.swizzles;
}

bool operator==(const ResourceDefinition& lhs, const ResourceDefinition& rhs)
{
  if ( lhs.metaType != rhs.metaType )
    return false;
  if ( ( !lhs.name.empty() || !rhs.name.empty()) && lhs.name == rhs.name )
    return true;
  return lhs.attachment == rhs.attachment;
}

bool operator<(const RenderOperation& lhs, const RenderOperation& rhs) { return lhs.name < rhs.name; }

uint32_t                                                          ResourceTransition::rteid() const                    { return rteid_; }
uint32_t                                                          ResourceTransition::tid() const                      { return tid_; }
const RenderOperation&                                            ResourceTransition::operation() const                { return operation_->second; }
const RenderOperationEntry&                                       ResourceTransition::entry() const                    { return entry_->second; }
const std::string&                                                ResourceTransition::operationName() const            { return operation_->first; }
const std::string&                                                ResourceTransition::entryName() const                { return entry_->first; }
const std::string&                                                ResourceTransition::externalMemoryObjectName() const { return externalMemoryObjectName_; }
const std::map<std::string, RenderOperation>::const_iterator      ResourceTransition::operationIter() const            { return operation_; }
const std::map<std::string, RenderOperationEntry>::const_iterator ResourceTransition::entryIter() const                { return entry_; }
void                                          ResourceTransition::setExternalMemoryObjectName(const std::string& emon) { externalMemoryObjectName_ = emon; }



}