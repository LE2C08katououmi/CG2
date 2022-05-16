#include<Windows.h>
#include<d3d12.h>
#include<dxgi1_6.h>
#include<cassert>
#include<vector>
#include<string>
#include<DirectXMath.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")

using namespace std;
using namespace DirectX;

//ウィンドウプロシージャ
LRESULT WincowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_DESTROY://ウィンドウが破棄

		PostQuitMessage(0);//OSに終了を伝える
		return 0;
	}

	//標準の処理を行う
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
{

#pragma region windowAPI

	//ウィンドウサイズ
	const int window_widht = 1200;
	const int window_height = 720;

	//ウィンドウクラス生成
	WNDCLASSEX w{};
	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WincowProc;//ウィンドウプロシージャを設定
	w.lpszClassName = L"DirectXGame";//クラス名
	w.hInstance = GetModuleHandle(nullptr);//ハンドル
	w.hCursor = LoadCursor(NULL, IDC_ARROW);//カーソル設定

	//クラスを登録
	RegisterClassEx(&w);
	//X,Y座標,縦,横幅
	RECT wrc = { 0,0,window_widht,window_height };
	//サイズ補正
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	//ウィンドウオブジェクトの生成
	HWND hwnd = CreateWindow(
		w.lpszClassName,//クラス名指定
		L"DirectXGame",//タイトルバーの文字
		WS_OVERLAPPEDWINDOW,//タイトルバーと境界線があるウィンドウです
		CW_USEDEFAULT,//表示X座標はOSにお任せします
		CW_USEDEFAULT,//表示Y座標はOSにお任せします
		wrc.right - wrc.left,//ウィンドウ幅
		wrc.bottom - wrc.top,//ウィンドウ高
		nullptr,//親ウィンドウハンドル
		nullptr,//メニューハンドル
		w.hInstance,//呼び出しアプリケーションハンドル
		nullptr);//追加パラメータ

	ShowWindow(hwnd, SW_SHOW);//ウィンドウ表示

	MSG msg{};

#pragma endregion

#pragma region DirectX初期化

#ifdef _DEBUG
	ID3D12Debug* debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();
	}
#endif // DEBUG

	HRESULT result;
	ID3D12Device* device = nullptr;
	IDXGIFactory7* dxgiFactory = nullptr;
	IDXGISwapChain4* swapChain = nullptr;
	ID3D12CommandAllocator* cmdAllocator = nullptr;
	ID3D12GraphicsCommandList* cmmandList = nullptr;
	ID3D12CommandQueue* cmmandQueue = nullptr;
	ID3D12DescriptorHeap* rtvHeap = nullptr;

	//DXGIファクトリー
	result = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));
	assert(SUCCEEDED(result));

	//アダプター列挙用
	vector<IDXGIAdapter4*> adapters;
	//ここに特定の名前を持つアダプターオブジェクトが入る
	IDXGIAdapter4* tmpAdapter = nullptr;

	//パフォーマンスが高いのもから全て列挙
	for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&tmpAdapter)) != DXGI_ERROR_NOT_FOUND; i++)
	{
		adapters.push_back(tmpAdapter);
	}

	//対応レベルの配列
	D3D_FEATURE_LEVEL levels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	//Direct3Dデバイスの初期化
	D3D_FEATURE_LEVEL featureLevel;
	for (auto lv : levels)
	{
		if (D3D12CreateDevice(tmpAdapter, lv, IID_PPV_ARGS(&device)) == S_OK)
		{
			//生成可能なバージョンが見つかったらループを打ち切り
			featureLevel = lv;
			break;
		}
	}

	//コマンドアロケータを生成
	result = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator));
	assert(SUCCEEDED(result));

	//コマンドリストを生成
	result = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocator, nullptr, IID_PPV_ARGS(&cmmandList));
	assert(SUCCEEDED(result));

	//コマンドキューの設定＆生成
	D3D12_COMMAND_QUEUE_DESC cmmandQueueDesc{};
	result = device->CreateCommandQueue(&cmmandQueueDesc, IID_PPV_ARGS(&cmmandQueue));
	assert(SUCCEEDED(result));

	//スワップチェーンの設定
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = window_widht;//横幅
	swapChainDesc.Height = window_height;//縦幅
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;//色情報の書式
	swapChainDesc.SampleDesc.Count = 1;//マルチサンプルなし
	swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;//バックバッファ用
	swapChainDesc.BufferCount = 2;//バッファ数2
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;//フリップ後は破棄
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	//生成
	result = dxgiFactory->CreateSwapChainForHwnd(cmmandQueue, hwnd, &swapChainDesc, nullptr, nullptr, (IDXGISwapChain1**)&swapChain);
	assert(SUCCEEDED(result));

	//デスクリプタヒープの設定
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;//レンダーターゲットビュー
	rtvHeapDesc.NumDescriptors = swapChainDesc.BufferCount;//表裏の2つ
	//デスクリプタヒープの生成
	device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap));

	//バックバッファ
	vector<ID3D12Resource*> backBuffers;
	backBuffers.resize(swapChainDesc.BufferCount);

	//スワップチェーンのバッファを処理
	for (size_t i = 0; i < backBuffers.size(); i++)
	{
		//バッファを取得
		swapChain->GetBuffer((UINT)i, IID_PPV_ARGS(&backBuffers[i]));
		//ヒープのハンドルの所得
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
		//裏か表かでアドレスがずれる
		rtvHandle.ptr += i * device->GetDescriptorHandleIncrementSize(rtvHeapDesc.Type);
		//ターゲットビューの設定
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
		//計算結果をSRGBに変換
		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		//ターゲットビューの生成
		device->CreateRenderTargetView(backBuffers[i], &rtvDesc, rtvHandle);
	}

	//フェンスの生成
	ID3D12Fence* fence = nullptr;
	UINT64 fenceVal = 0;
	result = device->CreateFence(fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));

#pragma endregion

#pragma region 描画初期化処理

	// 頂点データ
	XMFLOAT3 vertices[] = {
	 { -0.5f, -0.5f, 0.0f }, // 左下
	 { +0.5f, -0.5f, 0.0f }, // 右下
	 { -0.5f, +0.0f, 0.0f }, // 左中

	 { +0.5f, -0.0f, 0.0f }, // 右中
	 { -0.5f, +0.5f, 0.0f }, // 左上
	 { +0.5f, +0.5f, 0.0f }, // 右上
	};
	// 頂点データ全体のサイズ = 頂点データ一つ分のサイズ * 頂点データの要素数
	UINT sizeVB = static_cast<UINT>(sizeof(XMFLOAT3) * _countof(vertices));

	// 頂点バッファの設定
	D3D12_HEAP_PROPERTIES heapProp{}; // ヒープ設定
	heapProp.Type = D3D12_HEAP_TYPE_UPLOAD; // GPUへの転送用
	// リソース設定
	D3D12_RESOURCE_DESC resDesc{};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Width = sizeVB; // 頂点データ全体のサイズ
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	// 頂点バッファの生成
	ID3D12Resource* vertBuff = nullptr;
	result = device->CreateCommittedResource(
		&heapProp, // ヒープ設定
		D3D12_HEAP_FLAG_NONE,
		&resDesc, // リソース設定
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertBuff));
	assert(SUCCEEDED(result));

	// GPU上のバッファに対応した仮想メモリ(メインメモリ上)を取得
	XMFLOAT3* vertMap = nullptr;
	result = vertBuff->Map(0, nullptr, (void**)&vertMap);
	assert(SUCCEEDED(result));
	// 全頂点に対して
	for (int i = 0; i < _countof(vertices); i++)
	{
		vertMap[i] = vertices[i]; // 座標をコピー
	}
	// 繋がりを解除
	vertBuff->Unmap(0, nullptr);

	// 頂点バッファビューの作成
	D3D12_VERTEX_BUFFER_VIEW vbView{};
	// GPU仮想アドレス
	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();
	// 頂点バッファのサイズ
	vbView.SizeInBytes = sizeVB;
	// 頂点１つ分のデータサイズ
	vbView.StrideInBytes = sizeof(XMFLOAT3);

	ID3DBlob* vsBlob = nullptr; // 頂点シェーダオブジェクト
	ID3DBlob* psBlob = nullptr; // ピクセルシェーダオブジェクト
	ID3DBlob* errorBlob = nullptr; // エラーオブジェクト

	// 頂点シェーダの読み込みとコンパイル
	result = D3DCompileFromFile(
		L"BasicVS.hlsl", // シェーダファイル名
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE, // インクルード可能にする
		"main", "vs_5_0", // エントリーポイント名、シェーダーモデル指定
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // デバッグ用設定
		0,
		&vsBlob, &errorBlob);


	// エラーなら
	if (FAILED(result))
	{
		// errorBlobからエラー内容をstring型にコピー
		string error;
		error.resize(errorBlob->GetBufferSize());
		copy_n((char*)errorBlob->GetBufferPointer(),
			errorBlob->GetBufferSize(),
			error.begin());
		error += "\n";
		// エラー内容を出力ウィンドウに表示
		OutputDebugStringA(error.c_str());
		assert(0);
	}

	// ピクセルシェーダの読み込みとコンパイル
	result = D3DCompileFromFile(
		L"BasicPS.hlsl", // シェーダファイル名
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE, // インクルード可能にする
		"main", "ps_5_0", // エントリーポイント名、シェーダーモデル指定
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // デバッグ用設定
		0,
		&psBlob, &errorBlob);

	// エラーなら
	if (FAILED(result))
	{
		// errorBlobからエラー内容をstring型にコピー
		string error;
		error.resize(errorBlob->GetBufferSize());
		copy_n((char*)errorBlob->GetBufferPointer(),
			errorBlob->GetBufferSize(),
			error.begin());
		error += "\n";
		// エラー内容を出力ウィンドウに表示
		OutputDebugStringA(error.c_str());
		assert(0);
	}

	// 頂点レイアウト
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
	 {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};

	// グラフィックスパイプライン設定
	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc{};

	// シェーダーの設定
	pipelineDesc.VS.pShaderBytecode = vsBlob->GetBufferPointer();
	pipelineDesc.VS.BytecodeLength = vsBlob->GetBufferSize();
	pipelineDesc.PS.pShaderBytecode = psBlob->GetBufferPointer();
	pipelineDesc.PS.BytecodeLength = psBlob->GetBufferSize();

	// サンプルマスクの設定
	pipelineDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK; // 標準設定

	// ラスタライザの設定
	pipelineDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // カリングしない
	pipelineDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID; // ポリゴン内塗りつぶし
	pipelineDesc.RasterizerState.DepthClipEnable = true; // 深度クリッピングを有効に

	// ブレンドステート
	pipelineDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL; // RBGA全てのチャンネルを描画

	// 頂点レイアウトの設定
	pipelineDesc.InputLayout.pInputElementDescs = inputLayout;
	pipelineDesc.InputLayout.NumElements = _countof(inputLayout);

	// 図形の形状設定
	pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// その他の設定
	pipelineDesc.NumRenderTargets = 1; // 描画対象は1つ
	pipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // 0〜255指定のRGBA
	pipelineDesc.SampleDesc.Count = 1; // 1ピクセルにつき1回サンプリング

	// ルートシグネチャ
	ID3D12RootSignature* rootSignature;
	// ルートシグネチャの設定
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	// ルートシグネチャのシリアライズ
	ID3DBlob* rootSigBlob = nullptr;
	result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0,
		&rootSigBlob, &errorBlob);
	assert(SUCCEEDED(result));
	result = device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature));
	assert(SUCCEEDED(result));
	rootSigBlob->Release();
	// パイプラインにルートシグネチャをセット
	pipelineDesc.pRootSignature = rootSignature;

	// パイプランステートの生成
	ID3D12PipelineState* pipelineState = nullptr;
	result = device->CreateGraphicsPipelineState(&pipelineDesc, IID_PPV_ARGS(&pipelineState));
	assert(SUCCEEDED(result));

#pragma endregion



	while (true)
	{
#pragma region ウィンドウメッセージ処理

		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);

		}

		//もうアプリケーションが終わるって時にmessageがWM_QUITになる
		if (msg.message == WM_QUIT)
		{
			break;
		}

#pragma endregion

#pragma region 毎フレーム処理

		//1バックバッファ番号を取得
		UINT bbIndex = swapChain->GetCurrentBackBufferIndex();
		//書き込み可能に変更
		D3D12_RESOURCE_BARRIER barrierDesc{};
		barrierDesc.Transition.pResource = backBuffers[bbIndex];//バックバッファを指定
		barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;//表示状態から
		barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;//描画状態へ
		cmmandList->ResourceBarrier(1, &barrierDesc);

		//2描画先変更
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
		rtvHandle.ptr += static_cast<unsigned long long>(bbIndex) * device->GetDescriptorHandleIncrementSize(rtvHeapDesc.Type);
		cmmandList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);

		//3画面クリア
		FLOAT clearColor[] = { 0.1f,0.25f,0.5f,0.0f };
		cmmandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

#pragma region グラフィックスコマンド

		// ビューポート設定コマンド
		D3D12_VIEWPORT viewport{};
		viewport.Width = window_widht;
		viewport.Height = window_height;
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		// ビューポート設定コマンドを、コマンドリストに積む
		cmmandList->RSSetViewports(1, &viewport);

		// シザー矩形
		D3D12_RECT scissorRect{};
		scissorRect.left = 0; // 切り抜き座標左
		scissorRect.right = scissorRect.left + window_widht; // 切り抜き座標右
		scissorRect.top = 0; // 切り抜き座標上
		scissorRect.bottom = scissorRect.top + window_height; // 切り抜き座標下

		// シザー矩形設定コマンドを、コマンドリストに積む
		cmmandList->RSSetScissorRects(1, &scissorRect);

		// パイプラインステートとルートシグネチャの設定コマンド
		cmmandList->SetPipelineState(pipelineState);
		cmmandList->SetGraphicsRootSignature(rootSignature);

		// プリミティブ形状の設定コマンド
		cmmandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINESTRIP); // 三角形リスト

		// 頂点バッファビューの設定コマンド
		cmmandList->IASetVertexBuffers(0, 1, &vbView);

		// 描画コマンド
		cmmandList->DrawInstanced(_countof(vertices), 1, 0, 0); // 全ての頂点を使って描画

#pragma endregion

		//5元に戻す
		barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;//描画状態から
		barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;//表示状態へ
		cmmandList->ResourceBarrier(1, &barrierDesc);

		//命令のクローズ
		result = cmmandList->Close();
		assert(SUCCEEDED(result));
		//コマンドリストの実行
		ID3D12CommandList* cmmandListts[] = { cmmandList };
		cmmandQueue->ExecuteCommandLists(1, cmmandListts);

		//フリップ
		result = swapChain->Present(1, 0);
		assert(SUCCEEDED(result));

		//コマンド実行完了を待つ
		cmmandQueue->Signal(fence, ++fenceVal);
		if (fence->GetCompletedValue() != fenceVal)
		{
			HANDLE event = CreateEvent(nullptr, false, false, nullptr);
			fence->SetEventOnCompletion(fenceVal, event);
			if (event != 0)
			{
				WaitForSingleObject(event, INFINITE);
				CloseHandle(event);
			}
		}

		//キューをクリア
		result = cmdAllocator->Reset();
		assert(SUCCEEDED(result));
		//コマンドリストを貯める準備
		result = cmmandList->Reset(cmdAllocator, nullptr);
		assert(SUCCEEDED(result));

#pragma endregion
	}

	//クラス使用しないため登録解除
	UnregisterClass(w.lpszClassName, w.hInstance);

	return 0;
}