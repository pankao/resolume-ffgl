#pragma once
#include <string>
#include <FFGLSDK.h>

class TimeWave : public CFFGLPlugin
{
public:
	TimeWave();
	~TimeWave();

	//CFFGLPlugin
	FFResult InitGL( const FFGLViewportStruct* vp ) override;
	FFResult ProcessOpenGL( ProcessOpenGLStruct* pGL ) override;
	FFResult DeInitGL() override;
	unsigned int Resize( const FFGLViewportStruct* vp ) override;

	FFResult SetFloatParameter( unsigned int dwIndex, float value ) override;

	float GetFloatParameter( unsigned int index ) override;

private:
	struct WaveTarget
	{
		WaveTarget();

		bool Initialise( GLsizei width, GLsizei height );
		void Release();
		bool IsReady() const;

		GLuint fboID;
		GLuint heightTextureID;
		GLuint velocityTextureID;
		GLsizei width;
		GLsizei height;
	};

	bool EnsureWaveTargets( const FFGLTextureStruct& inputTexture );
	bool UpdateWaveState( const FFGLTextureStruct& inputTexture, const FFGLTexCoords& inputMaxCoords );
	void RenderOutput( const FFGLTextureStruct& inputTexture, const FFGLTexCoords& inputMaxCoords );
	void ReleaseWaveTargets();

	ffglex::FFGLShader updateShader;//!< Advances the persistent wave height and velocity textures.
	ffglex::FFGLShader outputShader;//!< Renders the final premultiplied image back to the host.
	ffglex::FFGLScreenQuad quad;//!< Utility to help us render a full screen quad.
	WaveTarget waveTargets[ 2 ];

	float amount;
	float feedback;
	float friction;
	float decay;
	float speed;
	float waveScale;
	float blur;
	float spread;
	float phase;
	float warp;
	float contrast;
	float resolutionMode;
	int frontTargetIndex;
	bool stateInitialized;
	bool resetRequested;
};
