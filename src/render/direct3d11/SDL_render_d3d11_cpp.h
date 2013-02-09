/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2012 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "SDL_config.h"

#include <D3D11_1.h>
#include <DirectXMath.h>
#include <wrl/client.h>

typedef struct
{
    Microsoft::WRL::ComPtr<ID3D11Device1> d3dDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext1> d3dContext;
    Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;
    Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> mainSampler;
    D3D_FEATURE_LEVEL featureLevel;
    unsigned int vertexCount;
    bool loadingComplete;

    // Cached renderer properties.
    DirectX::XMFLOAT2 windowSizeInDIPs;
    DirectX::XMFLOAT2 renderTargetSize;
    Windows::Graphics::Display::DisplayOrientations orientation;

    // Transform used for display orientation.
    DirectX::XMFLOAT4X4 orientationTransform3D;
} D3D11_RenderData;

typedef struct
{
    Microsoft::WRL::ComPtr<ID3D11Texture2D> mainTexture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mainTextureResourceView;
    SDL_PixelFormat * pixelFormat;
} D3D11_TextureData;

struct VertexPositionColor
{
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT2 tex;
};

/* vi: set ts=4 sw=4 expandtab: */
