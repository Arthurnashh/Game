#include "precomp.h"

// Copyright (c) 2007-2011 Simul Software Ltd
// All Rights Reserved.
//
// This source code is supplied under the terms of a license agreement or
// nondisclosure agreement with Simul Software Ltd and may not 
// be copied or disclosed except in accordance with the terms of that 
// agreement.

#include "BaseLightningRenderer.h"

using namespace simul;
using namespace clouds;

BaseLightningRenderer::BaseLightningRenderer(simul::clouds::LightningRenderInterface *lri)
{
	lightningRenderInterface=lri;
}

BaseLightningRenderer::~BaseLightningRenderer()
{
}
