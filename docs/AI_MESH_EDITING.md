# AI OBJ editing

The right-side AI Assistant edits the OBJ selected in the active view. Load a local,
closed solid OBJ and send **Make a hole through the center of the selected model.**
The result is a real cylinder boolean difference with new inner-wall triangles.
The source OBJ is never overwritten.

## Providers and command boundary

With no `OPENAI_API_KEY`, the panel automatically uses Mock AI. It recognizes:

- `make a hole through the center`
- `make a hole in the middle`
- `create a hole through this model`
- `Make a hole through the center of the selected model.`

Mock recognition is deliberately narrow and does not execute negated or unrelated
requests. It explains the supported operation for other messages.

Set `OPENAI_API_KEY` in the application's launching environment to use OpenAI.
Optionally set `OPENAI_MODEL` (default `gpt-4.1-mini`). No key is stored in the
project or sent to QML. The provider uses HTTPS Chat Completions with a strict
JSON schema, a 60-second network timeout, and refusal/error handling. It sends
only the model's filename and current request, not the OBJ, directory, or history.
Questions and unsupported edits produce text with a null command.

Both providers deliver the same command to C++:

```json
{
  "operation": "boolean_subtract",
  "primitive": "cylinder",
  "placement": "center",
  "axis": "z",
  "radius": 0.1,
  "through": true
}
```

`radius` is a fraction of the smaller **object-space XY bounding-box dimension**,
not a fixed world-unit radius. C++ accepts only finite values from 0.01 to 0.4.
The center is the bounding-box center. The 64-sided cylindrical cutter extends
beyond both Z bounds. Camera rotation does not change the cutting axis.
Unknown keys, arbitrary paths, other operations, and incorrect JSON types fail
validation. There are no shell, code execution, deletion, or filesystem tools
exposed to the provider. The OpenAI response envelope is
`{"message":"...","command":{...}}`, with `command:null` for conversation.

## Architecture and selection

```text
ChatPanel.qml -> ChatController -> AIProvider (Mock / OpenAI)
    -> MeshOperation::fromJson -> MeshEditingService (background QtConcurrent task)
    -> selected OBJ -> Manifold difference -> new OBJ
    -> AppController::loadModel -> existing OpenGLViewport binding and Loader
```

Selection remains `AppController::m_views[m_activeViewIndex]`. The added read-only
`selectedModel()` accessor returns that existing `ViewState` record; there is no
parallel model collection or selection flag. The filename is derived from its
`modelPath`. `modelPathChanged` updates the panel and advances a selection revision.
A request snapshots that revision and source path. A selection change before the
AI response prevents editing. A change while a background edit is already running
allows the original request's new file to finish but prevents replacing the newly
selected view. The chat reports the saved path instead.

`MeshEditingService` owns all OBJ parsing, validation, boolean work and writing.
Future operations can extend the validated operation type and service dispatch
without giving providers file access. Currently only the centered through-hole
operation is implemented.

## Geometry and OBJ preservation

CMake FetchContent pins [Manifold](https://github.com/elalish/manifold/tree/v3.3.2)
3.3.2 at `798d83c8d7fabcddd23c1617097b95ba40f2597c`. It builds the C++ boolean
backend statically, without Python, C bindings, parallel TBB or 2D Clipper support.
No files in `thirdparty/fast_obj` are changed.

The editor supports triangle faces, planar polygons (including concave polygons
via Manifold triangulation), positive/negative OBJ indices, texture-coordinate
seams, and optional normals. It preserves object-space positions and UV properties,
interpolates UVs across split faces, retains `mtllib` references and source-face
material assignments, and generates new per-face normals. Cutter faces have
default UVs and no assigned source material. Material files and textures remain
beside the original; the output stays in that same directory.

For example, `C:/models/vase.obj` becomes `C:/models/vase_ai_edited.obj`, then
`vase_ai_edited_1.obj`, etc. `QIODevice::NewOnly` reserves each new filename without
overwriting an existing file. Failed writes remove only the newly created partial
output. The complete geometry is generated before opening the destination.

The UI smoke test exposed a pre-existing renderer transform-order error for
off-origin models. `OpenGLViewport` now centers geometry before scaling/rotating,
so the editor can preserve original coordinates and still display the output.

## Build and test

PowerShell commands for the installed Windows Qt kit:

```powershell
$env:PATH = 'C:\Qt\Tools\mingw1310_64\bin;C:\Qt\6.11.0\mingw_64\bin;' + $env:PATH
& C:/Qt/Tools/CMake_64/bin/cmake.exe -S . -B build/ai-edit -G Ninja `
  -DCMAKE_MAKE_PROGRAM=C:/Qt/Tools/Ninja/ninja.exe `
  -DCMAKE_CXX_COMPILER=C:/Qt/Tools/mingw1310_64/bin/g++.exe `
  -DCMAKE_C_COMPILER=C:/Qt/Tools/mingw1310_64/bin/gcc.exe `
  -DCMAKE_PREFIX_PATH=C:/Qt/6.11.0/mingw_64 -DCMAKE_BUILD_TYPE=Release
& C:/Qt/Tools/CMake_64/bin/cmake.exe --build build/ai-edit --parallel 4
& C:/Qt/Tools/CMake_64/bin/ctest.exe --test-dir build/ai-edit --output-on-failure
```

First configuration needs Git/network access to fetch Manifold. For this session,
the exact pinned version was cloned into `build/manifold-inspect` for inspection,
and configuration additionally used
`-DFETCHCONTENT_SOURCE_DIR_MANIFOLD=C:/Users/busra/OneDrive/Documents/OpenView3D/build/manifold-inspect`.
That local override is only in the build cache, not hardcoded in the project.

The unused legacy shared GLEW target is excluded from the default build because
its `-nostdlib` link fails with modern MinGW; the existing static GLEW dependency
continues to build and link. Existing GLEW/Qt include warnings remain.

Create reviewable geometry and run the actual QML submission flow:

```powershell
& ./build/ai-edit/MeshEditingTests.exe build/ai-edit/artifacts
& ./build/ai-edit/OpenView3D.exe --ai-smoke-test build/ai-edit/artifacts/cube.obj
```

The smoke flag is available only in `BUILD_TESTING` builds, always uses Mock AI,
submits through `ChatPanel.submit()`, writes `ai-smoke.png` beside the fixture,
and exits. The normal application has no startup edits. Set `BUILD_TESTING=OFF`
to omit test executables and the smoke entry point. The test executable also
supports `MeshEditingTests --edit <local.obj>` for manually testing copied assets.

Verified in this session:

- Release application and test executable build successfully with Qt 6.11/MinGW 13.1.
- CTest: 1/1 passed. The test executable checks command validation, missing
  selection, mock aliases, negation, selection changes, invalid/open meshes,
  filename collisions, no-op subtraction, UV/material preservation and original-file integrity.
- An off-origin cube has volume 8 before editing and 7.74908 after editing.
  The saved result reloads as a closed manifold with Euler characteristic 0
  (genus 1), and its intersection with a central probe cylinder is empty.
  The existing viewer Loader reads its 272 triangles (816 expanded vertices).
- GUI smoke test exits 0, the chat loads and selects the generated OBJ,
  the renderer reports `model loaded: true`, and the screenshot shows the hole.
- A copy of `resources/models/capsule.obj` edits successfully, volume
  7.32143 -> 6.94765.
- A copy of `resources/model_vase/model.obj` is rejected as non-manifold;
  no edited file is written for that asset.

## Limitations

- Requires a closed, consistently oriented solid. No automatic hole filling,
  mesh repair, or self-intersection repair is performed. Invalid input produces
  a chat error; the bundled vase currently needs repair first.
- Local OBJ files only, up to 256 MB. Embedded resource URLs, non-planar polygons,
  freeform curves/surfaces, vertex-color extensions and other unsupported records
  are rejected. The source directory must be writable.
- Object/group names and smoothing groups are not retained. Normals are regenerated
  per triangle. New cutter surfaces do not get an authored UV unwrap or material.
  The existing viewer's separate texture selection remains unchanged.
- Only object-Z, bounding-box-centered, cylindrical subtraction is implemented.
  If the cutter removes no material, the service reports a no-op instead of success.
- No undo stack, cancellation, streamed responses, persistent chat history or
  multi-turn AI context yet. The original file provides a manual restore path.
- Live OpenAI calls were not exercised; the complete offline/mock path was tested.
  See [official Structured Outputs documentation](https://developers.openai.com/api/docs/guides/structured-outputs)
  for the API contract used by the real provider.

All changes are left uncommitted for review.
