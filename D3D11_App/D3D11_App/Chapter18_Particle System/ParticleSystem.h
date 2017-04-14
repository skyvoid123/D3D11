#pragma once

#include "d3dUtil.h"

class Camera;
class ParticleEffect;

class ParticleSystem
{
public:
	ParticleSystem();
	~ParticleSystem();

	// Time elapsed since the system was reset.
	float GetAge() const
	{
		return m_Age;
	}

	void SetEyePos(const XMFLOAT3& eyePosW)
	{
		m_EyePosW = eyePosW;
	}

	void SetEmitPos(const XMFLOAT3& emitPosW)
	{
		m_EmitPosW = emitPosW;
	}


	void SetEmitDir(const XMFLOAT3& emitDirW)
	{
		m_EmitDirW = emitDirW;
	}

	void Init(ID3D11Device* device, ParticleEffect* fx, ID3D11ShaderResourceView* texSRV,
		ID3D11ShaderResourceView* randomTexSRV, UINT maxParticles);

	void Reset();
	void Update(float dt, float gameTime);
	void Draw(ID3D11DeviceContext* dc, const Camera& cam);
	 
private:
	void BuildVB(ID3D11Device* device);

	ParticleSystem(const ParticleSystem& rhs);
	ParticleSystem& operator=(const ParticleSystem& rhs);

private:
	UINT m_MaxParticles;
	bool m_IsFirstRun;

	float m_GameTime;
	float m_TimeStep;
	float m_Age;

	XMFLOAT3 m_EyePosW;
	XMFLOAT3 m_EmitPosW;
	XMFLOAT3 m_EmitDirW;

	ParticleEffect* m_Fx;

	ID3D11Buffer* m_InitVB;
	ID3D11Buffer* m_DrawVB;
	ID3D11Buffer* m_StreamOutVB;

	ID3D11ShaderResourceView* m_TexSRV;
	ID3D11ShaderResourceView* m_RandomTexSRV;
};