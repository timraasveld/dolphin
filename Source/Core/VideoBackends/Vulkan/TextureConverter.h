// Copyright 2016 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <memory>

#include "Common/CommonTypes.h"
#include "VideoBackends/Vulkan/StreamBuffer.h"
#include "VideoBackends/Vulkan/TextureCache.h"
#include "VideoCommon/BPMemory.h"
#include "VideoCommon/TextureDecoder.h"
#include "VideoCommon/VideoCommon.h"

namespace Vulkan
{
class StagingTexture2D;
class Texture2D;

class TextureConverter
{
public:
  TextureConverter();
  ~TextureConverter();

  bool Initialize();

  // Applies palette to dst_entry, using indices from src_entry.
  void ConvertTexture(TextureCache::TCacheEntry* dst_entry, TextureCache::TCacheEntry* src_entry,
                      VkRenderPass render_pass, const void* palette, TlutFormat palette_format);

  // Uses an encoding shader to copy src_texture to dest_ptr.
  // NOTE: Executes the current command buffer.
  void EncodeTextureToMemory(VkImageView src_texture, u8* dest_ptr, u32 format, u32 native_width,
                             u32 bytes_per_row, u32 num_blocks_y, u32 memory_stride,
                             PEControl::PixelFormat src_format, bool is_intensity,
                             int scale_by_half, const EFBRectangle& source);

  // Encodes texture to guest memory in XFB (YUYV) format.
  void EncodeTextureToMemoryYUYV(void* dst_ptr, u32 dst_width, u32 dst_stride, u32 dst_height,
                                 Texture2D* src_texture, const MathUtil::Rectangle<int>& src_rect);

  // Decodes data from guest memory in XFB (YUYV) format to a RGBA format texture on the GPU.
  void DecodeYUYVTextureFromMemory(TextureCache::TCacheEntry* dst_texture, const void* src_ptr,
                                   u32 src_width, u32 src_stride, u32 src_height);

private:
  static const u32 NUM_TEXTURE_ENCODING_SHADERS = 64;
  static const u32 ENCODING_TEXTURE_WIDTH = EFB_WIDTH * 4;
  static const u32 ENCODING_TEXTURE_HEIGHT = 1024;
  static const VkFormat ENCODING_TEXTURE_FORMAT = VK_FORMAT_B8G8R8A8_UNORM;
  static const size_t NUM_PALETTE_CONVERSION_SHADERS = 3;

  bool CreateTexelBuffer();
  VkBufferView CreateTexelBufferView(VkFormat format) const;

  bool CompilePaletteConversionShaders();

  bool CompileEncodingShaders();
  bool CreateEncodingRenderPass();
  bool CreateEncodingTexture();
  bool CreateEncodingDownloadTexture();

  bool CompileYUYVConversionShaders();

  // Allocates storage in the texel command buffer of the specified size.
  // If the buffer does not have enough space, executes the current command buffer and tries again.
  // If this is done, g_command_buffer_mgr->GetCurrentCommandBuffer() will return a different value,
  // so it always should be re-obtained after calling this method.
  // Once the data copy is done, call m_texel_buffer->CommitMemory(size).
  bool ReserveTexelBufferStorage(size_t size, size_t alignment);

  // Returns the command buffer that the texture conversion should occur in for the given texture.
  // This can be the initialization/copy command buffer, or the drawing command buffer.
  VkCommandBuffer GetCommandBufferForTextureConversion(const TextureCache::TCacheEntry* src_entry);

  // Shared between conversion types
  std::unique_ptr<StreamBuffer> m_texel_buffer;
  VkBufferView m_texel_buffer_view_r16_uint = VK_NULL_HANDLE;
  VkBufferView m_texel_buffer_view_rgba8_unorm = VK_NULL_HANDLE;
  size_t m_texel_buffer_size = 0;

  // Palette conversion - taking an indexed texture and applying palette
  std::array<VkShaderModule, NUM_PALETTE_CONVERSION_SHADERS> m_palette_conversion_shaders = {};

  // Texture encoding - RGBA8->GX format in memory
  std::array<VkShaderModule, NUM_TEXTURE_ENCODING_SHADERS> m_encoding_shaders = {};
  VkRenderPass m_encoding_render_pass = VK_NULL_HANDLE;
  std::unique_ptr<Texture2D> m_encoding_render_texture;
  VkFramebuffer m_encoding_render_framebuffer = VK_NULL_HANDLE;
  std::unique_ptr<StagingTexture2D> m_encoding_download_texture;

  // XFB encoding/decoding shaders
  VkShaderModule m_rgb_to_yuyv_shader = VK_NULL_HANDLE;
  VkShaderModule m_yuyv_to_rgb_shader = VK_NULL_HANDLE;
};

}  // namespace Vulkan
