#include "ParticleSystem.h"
#include "Vertex.h"
#include "Effects.h"
#include "Camera.h"

ParticleSystem::ParticleSystem()
	: m_IsFirstRun(true)
	, m_GameTime(0.0f)
	, m_TimeStep(0.0f)
	, m_Age(0.0f)
	, m_EyePosW(0.0f, 0.0f, 0.0f)
	, m_EmitPosW(0.0f, 0.0f, 0.0f)
	, m_EmitDirW(0.0f, 1.0f, 0.0f)
	, m_InitVB(nullptr)
	, m_DrawVB(nullptr)
	, m_StreamOutVB(nullptr)
	, m_TexSRV(nullptr)
	, m_RandomTexSRV(nullptr)
{
	
}

ParticleSystem::~ParticleSystem()
{
	ReleaseCOM(m_InitVB);
	ReleaseCOM(m_DrawVB);
	ReleaseCOM(m_StreamOutVB);
}

void ParticleSystem::Init(ID3D11Device* device, ParticleEffect* fx, ID3D11ShaderResourceView* texSRV,
	ID3D11ShaderResourceView* randomTexSRV, UINT maxParticles)
{
	m_MaxParticles = maxParticles;
	m_Fx = fx;
	m_TexSRV = texSRV;
	m_RandomTexSRV = randomTexSRV;
	
	BuildVB(device);
}

void ParticleSystem::Reset()
{
	m_IsFirstRun = true;
	m_Age = 0.0f;
}

void ParticleSystem::Update(float dt, float gameTime)
{
	m_GameTime = gameTime;
	m_TimeStep = dt;
	m_Age += dt;
}

void ParticleSystem::BuildVB(ID3D11Device* device)
{
	//
	// Create the buffer to kick-off the particle system.
	//

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_DEFAULT;
	vbd.ByteWidth = sizeof(Vertex::Particle);
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;

	// The initial particle emitter has type 0 and age 0.  The rest
	// of the particle attributes do not apply to an emitter.
	Vertex::Particle p;
	ZeroMemory(&p, sizeof(Vertex::Particle));
	p.Age = 0.0f;
	p.Type = 0;

	D3D11_SUBRESOURCE_DATA vData;
	vData.pSysMem = &p;

	HR(device->CreateBuffer(&vbd, &vData, &m_InitVB));

	//
	// Create the ping-pong buffers for stream-out and drawing.
	//
	vbd.ByteWidth = sizeof(Vertex::Particle) * m_MaxParticles;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_STREAM_OUTPUT;
	
	HR(device->CreateBuffer(&vbd, nullptr, &m_DrawVB));
	HR(device->CreateBuffer(&vbd, nullptr, &m_StreamOutVB));
}

void ParticleSystem::Draw(ID3D11DeviceContext* dc, const Camera& cam)
{
	XMMATRIX vp = cam.ViewProj();

	//
	// Set constants.
	//
	m_Fx->SetViewProj(vp);
	m_Fx->SetGameTime(m_GameTime);
	m_Fx->SetTimeStep(m_TimeStep);
	m_Fx->SetEyePosW(m_EyePosW);
	m_Fx->SetEmitPosW(m_EmitPosW);
	m_Fx->SetEmitDirW(m_EmitDirW);
	m_Fx->SetTexArray(m_TexSRV);
	m_Fx->SetRandomTex(m_RandomTexSRV);

	//
	// Set IA stage.
	//
	dc->IASetInputLayout(InputLayouts::Particle);
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	UINT stride = sizeof(Vertex::Particle);
	UINT offset = 0;

	// On the first pass, use the initialization VB.  Otherwise, use
	// the VB that contains the current particle list.
	if (m_IsFirstRun)
	{
		dc->IASetVertexBuffers(0, 1, &m_InitVB, &stride, &offset);
	}
	else
	{
		dc->IASetVertexBuffers(0, 1, &m_DrawVB, &stride, &offset);
	}

	//
	// Draw the current particle list using stream-out only to update them.  
	// The updated vertices are streamed-out to the target VB. 
	//
	dc->SOSetTargets(1, &m_StreamOutVB, &offset);

	D3DX11_TECHNIQUE_DESC techDesc;
	m_Fx->StreamOutTech->GetDesc(&techDesc);
	for (int p = 0; p < techDesc.Passes; ++p)
	{
		m_Fx->StreamOutTech->GetPassByIndex(p)->Apply(0, dc);

		if (m_IsFirstRun)
		{
			dc->Draw(1, 0);
			m_IsFirstRun = false;
		}
		else
		{
			dc->DrawAuto();
		}
	}

	// done streaming-out--unbind the vertex buffer
	ID3D11Buffer* tempBuffer = nullptr;
	dc->SOSetTargets(1, &tempBuffer, &offset);

	// ping-pong the vertex buffers
	std::swap(m_DrawVB, m_StreamOutVB);

	//
	// Draw the updated particle system we just streamed-out. 
	//
	dc->IASetVertexBuffers(0, 1, &m_DrawVB, &stride, &offset);

	m_Fx->DrawTech->GetDesc(&techDesc);
	for (int p = 0; p < techDesc.Passes; ++p)
	{
		m_Fx->DrawTech->GetPassByIndex(p)->Apply(0, dc);
		dc->DrawAuto();
	}
}