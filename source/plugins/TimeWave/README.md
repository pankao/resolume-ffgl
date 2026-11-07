# TimeWave

TimeWave is a GPU FFGL effect for Resolume Arena inspired by Pete Warden's classic FreeFrame Wave plugin.

The original Wave effect was a CPU FreeFrame plugin from the early VJ era. It did not behave like a normal trail buffer: older images seemed to propagate through the frame as ripples, creating a drifting temporal feedback surface. TimeWave keeps that artistic behavior but rewrites the approach for modern OpenGL, using persistent ping-pong textures for wave height and velocity.

Original Wave concept: Pete Warden  
GPU FFGL port: Igor Teichman, alias VJ Double Slit <it.ten.tackles@gmail.com>

## What It Does

TimeWave stores a compact simulation state on the GPU:

- Height: the delayed image surface.
- Velocity: how fast that surface is changing.

Each frame, the shader blends the live input with neighboring values from the previous wave state, accelerates the stored height toward that target, damps the velocity, and renders the result back over the live image. The result is temporal motion that breathes and propagates instead of simply fading away.

## Parameters

Amount controls how much of the wave color is mixed into the live image. Lower values keep more of the source. Higher values make the temporal wave dominate.

Feedback controls how much the stored wave state influences the next frame. Higher values create longer memory and stronger temporal propagation. Lower values follow the live source more directly.

Friction controls velocity damping. Higher values let the wave keep moving for longer. Lower values calm the simulation faster.

Decay slowly reduces the stored wave height over time. Higher values preserve energy. Lower values make the image settle more quickly.

Speed controls how strongly the wave accelerates toward its target. Higher values react faster and can feel more elastic. Lower values are smoother and more liquid.

Wave Scale controls the neighbor sampling radius. Lower values produce tighter ripples. Higher values make broader, slower-looking propagation.

Blur softens the simulation state before it feeds forward. Use it to reduce harsh edges or make the result more dreamlike.

Spread controls how strongly neighboring pixels influence the current pixel. Higher values make the image travel through space more clearly. Lower values keep the motion more local.

Phase changes how height and velocity are combined for the final color. It is useful for shifting the visible point in the wave cycle.

Warp uses the wave gradient to distort the live input before compositing. Set Warp to 0 for no geometric distortion.

Contrast applies a final contrast adjustment after the wave mix. It is useful for restoring punch after heavy blur or low-resolution simulation.

Resolution controls the size of the internal simulation textures:

- Full: highest detail, highest GPU cost.
- Half: good default for live use.
- Quarter: fastest and chunkier, useful for heavy feedback or high-resolution content.

Reset reseeds the wave state from the current live input and clears velocity. Use it to remove accumulated motion without cutting the input to black.

## Usage Tips

For an exact dry image, set Amount to 0 and Warp to 0. Amount only controls the color mix; Warp is a separate distortion stage.

For classic slow temporal waves, try high Feedback, high Friction, high Decay, medium Speed, and low to medium Wave Scale.

For soft dreamy smears, increase Blur, keep Speed moderate, and use Half or Quarter Resolution.

For sharper ripples, reduce Blur, raise Spread, and keep Resolution on Full or Half.

For unstable or overly bright feedback, reduce Speed first, then lower Feedback or Decay.

Resolume already provides native layer and clip blend modes, so TimeWave intentionally outputs a normal premultiplied FFGL image and leaves final blending to Arena.

## Implementation Notes

TimeWave is fully GPU based. It does not process pixels on the CPU and does not read textures back from the GPU.

The C++ code owns FFGL lifecycle, parameter handling, shader setup, framebuffer allocation, and texture cleanup. The visual algorithm lives in GLSL.

The persistent wave state uses two framebuffer targets in a ping-pong layout. Each target stores a floating-point height texture and velocity texture. When the input resolution or Resolution parameter changes, the state textures are reallocated and reseeded from the current frame.
