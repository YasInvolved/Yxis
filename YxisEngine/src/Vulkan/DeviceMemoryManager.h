// this class manages resources allocated on GPU
// - resources creation (buffers, images)
// - resources deletion
// - TODO: moving data to resources
// - automatic resource freeing on crash and exit

#pragma once
#include "../internal_pch.h"

namespace Yxis::Vulkan
{
   class Device;

   class DeviceMemoryManager
   {
      enum class ResourceType { BUFFER, IMAGE };
      using ResourceKey = std::pair<uintptr_t, ResourceType>;

      struct ResourceKeyHash
      {
         std::size_t operator()(const ResourceKey& key) const
         {
            return std::hash<uintptr_t>{}(key.first) ^ std::hash<int>{}(static_cast<int>(key.second));
         }
      };

      struct ResourceKeyEqual
      {
         bool operator()(const ResourceKey& r1, const ResourceKey& r2) const
         {
            return r1.first == r2.first && r1.second == r2.second;
         }
      };

   public:
      DeviceMemoryManager(const Device* device);
      VkBuffer createBuffer(VkBufferCreateInfo& createInfo);
      VkImage createImage(VkImageCreateInfo& createInfo);
      void copyToBuffer(const void* asset, VkBuffer buffer, std::optional<VkDeviceSize> size = {});
      void copyToImage(const void* asset, VkImage image, std::optional<VkDeviceSize> size = {});
      void deleteAsset(VkBuffer buffer);
      void deleteAsset(VkImage image);
      ~DeviceMemoryManager();
   private:
      const Device* m_device;
      VkPhysicalDeviceMemoryProperties2 m_memoryProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2 };
      VmaAllocator m_allocator = nullptr;
      std::unordered_map<ResourceKey, VmaAllocation, ResourceKeyHash, ResourceKeyEqual> m_allocations;

      class StagingBuffer 
      {
      public:
         StagingBuffer(DeviceMemoryManager* memoryManager);
         ~StagingBuffer();
      
         void copyToBuffer(const void* data, VkBuffer buffer, VkDeviceSize size);
         void copyToImage();

      private:
         static constexpr size_t s_sbChunks = 8;
         static constexpr size_t s_sbChunkSize = 32 * 1024; // 32kb
         static constexpr VkDeviceSize s_sbSize = s_sbChunks * s_sbChunkSize;
         struct Chunk { uint8_t data[s_sbChunkSize]; };

         DeviceMemoryManager* m_memoryManager;
         std::array<VkCommandBuffer, 1> m_transferCommandBuffers;

         void* m_memPtr;
         VmaAllocation m_sbAllocation;
         VkBuffer m_stagingBuffer;
         VkQueue m_transferQueue;
      };

      std::unique_ptr<StagingBuffer> m_stagingBuffer;
   };
}