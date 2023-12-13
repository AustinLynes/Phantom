#include "Framebuffer.h"
#include "../Application/Application.h"
#include <Core/Debug/Debug.h>
#include <fstream>


namespace Core {


	// D3D12 Resources



	namespace Graphics {


		void ThrowIfFailed(HRESULT hres) {

			if (!SUCCEEDED(hres)) {
				auto err = GetLastError();

				LPSTR message;

				FormatMessageA(
					FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					nullptr,
					err,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					reinterpret_cast<LPSTR>(&message),
					0,
					nullptr
				);

				throw std::runtime_error(message);
			}
		}

#define CHECK(hr) ThrowIfFailed(hr)

#ifdef _DEBUG


		void EnableD3D12DebugLayer() {

			ComPtr<ID3D12Debug> dbgInterface;
			CHECK(D3D12GetDebugInterface(IID_PPV_ARGS(&dbgInterface)));
			dbgInterface->EnableDebugLayer();

		}


#endif

		ComPtr<IDXGIAdapter4> SelectAdapter() {
			ComPtr<IDXGIFactory4> factory;

			UINT flags = 0;
#ifdef _DEBUG
			flags = DXGI_CREATE_FACTORY_DEBUG;
#endif 
			auto hres = CreateDXGIFactory2(flags, IID_PPV_ARGS(&factory));
			CHECK(hres);

			ComPtr<IDXGIAdapter1> temp;
			ComPtr<IDXGIAdapter4> adapter;
			int maxMemory = 0;

			for (UINT i = 0; factory->EnumAdapters1(i, &temp) != DXGI_ERROR_NOT_FOUND; i++)
			{
				DXGI_ADAPTER_DESC1 desc{};
				temp->GetDesc1(&desc);
				bool isDedicated = (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0;
				if (isDedicated && desc.DedicatedVideoMemory > maxMemory) {
					maxMemory = desc.DedicatedVideoMemory;
					hres = temp.As(&adapter);
					CHECK(hres);
				}

			}

			return adapter;
		}

		ComPtr<ID3D12Device2> CreateDevice(IDXGIAdapter4* adapter) {
			HRESULT hres = 0;

			ComPtr<ID3D12Device2> device;

			hres = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device));
			CHECK(hres);

#if _DEBUG
			ComPtr<ID3D12InfoQueue> infoQueue;
			if (SUCCEEDED(device.As(&infoQueue))) {
				infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
				infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
				infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

				/*D3D12_MESSAGE_CATEGORY categories[]{

				}*/

				D3D12_MESSAGE_SEVERITY severities[]{
					D3D12_MESSAGE_SEVERITY_INFO
				};

				D3D12_MESSAGE_ID denyList[]{
					D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
					D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
					D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE
				};


				D3D12_INFO_QUEUE_FILTER filter{};
				filter.DenyList.pIDList = denyList;
				filter.DenyList.NumIDs = _countof(denyList);
				filter.DenyList.pSeverityList = severities;
				filter.DenyList.NumSeverities = _countof(severities);

				hres = infoQueue->PushStorageFilter(&filter);

			}
#endif

			return device;
		}

		ComPtr<ID3D12CommandQueue> CreateCommandQueue(ID3D12Device2* pDevice, int priority = 0, D3D12_COMMAND_LIST_TYPE cmdListType = D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_QUEUE_FLAGS flags = D3D12_COMMAND_QUEUE_FLAG_NONE) {
			ComPtr<ID3D12CommandQueue> cmdQueue;

			D3D12_COMMAND_QUEUE_DESC desc{};
			desc.Flags = flags;
			desc.NodeMask = 0;
			desc.Priority = priority;
			desc.Type = cmdListType;

			HRESULT err = pDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&cmdQueue));
			CHECK(err);

			return cmdQueue;

		}

		void Framebuffer::Init() {

#ifdef _DEBUG
			EnableD3D12DebugLayer();
#endif
			
			adapter = SelectAdapter();
			device = CreateDevice(adapter.Get());

			cmdQueue_DIRECT = CreateCommandQueue(device.Get(), 0, D3D12_COMMAND_LIST_TYPE_DIRECT);

			frames = new Frame[frameCount];
			
			// create the RTVs descriptor heap
			D3D12_DESCRIPTOR_HEAP_DESC rtvHeap{};
			rtvHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			rtvHeap.NodeMask = 0;
			rtvHeap.NumDescriptors = frameCount;
			rtvHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

			HRESULT hres = device->CreateDescriptorHeap(&rtvHeap, IID_PPV_ARGS(&rtvDescriptorHeap));

			// aquire handle to the newly created descriptor heap
			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();


			// create the pipeline state object
			D3D12_INPUT_ELEMENT_DESC elements[]{
				{"POSITION", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
				{"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			};

			
			D3D12_INPUT_LAYOUT_DESC inputLayout{ 0 };
			inputLayout.NumElements = _countof(elements);
			inputLayout.pInputElementDescs = elements;

			D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeDesc{};
			pipeDesc.NodeMask = 0;
			pipeDesc.InputLayout = inputLayout;
			


			device->CreateGraphicsPipelineState(&pipeDesc, IID_PPV_ARGS(&pipelineState));

			// each frame needs to be created from scratch to render-to-texture.
			for (int i = 0; i < frameCount; i++)
			{
				Frame frame{0};

				D3D12_RESOURCE_DESC desc{};
				desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
				desc.Alignment = 0;
				desc.Width = width;
				desc.Height = height;
				desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
				desc.SampleDesc.Count = 1;
				desc.SampleDesc.Quality = 0;
				desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
				desc.MipLevels = 1;
				desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
				desc.DepthOrArraySize = 1;
				// create the render textures
				auto rtvHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
				hres = device->CreateCommittedResource(&rtvHeapProperties, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_RENDER_TARGET, nullptr, IID_PPV_ARGS(&frame.RenderTexture));
				CHECK(hres);

				D3D12_RENDER_TARGET_VIEW_DESC rtv{};
				rtv.Format = desc.Format;
				rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
				rtv.Texture2D.MipSlice = 0;

				device->CreateRenderTargetView(frame.RenderTexture.Get(), &rtv, rtvHandle);

				const UINT descriptrSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
				rtvHandle.ptr += descriptrSize;


				// create the command list

				hres = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&frame.commandAllocator));
				CHECK(hres);

				hres = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, frame.commandAllocator.Get(), pipelineState.Get(), IID_PPV_ARGS(&frame.commandList));
				CHECK(hres);
				hres = frame.commandList->Close();
				CHECK(hres);

				hres = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&frame.fence));
				CHECK(hres);
				frame.frameFenceValue = 0;

				frame.fenceEvent = CreateEventEx(NULL, L"FenceEvent", NULL, NULL);

				frames[i] = frame;
			}

		}



		Framebuffer::Framebuffer(uint32_t width, uint32_t height)
			: width{ width }, height{ height }, frameCount{ 3 }, frames{ nullptr }
		{
			Init();
		}


		Framebuffer::~Framebuffer()
		{
			delete[] frames;
		}

		void Framebuffer::Render() {
			HRESULT hr;
			for (UINT64 i = 0; i < frameCount; ++i) {

				auto& frame = frames[i];

				hr = frame.commandAllocator.Reset();
				CHECK(hr);
				hr = frame.commandList->Reset(frame.commandAllocator.Get(), nullptr);
				CHECK(hr);
				// clear render target view
				{
					float clearColor[4]{ 0.7f, 0.4f ,0.3f, 1.0f };
					CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), i, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
					frame.commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
				}


				hr = frame.commandList->Close();
				CHECK(hr);


				ID3D12CommandList* const commandLists[] = {
					frame.commandList.Get()
				};

				cmdQueue_DIRECT->ExecuteCommandLists(_countof(commandLists), commandLists);

				// signal fence for current frame

				cmdQueue_DIRECT->Signal(frame.fence.Get(), frame.frameFenceValue);

				// wait for fence to complete 
				if (frame.fence->GetCompletedValue() < frame.frameFenceValue) {
					frame.fence->SetEventOnCompletion(frame.frameFenceValue, frame.fenceEvent);
					WaitForSingleObject(frame.fenceEvent, (DWORD)UINT64_MAX);
				}

				// update fence value for next frame
				frame.frameFenceValue++;
			}

		}

		void Framebuffer::Present()
		{
		}

		


	}
}