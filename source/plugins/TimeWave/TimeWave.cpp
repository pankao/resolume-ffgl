/*
	TimeWave

	A modern GPU FFGL effect inspired by Pete Warden's classic FreeFrame Wave.
	Pete's original CPU plugin treated the image as a damped temporal wave
	surface rather than a simple trail buffer; TimeWave keeps that artistic idea
	but implements the state update with persistent OpenGL textures.

	Original Wave concept: Pete Warden
	GPU FFGL port: Igor Teichman, alias VJ Double Slit <it.ten.tackes@gmail.com>
*/

#include "TimeWave.h"
#include "ffglex/FFGLScopedFBOBinding.h"
using namespace ffglex;

enum ParamType : FFUInt32
{
	PT_AMOUNT,
	PT_FEEDBACK,
	PT_FRICTION,
	PT_DECAY,
	PT_SPEED,
	PT_WAVE_SCALE,
	PT_BLUR,
	PT_SPREAD,
	PT_PHASE,
	PT_WARP,
	PT_CONTRAST,
	PT_RESOLUTION,
	PT_RESET
};

static CFFGLPluginInfo PluginInfo(
	PluginFactory< TimeWave >,// Create method
	"ITTW",                      // Plugin unique ID of maximum length 4.
	"TimeWave",                 // Plugin name
	2,                           // API major version number
	1,                           // API minor version number
	1,                           // Plugin major version number
	0,                           // Plugin minor version number
	FF_EFFECT,                   // Plugin type
	"GPU temporal wave feedback inspired by Pete Warden's FreeFrame Wave",// Plugin description
	"TimeWave GPU FFGL port by Igor Teichman / VJ Double Slit. Original Wave concept by Pete Warden."// About
);

static const char _vertexShaderCode[] = R"(#version 410 core
uniform vec2 InputMaxUV;

layout( location = 0 ) in vec4 vPosition;
layout( location = 1 ) in vec2 vUV;

out vec2 inputUV;
out vec2 stateUV;

void main()
{
	gl_Position = vPosition;
	stateUV = vUV;
	inputUV = vUV * InputMaxUV;
}
)";

static const char _updateFragmentShaderCode[] = R"(#version 410 core
uniform sampler2D InputTexture;
uniform sampler2D PreviousHeightTexture;
uniform sampler2D PreviousVelocityTexture;
uniform vec2 TexelSize;
uniform float Feedback;
uniform float Friction;
uniform float Decay;
uniform float Speed;
uniform float WaveScale;
uniform float Blur;
uniform float Spread;
uniform bool ResetState;

in vec2 inputUV;
in vec2 stateUV;

layout( location = 0 ) out vec4 outHeight;
layout( location = 1 ) out vec4 outVelocity;

vec4 sampleStraightInput()
{
	vec4 color = texture( InputTexture, inputUV );
	if( color.a > 0.0 )
		color.rgb /= color.a;
	color.rgb = clamp( color.rgb, vec3( 0.0 ), vec3( 1.0 ) );
	return color;
}

void main()
{
	vec4 source = sampleStraightInput();

	if( ResetState )
	{
		outHeight = vec4( source.rgb, source.a );
		outVelocity = vec4( 0.0 );
		return;
	}

	float sampleRadius = mix( 0.5, 10.0, WaveScale * WaveScale );
	vec2 stepX = vec2( TexelSize.x * sampleRadius, 0.0 );
	vec2 stepY = vec2( 0.0, TexelSize.y * sampleRadius );

	vec3 center = texture( PreviousHeightTexture, stateUV ).rgb;
	vec3 left = texture( PreviousHeightTexture, stateUV - stepX ).rgb;
	vec3 right = texture( PreviousHeightTexture, stateUV + stepX ).rgb;
	vec3 down = texture( PreviousHeightTexture, stateUV - stepY ).rgb;
	vec3 up = texture( PreviousHeightTexture, stateUV + stepY ).rgb;

	vec3 cardinalAverage = ( left + right + down + up ) * 0.25;

	vec3 diagonalA = texture( PreviousHeightTexture, stateUV - stepX - stepY ).rgb;
	vec3 diagonalB = texture( PreviousHeightTexture, stateUV + stepX - stepY ).rgb;
	vec3 diagonalC = texture( PreviousHeightTexture, stateUV - stepX + stepY ).rgb;
	vec3 diagonalD = texture( PreviousHeightTexture, stateUV + stepX + stepY ).rgb;

	vec3 eightWayAverage = ( left + right + down + up + diagonalA + diagonalB + diagonalC + diagonalD ) * 0.125;

	// Pete Warden's original FreeFrame Wave treated the image as a damped
	// spring surface: the source and neighboring heights define a desired
	// height, then velocity carries the pixel toward that target over multiple
	// frames. Spread controls how strongly neighboring pixels pull on the
	// current cell; Feedback controls how much the old wave state wins over the
	// live source.
	vec3 neighborField = mix( cardinalAverage, eightWayAverage, Spread );
	vec3 propagated = mix( center, neighborField, Spread );
	vec3 desired = mix( source.rgb, propagated, Feedback );

	vec3 softened = ( center + left + right + down + up + diagonalA + diagonalB + diagonalC + diagonalD ) / 9.0;
	desired = mix( desired, ( desired + softened ) * 0.5, Blur );

	vec3 velocity = texture( PreviousVelocityTexture, stateUV ).rgb;
	velocity = ( velocity + ( desired - center ) * Speed ) * Friction;

	vec3 nextHeight = ( center + velocity ) * Decay;

	// Keep the oscillator bounded enough for live use while still allowing
	// overshoot, which is where the dreamy wave feel comes from.
	outHeight = vec4( clamp( nextHeight, vec3( -1.0 ), vec3( 2.0 ) ), source.a );
	outVelocity = vec4( clamp( velocity, vec3( -1.0 ), vec3( 1.0 ) ), 0.0 );
}
)";

static const char _outputFragmentShaderCode[] = R"(#version 410 core
uniform sampler2D InputTexture;
uniform sampler2D HeightTexture;
uniform sampler2D VelocityTexture;
uniform vec2 InputMaxUV;
uniform vec2 TexelSize;
uniform float Amount;
uniform float Phase;
uniform float Warp;
uniform float Contrast;

in vec2 inputUV;
in vec2 stateUV;

out vec4 fragColor;

void main()
{
	float left = dot( texture( HeightTexture, stateUV - vec2( TexelSize.x, 0.0 ) ).rgb, vec3( 0.299, 0.587, 0.114 ) );
	float right = dot( texture( HeightTexture, stateUV + vec2( TexelSize.x, 0.0 ) ).rgb, vec3( 0.299, 0.587, 0.114 ) );
	float down = dot( texture( HeightTexture, stateUV - vec2( 0.0, TexelSize.y ) ).rgb, vec3( 0.299, 0.587, 0.114 ) );
	float up = dot( texture( HeightTexture, stateUV + vec2( 0.0, TexelSize.y ) ).rgb, vec3( 0.299, 0.587, 0.114 ) );
	vec2 waveGradient = vec2( right - left, up - down );
	vec2 warpedInputUV = clamp( inputUV + waveGradient * InputMaxUV * Warp * 0.08, vec2( 0.0 ), InputMaxUV );

	vec4 color = texture( InputTexture, warpedInputUV );
	//The InputTexture contains premultiplied colors, so we need to unpremultiply first to apply our effect on straight colors.
	if( color.a > 0.0 )
		color.rgb /= color.a;

	vec3 waveHeight = texture( HeightTexture, stateUV ).rgb;
	vec3 waveVelocity = texture( VelocityTexture, stateUV ).rgb;
	vec3 waveColor = clamp( waveHeight + waveVelocity * ( Phase * 2.0 - 1.0 ), vec3( 0.0 ), vec3( 1.0 ) );
	color.rgb = mix( color.rgb, waveColor, clamp( Amount, 0.0, 1.0 ) );
	color.rgb = clamp( ( color.rgb - 0.5 ) * exp2( ( Contrast - 0.5 ) * 3.0 ) + 0.5, vec3( 0.0 ), vec3( 1.0 ) );

	//The plugin has to output premultiplied colors, this is how we're premultiplying our straight color while also
	//ensuring we aren't going out of the LDR the video engine is working in.
	color.rgb = clamp( color.rgb * color.a, vec3( 0.0 ), vec3( color.a ) );
	fragColor = color;
}
)";

TimeWave::TimeWave() :
	amount( 0.75f ),
	feedback( 0.7f ),
	friction( 0.95f ),
	decay( 0.995f ),
	speed( 0.2f ),
	waveScale( 0.25f ),
	blur( 0.0f ),
	spread( 0.65f ),
	phase( 0.5f ),
	warp( 0.15f ),
	contrast( 0.5f ),
	resolutionMode( 0.0f ),
	frontTargetIndex( 0 ),
	stateInitialized( false ),
	resetRequested( true )
{
	SetMinInputs( 1 );
	SetMaxInputs( 1 );

	SetParamInfof( PT_AMOUNT, "Amount", FF_TYPE_STANDARD );
	SetParamInfof( PT_FEEDBACK, "Feedback", FF_TYPE_STANDARD );
	SetParamInfof( PT_FRICTION, "Friction", FF_TYPE_STANDARD );
	SetParamInfof( PT_DECAY, "Decay", FF_TYPE_STANDARD );
	SetParamInfof( PT_SPEED, "Speed", FF_TYPE_STANDARD );
	SetParamInfof( PT_WAVE_SCALE, "Wave Scale", FF_TYPE_STANDARD );
	SetParamInfof( PT_BLUR, "Blur", FF_TYPE_STANDARD );
	SetParamInfof( PT_SPREAD, "Spread", FF_TYPE_STANDARD );
	SetParamInfof( PT_PHASE, "Phase", FF_TYPE_STANDARD );
	SetParamInfof( PT_WARP, "Warp", FF_TYPE_STANDARD );
	SetParamInfof( PT_CONTRAST, "Contrast", FF_TYPE_STANDARD );
	SetOptionParamInfo( PT_RESOLUTION, "Resolution", 3, resolutionMode );
	SetParamElementInfo( PT_RESOLUTION, 0, "Full", 0.0f );
	SetParamElementInfo( PT_RESOLUTION, 1, "Half", 1.0f );
	SetParamElementInfo( PT_RESOLUTION, 2, "Quarter", 2.0f );
	SetParamInfof( PT_RESET, "Reset", FF_TYPE_EVENT );

	FFGLLog::LogToHost( "Created TimeWave effect" );
}
TimeWave::~TimeWave()
{
}

FFResult TimeWave::InitGL( const FFGLViewportStruct* vp )
{
	if( !updateShader.Compile( _vertexShaderCode, _updateFragmentShaderCode ) )
	{
		DeInitGL();
		return FF_FAIL;
	}
	if( !outputShader.Compile( _vertexShaderCode, _outputFragmentShaderCode ) )
	{
		DeInitGL();
		return FF_FAIL;
	}
	if( !quad.Initialise() )
	{
		DeInitGL();
		return FF_FAIL;
	}

	//Use base-class init as success result so that it retains the viewport.
	return CFFGLPlugin::InitGL( vp );
}
FFResult TimeWave::ProcessOpenGL( ProcessOpenGLStruct* pGL )
{
	if( pGL->numInputTextures < 1 )
		return FF_FAIL;

	if( pGL->inputTextures[ 0 ] == NULL )
		return FF_FAIL;

	if( !EnsureWaveTargets( *pGL->inputTextures[ 0 ] ) )
		return FF_FAIL;

	//The input texture's dimension might change each frame and so might the content area.
	//We're adopting the texture's maxUV using a uniform because that way we dont have to update our vertex buffer each frame.
	FFGLTexCoords maxCoords = GetMaxGLTexCoords( *pGL->inputTextures[ 0 ] );

	if( !UpdateWaveState( *pGL->inputTextures[ 0 ], maxCoords ) )
		return FF_FAIL;
	RenderOutput( *pGL->inputTextures[ 0 ], maxCoords );

	return FF_SUCCESS;
}
FFResult TimeWave::DeInitGL()
{
	ReleaseWaveTargets();
	updateShader.FreeGLResources();
	outputShader.FreeGLResources();
	quad.Release();

	return FF_SUCCESS;
}

unsigned int TimeWave::Resize( const FFGLViewportStruct* vp )
{
	ReleaseWaveTargets();
	stateInitialized = false;
	resetRequested   = true;
	return CFFGLPlugin::Resize( vp );
}

FFResult TimeWave::SetFloatParameter( unsigned int dwIndex, float value )
{
	switch( dwIndex )
	{
	case PT_AMOUNT:
		amount = value;
		break;
	case PT_FEEDBACK:
		feedback = value;
		break;
	case PT_FRICTION:
		friction = value;
		break;
	case PT_DECAY:
		decay = value;
		break;
	case PT_SPEED:
		speed = value;
		break;
	case PT_WAVE_SCALE:
		waveScale = value;
		break;
	case PT_BLUR:
		blur = value;
		break;
	case PT_SPREAD:
		spread = value;
		break;
	case PT_PHASE:
		phase = value;
		break;
	case PT_WARP:
		warp = value;
		break;
	case PT_CONTRAST:
		contrast = value;
		break;
	case PT_RESOLUTION:
		if( resolutionMode != value )
		{
			resolutionMode = value;
			stateInitialized = false;
			resetRequested   = true;
		}
		break;
	case PT_RESET:
		if( value != 0.0f )
			resetRequested = true;
		break;

	default:
		return FF_FAIL;
	}

	return FF_SUCCESS;
}

float TimeWave::GetFloatParameter( unsigned int index )
{
	switch( index )
	{
	case PT_AMOUNT:
		return amount;
	case PT_FEEDBACK:
		return feedback;
	case PT_FRICTION:
		return friction;
	case PT_DECAY:
		return decay;
	case PT_SPEED:
		return speed;
	case PT_WAVE_SCALE:
		return waveScale;
	case PT_BLUR:
		return blur;
	case PT_SPREAD:
		return spread;
	case PT_PHASE:
		return phase;
	case PT_WARP:
		return warp;
	case PT_CONTRAST:
		return contrast;
	case PT_RESOLUTION:
		return resolutionMode;
	case PT_RESET:
		return 0.0f;
	}

	return 0.0f;
}

TimeWave::WaveTarget::WaveTarget() :
	fboID( 0 ),
	heightTextureID( 0 ),
	velocityTextureID( 0 ),
	width( 0 ),
	height( 0 )
{
}

bool TimeWave::WaveTarget::Initialise( GLsizei targetWidth, GLsizei targetHeight )
{
	Release();

	width  = targetWidth;
	height = targetHeight;

	glGenFramebuffers( 1, &fboID );
	glGenTextures( 1, &heightTextureID );
	glGenTextures( 1, &velocityTextureID );

	if( fboID == 0 || heightTextureID == 0 || velocityTextureID == 0 )
	{
		Release();
		return false;
	}

	const GLuint textures[] = { heightTextureID, velocityTextureID };
	for( GLuint textureID : textures )
	{
		Scoped2DTextureBinding textureBinding( textureID );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	}

	ScopedFBOBinding fboBinding( fboID, ScopedFBOBinding::RB_REVERT );
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, heightTextureID, 0 );
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, velocityTextureID, 0 );

	const GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers( 2, drawBuffers );

	if( glCheckFramebufferStatus( GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE )
	{
		Release();
		return false;
	}

	return true;
}

void TimeWave::WaveTarget::Release()
{
	if( fboID != 0 )
	{
		glDeleteFramebuffers( 1, &fboID );
		fboID = 0;
	}

	if( heightTextureID != 0 )
	{
		glDeleteTextures( 1, &heightTextureID );
		heightTextureID = 0;
	}

	if( velocityTextureID != 0 )
	{
		glDeleteTextures( 1, &velocityTextureID );
		velocityTextureID = 0;
	}

	width  = 0;
	height = 0;
}

bool TimeWave::WaveTarget::IsReady() const
{
	return fboID != 0 && heightTextureID != 0 && velocityTextureID != 0 && width > 0 && height > 0;
}

bool TimeWave::EnsureWaveTargets( const FFGLTextureStruct& inputTexture )
{
	const int selectedResolutionMode = static_cast< int >( resolutionMode + 0.5f );
	const GLsizei resolutionDivisor = selectedResolutionMode == 2 ? 4 : selectedResolutionMode == 1 ? 2 : 1;
	const GLsizei targetWidth  = static_cast< GLsizei >( ( inputTexture.Width + resolutionDivisor - 1 ) / resolutionDivisor );
	const GLsizei targetHeight = static_cast< GLsizei >( ( inputTexture.Height + resolutionDivisor - 1 ) / resolutionDivisor );

	if( targetWidth <= 0 || targetHeight <= 0 )
		return false;

	const bool targetsReady =
		waveTargets[ 0 ].IsReady() &&
		waveTargets[ 1 ].IsReady() &&
		waveTargets[ 0 ].width == targetWidth &&
		waveTargets[ 0 ].height == targetHeight &&
		waveTargets[ 1 ].width == targetWidth &&
		waveTargets[ 1 ].height == targetHeight;

	if( targetsReady )
		return true;

	ReleaseWaveTargets();

	if( !waveTargets[ 0 ].Initialise( targetWidth, targetHeight ) )
	{
		ReleaseWaveTargets();
		return false;
	}

	if( !waveTargets[ 1 ].Initialise( targetWidth, targetHeight ) )
	{
		ReleaseWaveTargets();
		return false;
	}

	frontTargetIndex = 0;
	stateInitialized = false;
	resetRequested   = true;

	return true;
}

bool TimeWave::UpdateWaveState( const FFGLTextureStruct& inputTexture, const FFGLTexCoords& inputMaxCoords )
{
	const int backTargetIndex = ( frontTargetIndex + 1 ) % 2;
	WaveTarget& previousTarget = waveTargets[ frontTargetIndex ];
	WaveTarget& nextTarget = waveTargets[ backTargetIndex ];

	if( !previousTarget.IsReady() || !nextTarget.IsReady() )
		return false;

	GLint previousViewport[ 4 ];
	glGetIntegerv( GL_VIEWPORT, previousViewport );

	{
		ScopedFBOBinding fboBinding( nextTarget.fboID, ScopedFBOBinding::RB_REVERT );
		const GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
		glDrawBuffers( 2, drawBuffers );
		glViewport( 0, 0, nextTarget.width, nextTarget.height );

		ScopedShaderBinding shaderBinding( updateShader.GetGLID() );

		glActiveTexture( GL_TEXTURE0 );
		glBindTexture( GL_TEXTURE_2D, inputTexture.Handle );
		glActiveTexture( GL_TEXTURE1 );
		glBindTexture( GL_TEXTURE_2D, previousTarget.heightTextureID );
		glActiveTexture( GL_TEXTURE2 );
		glBindTexture( GL_TEXTURE_2D, previousTarget.velocityTextureID );

		updateShader.Set( "InputTexture", 0 );
		updateShader.Set( "PreviousHeightTexture", 1 );
		updateShader.Set( "PreviousVelocityTexture", 2 );
		updateShader.Set( "InputMaxUV", inputMaxCoords.s, inputMaxCoords.t );
		updateShader.Set( "TexelSize", 1.0f / nextTarget.width, 1.0f / nextTarget.height );
		updateShader.Set( "Feedback", feedback );
		updateShader.Set( "Friction", friction );
		updateShader.Set( "Decay", decay );
		updateShader.Set( "Speed", speed );
		updateShader.Set( "WaveScale", waveScale );
		updateShader.Set( "Blur", blur );
		updateShader.Set( "Spread", spread );
		updateShader.Set( "ResetState", stateInitialized && !resetRequested ? 0 : 1 );

		quad.Draw();

		glActiveTexture( GL_TEXTURE2 );
		glBindTexture( GL_TEXTURE_2D, 0 );
		glActiveTexture( GL_TEXTURE1 );
		glBindTexture( GL_TEXTURE_2D, 0 );
		glActiveTexture( GL_TEXTURE0 );
		glBindTexture( GL_TEXTURE_2D, 0 );
	}

	glViewport( previousViewport[ 0 ], previousViewport[ 1 ], previousViewport[ 2 ], previousViewport[ 3 ] );

	frontTargetIndex = backTargetIndex;
	stateInitialized = true;
	resetRequested   = false;

	return true;
}

void TimeWave::RenderOutput( const FFGLTextureStruct& inputTexture, const FFGLTexCoords& inputMaxCoords )
{
	const WaveTarget& currentTarget = waveTargets[ frontTargetIndex ];

	ScopedShaderBinding shaderBinding( outputShader.GetGLID() );

	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, inputTexture.Handle );
	glActiveTexture( GL_TEXTURE1 );
	glBindTexture( GL_TEXTURE_2D, currentTarget.heightTextureID );
	glActiveTexture( GL_TEXTURE2 );
	glBindTexture( GL_TEXTURE_2D, currentTarget.velocityTextureID );

	outputShader.Set( "InputTexture", 0 );
	outputShader.Set( "HeightTexture", 1 );
	outputShader.Set( "VelocityTexture", 2 );
	outputShader.Set( "InputMaxUV", inputMaxCoords.s, inputMaxCoords.t );
	outputShader.Set( "TexelSize", 1.0f / currentTarget.width, 1.0f / currentTarget.height );
	outputShader.Set( "Amount", amount );
	outputShader.Set( "Phase", phase );
	outputShader.Set( "Warp", warp );
	outputShader.Set( "Contrast", contrast );

	quad.Draw();

	glActiveTexture( GL_TEXTURE2 );
	glBindTexture( GL_TEXTURE_2D, 0 );
	glActiveTexture( GL_TEXTURE1 );
	glBindTexture( GL_TEXTURE_2D, 0 );
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, 0 );
}

void TimeWave::ReleaseWaveTargets()
{
	waveTargets[ 0 ].Release();
	waveTargets[ 1 ].Release();
	frontTargetIndex = 0;
	stateInitialized = false;
	resetRequested   = true;
}
