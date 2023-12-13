#pragma once

#include <string>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")


#include <wrl/client.h>

#include <directx/d3d12.h>
#include <directx/d3dx12.h>
#include <directx/d3dx12_resource_helpers.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include <DirectXHelpers.h>
#include <GraphicsMemory.h>


namespace Core {
	namespace Graphics {
		
		using namespace Microsoft::WRL;


		class Framebuffer
		{
			struct Frame {
				HANDLE fenceEvent;
				UINT64 frameFenceValue;
				ComPtr<ID3D12Fence> fence;
				ComPtr<ID3D12CommandAllocator> commandAllocator;
				ComPtr<ID3D12GraphicsCommandList> commandList;
				ComPtr<ID3D12Resource> RenderTexture;

			
			};

		public:
			Framebuffer(uint32_t width, uint32_t height);
			~Framebuffer();

			void Render();
			void Present();



			Frame* frames;
		private:
			void Init();

		private:
			ComPtr<ID3D12Device2> device;
			ComPtr<IDXGIAdapter4> adapter;
			ComPtr<ID3D12CommandQueue> cmdQueue_DIRECT;

			ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;
			ComPtr<ID3D12PipelineState> pipelineState;
			

			uint32_t width;
			uint32_t height;
			uint32_t frameCount;
	
			int currentFrame = 0;


		};

	}
}