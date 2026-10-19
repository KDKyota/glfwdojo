# glfwdojo {#mainpage}

LearnOpenGL で OpenGL を学習し、
それをモダンな C++ コードにして、
様々なレンダリングや 3D モデルなどの要素を付け足しています。

## 描画パイプライン

`Scene::Render()` が 1 フレームの間に呼ぶパスを、実行順に並べた図です。
各ノードをクリックすると、対応するクラスのページへ移動します。

- 太い実線は実行順を表します。
- 細い点線は、パス同士が受け渡すデータ（テクスチャ）を表します。
- 破線のノードは、設定で切り替えられる任意のパスです。

@dot
digraph pipeline {
    rankdir=TB;
    nodesep=0.4;
    ranksep=0.5;
    node [shape=box, style=rounded, fontsize=12];
    edge [fontsize=10];

    shadow     [label="Shadow\n点光源の深度と透過色",        URL="\ref gl::ShadowPass"];
    reflection [label="Reflection\n床の平面反射", style="rounded,dashed", URL="\ref gl::ReflectionPass"];
    geometry   [label="Geometry\nG-Buffer を書く",            URL="\ref gl::GeometryPass"];
    ssao       [label="SSAO\n近距離の遮蔽",                   URL="\ref gl::SsaoPass"];
    sdf        [label="SDF 遮蔽\n数 m 規模の遮蔽",           URL="\ref gl::SdfOcclusionPass"];
    blit       [label="深度のコピー",                         URL="\ref gl::GBuffer::BlitDepthTo()"];
    lighting   [label="Lighting\nPBR と IBL を合成",          URL="\ref gl::DeferredLightingPass"];
    forward    [label="Forward\n空・ガラス・ライト",         URL="\ref gl::ForwardPass"];
    bloom      [label="Bloom\n明るい部分をぼかす",            URL="\ref gl::BloomPass"];
    tonemap    [label="ToScreen\nトーンマップして画面へ",     URL="\ref gl::TonemapPass"];

    // 実行順
    edge [penwidth=2];
    shadow -> reflection -> geometry -> ssao -> sdf -> blit -> lighting -> forward -> bloom -> tonemap;

    // パス同士が受け渡すデータ
    edge [penwidth=1, style=dotted, constraint=false, color="#555555", fontcolor="#555555"];
    shadow     -> lighting [label="シャドウ"];
    reflection -> lighting [label="反射像"];
    geometry   -> ssao     [label="G-Buffer"];
    geometry   -> sdf      [label="G-Buffer"];
    geometry   -> lighting [label="G-Buffer"];
    ssao       -> lighting [label="AO"];
    sdf        -> lighting [label="SDF 遮蔽"];
    blit       -> forward  [label="深度"];
    lighting   -> forward  [label="HDR"];
    forward    -> bloom    [label="BrightColor"];
    bloom      -> tonemap  [label="ぼかし結果"];
}
@enddot

## クラス早見表

`src/` のディレクトリごとに、主なクラスと役割をまとめています。
クラス名をクリックすると、詳しいページへ移動します。

まず読むなら、全体を統括する Scene から入ると流れを追えます。

### render（描画パイプライン）

| クラス | 役割 |
| --- | --- |
| Scene | シーンのジオメトリ・ライト・レンダーパイプライン全体を所有し 1フレームの描画を統括する |
| gl::SceneGeometry | シーンの静的メッシュとそのテクスチャを持ち 与えられたシェーダーで描画する |
| gl::RenderSettings | UI から実行時に変更する描画設定とマテリアル |
| gl::PbrMaterial | glTF の Metallic-Roughness ワークフローに基づく PBR マテリアル |
| gl::RenderView | 描画に使う視点 鏡像の視点も渡せる |
| gl::PointLight | 点光源 |
| gl::DirectionalLight | 平行光源 |
| gl::SpotLight | 円錐状に照らすスポットライト |

### render/pass（レンダーパス 1パス1クラス）

| クラス | 役割 |
| --- | --- |
| gl::ShadowPass | 点光源ごとに深度とガラスの透過色をキューブマップへ描く |
| gl::ReflectionPass | 床で折り返した鏡像カメラからシーンを描いて ReflectionTarget へ書く |
| gl::GeometryPass | 不透明オブジェクトの幾何情報とマテリアルを G-Buffer へ描く |
| gl::SsaoPass | G-Buffer から近距離の遮蔽率を求め ノイズを均すブラーまで掛ける |
| gl::SdfOcclusionPass | SSAO が届かない数m規模の遮蔽を SDF のレイマーチで求める |
| gl::DeferredLightingPass | G-Buffer と影と遮蔽と IBL を合成して HDR ターゲットへ書く |
| gl::ForwardPass | G-Buffer に載せられないもの（ライトキューブ・空・ガラス）を前方描画する |
| gl::BloomPass | HDR ターゲットの明るい部分を縦横交互のガウシアンブラーでぼかす |
| gl::TonemapPass | トーンマッピングとガンマ補正をかけてデフォルト FBO へ出力する |

### render/targets（パスの出力先）

| クラス | 役割 |
| --- | --- |
| gl::GBuffer | Deferred Shading の幾何情報を受け取る3枚のテクスチャと FBO |
| gl::HdrTarget | ライティング結果を HDR のまま受け取る FBO 明るい部分を2枚目へ分離する |
| gl::OcclusionTarget | 遮蔽率を書くパスの出力先 本体とブラー後の2枚を持つ |
| gl::ReflectionTarget | 鏡像カメラで描いた床の映り込みを受け取る FBO |
| gl::ShadowCubeTargets | 点光源ごとの深度キューブマップと ガラスを透過した光の色を持つ |
| gl::IblMaps | 起動時に焼いた IBL 用のテクスチャ一式（render/ibl） |

### asset（モデルとメッシュ）

| クラス | 役割 |
| --- | --- |
| Model | Assimp で glTF/glb モデルを読み込み Mesh の集合とスキニング用のボーン情報を保持する |
| Mesh | 1つの VAO/VBO/EBO と PbrMaterial を持つ 描画可能な最小単位 |
| ModelNode | モデルのノード階層の1要素 親からの相対変換とメッシュ番号と子を持つ スキニングのため木のまま保持する |
| BoneInfo | ボーンの番号と メッシュ空間からボーン空間への変換（オフセット行列） |
| gl::MeshDistanceField | 焼き込んだメッシュ距離場 3D テクスチャと AABB を持つ |
| gl::MeshDistanceFieldCache | 同じメッシュの距離場を複数回焼かないための弱参照キャッシュ |

### scene（シーン上の存在）

| クラス | 役割 |
| --- | --- |
| Camera | View Matrix を生成するカメラ FreeLook（自由視点）と ThirdPerson（追従）の2モードを持つ |
| Character | プレイヤーが操作するキャラクターの位置 向き 動作状態を持つ |
| gl::SceneModels | 読み込んだモデルとその配置行列 および操作対象のキャラクターを持つ |
| gl::CollisionWorld | 障害物の一覧を持ち 円柱との重なりを解消する |
| Transform | オブジェクトの Local → World 変換（TRS）を保持する |

### gl（GL リソースの薄いラッパ）

| クラス | 役割 |
| --- | --- |
| gl::Shader | シェーダーのコンパイル・リンクと uniform 設定を行う |
| Texture | 画像を読み込み 2D テクスチャとして GPU へアップロードする |
| TextureCache | 同じ画像を複数回ロードしないための Texture の弱参照キャッシュ |
| gl::HandleBase | OpenGL オブジェクトの ID を RAII 管理する基底クラス 以下の `*Handle` が派生する |
| gl::VertexArrayHandle | VAO を管理する |
| gl::BufferHandle | VBO・EBO・UBO などのバッファを管理する |
| gl::TextureHandle | テクスチャを管理する |
| gl::FramebufferHandle | FBO を管理する |
| gl::RenderbufferHandle | RBO を管理する |
| gl::QueryHandle | 時間計測やオクルージョンに使うクエリを管理する |
| gl::NoiseTexture | SSAO と SDF 遮蔽が共有する 4x4 のランダムテクスチャ |

### app（ウィンドウ・入力・UI）

| クラス | 役割 |
| --- | --- |
| Window | GLFW ウィンドウと OpenGL コンテキストを管理する RAII ラッパー |
| InputState | 入力とカーソルの捕捉状態を Gameplay/Paused の2値で管理する |
| MouseState | マウス座標イベントから前フレームとの差分を計算する |
| Gui | Dear ImGui を GLFW・OpenGL・フレーム管理ごと閉じ込める |

### debug・core

| クラス | 役割 |
| --- | --- |
| gl::GpuProfiler | パスごとの GPU 実行時間を計測する |
| gl::CollisionDebugDraw | 衝突判定の円柱をワイヤーフレームで重ねて描く |
| gl::FrameArena | 寿命が1フレームのデータ専用のバンプアロケータ |

## 関連ドキュメント

このページはクラスの構成を調べるための入口です。使い方や設計の背景は、次の文書を参照してください。

| 文書 | 内容 |
| --- | --- |
| [README.md](https://github.com/KDKyota/glfwdojo/blob/master/README.md) | プロジェクトの概要 実装済みの機能 ビルドと実行の方法 操作方法 動作環境 |
| [DEVELOPMENT.md](https://github.com/KDKyota/glfwdojo/blob/master/docs/DEVELOPMENT.md) | 機能の追加や変更の手順と注意点 冒頭の「症状から探す」から 画面が壊れたときの原因を引ける |
