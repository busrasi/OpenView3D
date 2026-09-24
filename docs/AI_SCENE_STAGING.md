# Native AI scene staging

The existing right-side AI Assistant now stages the active view's OBJ in a real
3D environment. No browser, web renderer, Node, Python, localhost service or new
third-party dependency is used by this feature. Procedural construction and
rendering work offline. The older, separate Image to 3D Model feature is unchanged.

## Try it

Load a local OBJ, wait for it to appear, and send:

1. `Show this vase in a modern room on a wooden table.`
2. `Make the room more minimalist.`

Other supported examples are `Put this model on a pedestal in a minimalist room`,
`Create a cozy room around this model`, and `Make the room Scandinavian instead.`
Arrow keys orbit; the wheel and existing transform controls change zoom/rotation.
`Make the room use a wide camera` selects a wider framing in mock mode.
The Clear Generated Scene button (or chat command) removes staging and restores
the standalone model view. The source OBJ and texture selection are preserved.

No `OPENAI_API_KEY` means Mock AI, including all examples above. With a key, the
existing OpenAI provider sends the request, model filename and previous validated
SceneSpec through HTTPS. It does not send mesh data or local paths. `OPENAI_MODEL`
continues to default to `gpt-4.1-mini`. Real provider calls were not exercised.
The response contract follows the official
[Structured Outputs guide](https://developers.openai.com/api/docs/guides/structured-outputs).

## Architecture

```text
ChatPanel -> ChatController -> RequestRouter
  MESH_EDIT        -> existing AIProvider / MeshEditingService
  MODEL_QUERY      -> existing AppController selection
  SCENE_GENERATION -> SceneGenerationController -> AIProvider
                      -> SceneSpec validation -> SceneAssetManager / SceneBuilder
                      -> active ViewState.scene -> existing OpenGLViewport
```

RequestRouter is a conservative, extensible C++ intent classifier. It routes
explicit scene commands and supported follow-ups, rejects negated/unknown
commands, and keeps routing out of QML. Multi-intent and unrestricted natural
language routing are not implemented.

SceneGenerationController coordinates a SceneGenerationJob through
SceneJobManager. Each job records a UUID, original prompt, selected-model
snapshot, status, validated specification, asset IDs, saved path and error.
QML receives Idle, UnderstandingRequest, GeneratingScene, LoadingAssets,
BuildingScene, Completed and Failed. There is one active chat operation; queueing,
history, retries and cancellation controls are extension points, not implemented
features. Clear through AppController also invalidates pending staging results.

QNetworkAccessManager is asynchronous, with a 60-second transfer timeout and
wall-clock deadline, response size limit, refusal and JSON-envelope checks.
QtConcurrent workers perform OBJ parsing, asset resolution, construction and
atomic JSON persistence. QFutureWatcher delivers completion on the GUI thread.
Only a small immutable scene pointer is swapped into the active view. No worker
touches QML or OpenGL. GPU upload/draw/deletion stays on the viewport render thread.

## Selection and lifecycle

Selection remains `AppController::m_views[m_activeViewIndex]`; there is no second
selection system. Its ViewState now holds an immutable shared mesh and optional
generated SceneData. `selectedSnapshot()` provides the path, filename, mesh
bounds/dimensions, staging transform and selection revision. Camera controls
remain camera controls, not source mesh edits. Loader calculates bounds once on
the background thread. Render synchronization only copies shared pointers.

A job captures the selection revision. Changing/reloading/closing views or
clearing staging prevents stale results from being installed. Async OBJ loads
have separate tokens, so closing or switching tabs cannot load into the wrong
record. Failed generation preserves the last successful environment.

The hero mesh has one CPU representation shared with the renderer and retains
its existing GPU buffers during regeneration. The generated scene contains a
separate hero staging matrix, environment objects, lighting and camera data.
Replacing the scene does not append objects or reload/duplicate the hero. Clear
discards the environment and releases its GPU assets on the next render.

The fit transform uses all three source dimensions and a uniform positive scale.
Its translation centers source X/Z and maps the source minimum Y exactly to the
support top (0.8 m table, 1.05 m pedestal). Source geometry is never rewritten.
OBJ is assumed Y-up; automatic orientation or physical-unit inference is not
implemented. Source coordinates may be off-origin.

## SceneSpec and assets

SceneSpec contains exactly type, style, room, surface, lighting, camera and
heroModel, following the requested nested JSON shape. One schema defines both
the OpenAI response and recursive C++ validation. Unknown/missing fields, wrong
types, unsupported enum values, paths, and non-finite/out-of-range numbers fail.
Room width/depth are 3–20 m; height is 2.4–6 m. Hero source/placement/scale mode and
surface placement are fixed allowlisted values. The model cannot supply code,
tools, arbitrary filesystem paths, deletion requests or executable content.

SceneBuilder composes a floor, two cutaway walls and a table or pedestal from
CPU geometry supplied by SceneAssetManager. Cozy scenes add a rug and bench.
GeneratedAssetProvider is the replacement interface; ProceduralAssetProvider
is the shipped implementation. Providers return immutable, stable-ID interleaved
position/normal geometry. The manager validates geometry before rendering. A
future local/GenAI provider can supply different mesh data through that interface;
network asset-service integration and asset import are not implemented.

One unit-box geometry is shared by all procedural objects. The renderer caches
VAO/VBO resources by asset ID across regeneration, draws lightweight transformed
instances, and releases unused resources safely. It does not upload the same
box for every object. There is no hardware instanced draw batching yet.

ScenePresets contains material, lighting and camera configuration separately from
the builder and QML. Materials include wood, white wall, neutral floor, metal,
glass-like and ceramic-like colors/specular responses. Glass-like is opaque;
there is no PBR, refraction, shadows or authored wood texture. OBJ meshes without
normals receive triangle normals for real surface lighting.

Camera presets are product_focus, wide_scene and close_product. They derive target
and radius from hero/scene bounds; the renderer adjusts distance for viewport
aspect ratio and applies orbit/zoom. The outside-facing wall is omitted when
orbiting outside the room. The floor and other objects remain actual geometry.
Qt's framebuffer vertical convention is corrected in the scene projection, not
by inverting physical placement or changing the existing standalone model view.

## Storage and Windows

`QStandardPaths::AppLocalDataLocation/generated/` contains scenes, models and
textures directories. Each successful scene is saved as `scenes/<UUID>.json`
using QSaveFile, outside the build tree. Procedural meshes need no generated OBJ;
model/texture directories are reserved for future asset providers. The JSON
records version, procedural generator version, SceneSpec, original hero path,
bounds and transform. No pointers or GL handles are serialized. Recreating the
scene requires that hero file and the recorded generator version. There is no
scene-open UI or automatic session restoration yet; old scene JSON is retained.

All new paths use Qt APIs, with no user-specific runtime paths or symlinks. Loader
uses QFile for Unicode/space-containing Windows paths. Project Assets use bundled
Qt resources rather than a developer's C: directory; OBJ and resource-side files
are extracted in a worker to application-data `bundled/<stable-ID>/` so mesh edits
still have a writable local source. Existing user files are not overwritten.
Qt resource textures are also supported. Missing resources/load failures surface
in chat. The existing capsule MTL references a missing JPG; its supplied PNG can
still be selected using Load Texture, as before.

The implementation uses C++17 and existing Qt Core/Gui/Network/Concurrent/OpenGL
facilities. Qt 6.11 / MinGW 13.1 on Windows is the tested kit. MSVC and Windows 10
were not independently tested. Existing GLEW/Qt include warnings and CMake policy
warnings remain. No changes were made to thirdparty/fast_obj.

## Build and verification

Exact configure/build commands used from the repository root in PowerShell:

```powershell
$env:PATH = 'C:\Qt\Tools\mingw1310_64\bin;C:\Qt\6.11.0\mingw_64\bin;' + $env:PATH
& C:/Qt/Tools/CMake_64/bin/cmake.exe -S . -B build/ai-edit -G Ninja `
  -DCMAKE_MAKE_PROGRAM=C:/Qt/Tools/Ninja/ninja.exe `
  -DCMAKE_CXX_COMPILER=C:/Qt/Tools/mingw1310_64/bin/g++.exe `
  -DCMAKE_PREFIX_PATH=C:/Qt/6.11.0/mingw_64 `
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
& C:/Qt/Tools/CMake_64/bin/cmake.exe --build build/ai-edit --parallel 4
& C:/Qt/Tools/CMake_64/bin/ctest.exe --test-dir build/ai-edit --output-on-failure
```

The existing cache uses `FETCHCONTENT_SOURCE_DIR_MANIFOLD` pointing at the local
pinned `build/manifold-inspect` checkout. For a clean offline configure, set
`$manifoldPath = (Resolve-Path build/manifold-inspect).Path` and add
`-DFETCHCONTENT_SOURCE_DIR_MANIFOLD=$manifoldPath`.
Otherwise the unchanged FetchContent declaration obtains the pinned dependency.
No additional dependency was introduced.

GUI smoke invocation (wait for the build to finish before launching on Windows):

```powershell
& ./build/ai-edit/OpenView3D.exe --scene-smoke-test `
  resources/model_vase/model.obj build/ai-edit/scene-artifacts
& ./build/ai-edit/OpenView3D.exe --ai-smoke-test build/ai-edit/artifacts/cube.obj
```

The scene smoke entry is compiled only with BUILD_TESTING and forces Mock AI.
It loads the bundled vase OBJ from disk, submits the exact initial and follow-up
prompts through ChatPanel.submit(), checks routing, object count, hero/support
contact, selected mesh identity and regeneration ID/style. It captures modern,
minimalist and orbit/zoom screenshots, checks rendered hero/support orientation,
exercises the QML arrow-key handler and zoom binding, then clears the scene and
verifies hero preservation. Rendering failures cause nonzero exit.

SceneGenerationTests covers recursive validation, bounds-based placement,
camera presets, unknown/bad asset providers, Unicode/spaced paths, real async
chat jobs, persistence, mock variants, source integrity, missing models and stale
selection/clear races. MeshEditingTests retains the boolean-edit regression suite.

## File inventory

Created:

- `src/ai/RequestRouter.{h,cpp}`
- `src/core/MeshData.h`
- `src/scene/SceneSpec.{h,cpp}`, `SceneData.h`, `ScenePresets.{h,cpp}`
- `src/scene/SceneAssetManager.{h,cpp}`, `SceneBuilder.{h,cpp}`
- `src/scene/SceneJobManager.h`, `SceneGenerationController.{h,cpp}`
- `tests/SceneGenerationTests.cpp`, `tests/SceneUiSmoke.h`
- `docs/AI_SCENE_STAGING.md`

Modified: CMakeLists.txt; AIProvider and ChatController headers/sources;
AppController and OpenGLViewport headers/sources; Loader header/source;
src/main.cpp; ChatPanel.qml, ViewportPanel.qml and AssetBrowser.qml.

No commit or push is performed.


