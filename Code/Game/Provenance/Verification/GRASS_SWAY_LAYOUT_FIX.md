# Grass sway startup allocation fix — 2026-09-07

## Observed failure

User's Visual Studio screenshot shows `RenderSystem::CreateShaderParameters`
asserting `( parametersSizeInBytes % 32 ) == 0`, followed by a CRT invalid-
parameter exception inside `SystemLog::TraceMessage` while printing it.

The generated grass metadata ends with `m_coverTexture`, stride 2 at offset 60:
the last reflected field ends at byte 62. The generated HLSL struct contains
`uint m_packedData60` and occupies 64 bytes. No extra padding property is emitted
because that packed word already completes the second 32-byte block.

The allocator used maximum field end (62) as allocation size and required that
value to be block aligned. The two new wind scalars exposed this latent trailing
half-word case. Earlier wind scalar/grass tests and successful compilation did
not exercise material allocation; the user's live run identified the gap.

## Corrections

- `Code/Engine/Render/RenderShaderParameterLayout.h` computes maximum field end
  and rounds UP to complete 32-byte blocks. `RenderSystem::CreateShaderParameters`
  uses that helper, retains the alignment assertion, and zero-initializes the
  whole allocation before initializing resource handles. No shader field offsets,
  texture identities or wind amplitudes were changed.
- `Code/Base/Logging/SystemLog.cpp::LogAssert` passes assertion text via `"%s"`
  to BOTH tracing and the log-entry path. Previously the literal percent sign
  in the expression was interpreted as a printf directive without arguments,
  masking the original assertion with the CRT exception. Formatted assertions
  continue to format once before reaching this literal-text path.

## Verification

- `Scripts/RunShaderParameterLayoutTest.cmd`: 4,108 allocation checks pass,
  including all seven current generated material layouts, the exact 62-to-64
  case, reverse field ordering, block boundaries, empty layout, and 4,096 sizes.
  Expected custom packed sizes come from the generated HLSL root-constant word
  count; the common-only Placeholder uses its 32-byte storage block.
- The same runner extracts current production `LogAssert` and Win32
  `TraceMessage` bodies into a test harness: 16 checks pass with and without
  an initialized mock log sink, preserving `% 32`, `%s`, `%n`, `%p`, and `100%`.
- `Scripts/RunLogAssertDllSmokeTest.cmd` links the ACTUAL rebuilt Base DLL and
  invokes its early-startup reporting path four times; all return without CRT
  failure. This does not deliberately trigger a debugger breakpoint.
- Debug Base and Engine Runtime builds succeed. No ABI/public layout change
  required rebuilding the unchanged Game DLL for this correction. Renderer
  shader source, material resource and gameplay geometry are unchanged.
- Editor was confirmed closed; only the authorized eight resource helpers from
  this build were stopped. Visual Studio and unrelated processes were left alone.

Build/test logs: `Build/Verification/MovementBaseline/grass-wind-layout-*`.
Source/build identity: `Fixtures/GrassSwayLayoutFixBuild.json`.

## Remaining live check

Reopen `provenancesandbox.map`, start preview, then inspect sway and grass roots.
Neither the headless layout test nor the DLL logging smoke test is a graphics
startup/playable-space certificate. Initial `GrassSwayV1Build.json` remains as
historical evidence of the failed first publication, superseded by this fix.
