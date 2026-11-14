//-----------------------------------------------------------------------------
// File: enf_rendrendertarget.cpp
//
// Desc: Render target abstraction
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

//#include "precomp.h"
#include "render/enf_irendmain.h"
#include "render/enf_texture.h"

namespace enf
{

//-----------------------------------------------------------------------
RenderTarget::RenderTarget(IRenderer *dev, uint width, uint height, TextureFormat rtfmt, TextureFormat depthfmt, uint edramoffset):
	Texture(dev, NULL, width, height, 0),
#ifdef ENF_DX9
	m_pRenderSurface(NULL),
	m_pDepthSurface(NULL),
#elif defined(ENF_OGL)
	m_iFrameBuffer(0),
	m_iDepthBuffer(0),
#endif
	m_DepthFormat(depthfmt),
	m_iUsage(0)
{
	#if defined(ENF_DX9)
		m_bManaged = false;
	#endif
	m_Format = rtfmt;
}

//-----------------------------------------------------------------------
void RenderTarget::Free()
{
	enf_rlsassert(GetDevice()->CheckThread());
	MemStats::m_iRenderTargets -= GetMemorySize();

	#ifdef ENF_DX9
		uint c;

		if(m_pDepthSurface)
		{
			c = m_pDepthSurface->Release();
			enf_assert(c == 0);
			m_pDepthSurface = NULL;
		}

		//don't need to check, as we are referenced by texture anyway
		SAFE_RELEASE(m_pRenderSurface);

		if(m_pTexture)
		{
			c = m_pTexture->Release();
			enf_assert(c == 0);
			m_pTexture = NULL;
		}
	#elif defined(ENF_OGL)
		if(m_iDepthBuffer != 0)
		{
			glDeleteRenderbuffersEXT(1, &m_iDepthBuffer);
			m_iDepthBuffer = 0;
		}
		if(m_iTexture != 0)
		{
			glDeleteTextures(1, &m_iTexture);
			m_iTexture = 0;
		}
	#endif
}

//-----------------------------------------------------------------------
size_t RenderTarget::GetMemorySize() const
{
	size_t size = Texture::GetMemorySize();
	#ifdef ENF_DX9
		if(m_DepthFormat)
		{
			size += GetSurfaceSize(m_iWidth, m_iHeight, m_DepthFormat);
		}
	#endif
	return size;
}

//-----------------------------------------------------------------------
ERESULT RenderTarget::Allocate()
{
	#if defined(ENF_DX9) 

		m_pDepthSurface = NULL;
		m_pRenderSurface = NULL;
		m_pTexture = NULL;

		if(m_Format != D3DFMT_UNKNOWN)
		{
			uint miplevels = 1;
			uint usage = m_iUsage;

			if(m_iUsage & RES_USAGE_AUTOGENMIPMAP)
			{
	//			HRESULT hr = m_pDevice->m_pD3D->CheckDeviceFormat(m_pDevice->m_iAdapter, D3DDEVTYPE_HAL, m_pDevice->m_d3dpp.BackBufferFormat, usage, D3DRTYPE_TEXTURE, m_Format);
	//			hr = m_pDevice->m_pD3D->CheckDeviceFormat(m_pDevice->m_iAdapter, D3DDEVTYPE_HAL, m_pDevice->m_d3dpp.BackBufferFormat, usage, D3DRTYPE_TEXTURE, D3DFMT_A8R8G8B8);
	//			hr = m_pDevice->m_pD3D->CheckDeviceFormat(m_pDevice->m_iAdapter, D3DDEVTYPE_HAL, m_pDevice->m_d3dpp.BackBufferFormat, usage & (~RES_USAGE_AUTOGENMIPMAP), D3DRTYPE_TEXTURE, m_Format);

				if(/*hr == D3D_OK &&*/ GetDevice()->m_d3dCaps.Caps2 & D3DCAPS2_CANAUTOGENMIPMAP && !GetDevice()->m_bATICrap)
					miplevels = 0;
				else
					usage &= ~RES_USAGE_AUTOGENMIPMAP;
			}

			IDirect3DBaseTexture9* pTexture = NULL;
			uint attempts = 0;
			do
			{
				if(m_iFlags & TEXTR_CUBE)
				{
					if(GetDevice()->GetDXDevice()->CreateCubeTexture(m_iWidth, miplevels, usage, (D3DFORMAT)m_Format, D3DPOOL_DEFAULT, (IDirect3DCubeTexture9**)&pTexture, NULL) == D3D_OK)
						break;
				}
				else
				{
					if(GetDevice()->GetDXDevice()->CreateTexture(m_iWidth, m_iHeight, miplevels, usage, (D3DFORMAT)m_Format, D3DPOOL_DEFAULT, (IDirect3DTexture9**)&pTexture, NULL) == D3D_OK)
						break;
				}
				if(++attempts > 4)
					break;
			} while(MemoryManager::OutOfMemory(__FILE__, __LINE__, MT_GENERAL|MT_VIDEO, GetMemorySize()));
		
			m_pTexture = static_cast<_D3DTexture*>(pTexture);

			if(m_pTexture == NULL)
				return EE_ERROR;

			if(m_FSAAType != FSAATYPE_NONE)
			{
				if(GetDevice()->GetDXDevice()->CreateRenderTarget(m_iWidth, m_iHeight, (D3DFORMAT)m_Format, (D3DMULTISAMPLE_TYPE)m_FSAAType, 0, 0, &m_pRenderSurface, NULL) != D3D_OK)
				{
					return EE_ERROR;
				}
			}
			else
			{
				if(m_pTexture && (m_iFlags & TEXTR_CUBE) == 0)
				{
					m_pTexture->GetSurfaceLevel(0, &m_pRenderSurface);
				
					#ifdef ENF_STEREO_VISION
						if (m_iFlags & TEXTR_STEREO && GetRender()->GetStereoInfo().Available)
						{
							_D3DSurface *offSurface = NULL;
							GetRender()->GetDXDevice()->CreateOffscreenPlainSurface(m_iWidth*2, m_iHeight + 1, (D3DFORMAT) m_Format, D3DPOOL_SYSTEMMEM, &offSurface, NULL);

							D3DLOCKED_RECT lrect;
							if (offSurface && offSurface->LockRect(&lrect, NULL, 0) == D3D_OK)
							{
								/*
								//to test validity of stereo texture in pixel shader, using anaglyfic glasses the result should be magenta
								uint *ch1 = (uint*) lrect.pBits;
								uint *ch2 = (uint*) ((uchar*) lrect.pBits + m_iWidth*GetFormatStride(m_Format));
								for (uint i = 0; i < m_iWidth; i++)
								{
									ch1[i] = 0xFFFF0000;
									ch2[i] = 0xFF0000FF;
								}
								*/


								IRenderer::NVStereoImageHeader* signature = (IRenderer::NVStereoImageHeader*) ((uchar*) lrect.pBits + m_iHeight*lrect.Pitch);
								signature->Init(m_iWidth*2, m_iHeight, 8*GetFormatStride(m_Format));
								offSurface->UnlockRect();

								RECT stereoSrcRect = {0, 0, m_iWidth, m_iHeight};
								POINT stereoDstPoint = {0, 0};

								if (GetRender()->GetDXDevice()->UpdateSurface( offSurface, &stereoSrcRect, m_pRenderSurface, &stereoDstPoint ) != D3D_OK)
								{
									#ifdef ENF_DEVELOPER
										LogF(LL_ERROR, "NVIDIA stereo: Cannot create stereo texture (%s)", m_strName.IsEmpty() ? "empty name": m_strName);
									#endif
								}
							}

							if (offSurface)
							{
								offSurface->Release();
							}
						}
					#endif
				}
			}
		}

		if(m_DepthFormat != TEXFMT_UNKNOWN)
		{
			uint attempts = 0;
			do 
			{
				if(GetDevice()->GetDXDevice()->CreateDepthStencilSurface( m_iWidth, m_iHeight,
											(D3DFORMAT)m_DepthFormat,
											(D3DMULTISAMPLE_TYPE)m_FSAAType,0,false,
											&m_pDepthSurface, NULL) == D3D_OK)
					break;
				if(++attempts > 4)
					break;
			} while(MemoryManager::OutOfMemory(__FILE__, __LINE__, MT_VIDEO, GetMemorySize()));

			if(m_pDepthSurface == NULL)
				return EE_ERROR;
		}

	#elif defined(ENF_OGL)

		GLuint renderedTexture;
		GLCALL(glGenTextures(1, &renderedTexture));
		GLCALL(glBindTexture(GL_TEXTURE_2D, renderedTexture));
		GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0));
	
		GLenum dataFormat, dataType;
		GLint glfmt = TexFormatToGLFormatEx(m_Format, dataFormat, dataType, true);
		GLCALL(glTexImage2D(GL_TEXTURE_2D, 0, glfmt, m_iWidth, m_iHeight, 0, dataFormat, dataType, 0));

		GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP));
		GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP));
		// Poor filtering. Needed !
		GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
		GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));

		if(m_DepthFormat != TEXFMT_UNKNOWN)
		{
			GLuint depthrenderbuffer;
			GLCALL(glGenRenderbuffers(1, &depthrenderbuffer));
			GLCALL(glBindRenderbuffer(GL_RENDERBUFFER, depthrenderbuffer));
			GLCALL(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, m_iWidth, m_iHeight));
			m_iDepthBuffer = depthrenderbuffer;
		}

		m_iTexture = renderedTexture;
		// Set "renderedTexture" as our colour attachement #0
		//GLCALL(glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, renderedTexture, 0));

		//GL_CHECKFBO(GL_FRAMEBUFFER_EXT)

		
		GLint primaryBackBuffer;
		GLint primaryDepthBuffer;


	//	GLCALL(glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &primaryBackBuffer));
		//GLCALL(glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &primaryDepthBuffer));

		//GLCALL(glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_BACK_RIGHT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &primaryBackBuffer));
		//GLCALL(glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_DEPTH, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &primaryDepthBuffer));
		//GLCALL(glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &primaryDepthBuffer));

		GLCALL(glBindRenderbuffer(GL_RENDERBUFFER, 0));
		GLCALL(glBindTexture(GL_TEXTURE_2D, 0));

		GetDevice()->DirtyMask(Renderer::GLCM_TEXTURES|Renderer::GLCM_SAMPLERFILTER|Renderer::GLCM_FBO);

	#endif

	MemStats::m_iRenderTargets += GetMemorySize();
	return EE_OK;
}

//-----------------------------------------------------------------------
RenderTarget* IRenderer::CreateRenderTarget(uint width, uint height, TextureFormat rtfmt, TextureFormat depthfmt, FSAAType fsaa, uint usage, uint edramoffset, uint texFlags, CStr name)
{
	enf_rlsassert(CheckThread());

	RenderTarget *rt = ENF_NEW RenderTarget(this, width, height, rtfmt, depthfmt, edramoffset);
	rt->m_FSAAType = fsaa;
	rt->m_iUsage = usage | RES_USAGE_RENDERTARGET;
	rt->m_iFlags |= texFlags;
	#ifdef ENF_DEVELOPER
		rt->m_strName = name;
	#endif

	if(rt->Allocate() != EE_OK)
	{
		rt->Free();
		SAFE_DELETE(rt);
		return NULL;
	}

	AddResource(rt);

	return rt;
}

//-----------------------------------------------------------------------
void RenderTarget::Resolve(uint rt, const RRect* rect, const RPoint* point)
{
	enf_assert(rt < 4);

	#if defined(ENF_DX9)
		if(m_FSAAType != FSAATYPE_NONE)
		{
			ComPtr<IDirect3DSurface9> surf;
			if(GetD3DTexture()->GetSurfaceLevel(0, surf.Init()) == D3D_OK)
			{
				//TODO: not tested
				RECT d3drect1, d3drect2;
				if (rect)
				{
					d3drect1.left   = rect->Left;
					d3drect1.right  = rect->Right;
					d3drect1.top    = rect->Top;
					d3drect1.bottom = rect->Bottom;
				}

				if (point)
				{
					enf_assert(rect);

					d3drect2.left   = point->X;
					d3drect2.right  = point->X + rect->Right - rect->Left;
					d3drect2.top    = point->Y;
					d3drect2.bottom = point->Y + rect->Bottom - rect->Top;
				}

				HRESULT hr = GetDevice()->GetDXDevice()->StretchRect(m_pRenderSurface, rect ? &d3drect1 : NULL, surf, point ? &d3drect2 : rect ? &d3drect1 : NULL, D3DTEXF_NONE);
				enf_assert(hr == D3D_OK);
			}
		}
	#elif defined(ENF_OGL)
	#endif
}





}