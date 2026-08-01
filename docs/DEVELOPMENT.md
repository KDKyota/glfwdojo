# glfwdojo 開発ガイド

このドキュメントは、既存の機能を変更したり新しい要素を追加するときの手順と注意点をまとめたものです。

---

## 目次

1. [アーキテクチャ概要](#アーキテクチャ概要)
2. [UBO（Uniform Buffer Object）](#ubouniform-buffer-object)
3. [ガンマ補正](#ガンマ補正)
4. [ライトの変更](#ライトの変更)
5. [ライトキューブの追加](#ライトキューブの追加)
6. [シャドウマッピング：デプスマップ FBO](#シャドウマッピングデプスマップ-fbo)
7. [Deferred Shading](#deferred-shading)
8. [頂点属性 location の割り当て規約](#頂点属性-location-の割り当て規約)
9. [新しい3Dオブジェクトの追加](#新しい3dオブジェクトの追加)
10. [インスタンシングの追加](#インスタンシングの追加)
11. [新しいシェーダーの追加](#新しいシェーダーの追加)
12. [テクスチャの追加](#テクスチャの追加)
13. [Linux (WSL2) で動かす](#linux-wsl2-で動かす)
14. [画面が真っ黒・真っ白になったときの調べ方](#画面が真っ黒真っ白になったときの調べ方)
15. [よくある落とし穴](#よくある落とし穴)
16. [今後の課題](#今後の課題)

---

## アーキテクチャ概要

```
main.cpp
 └── Window       : GLFWウィンドウ管理・メインループ
 └── Scene        : すべての描画処理を管理する中心クラス
      ├── Shader       : GLSLシェーダープログラムのラッパー
      ├── Camera       : 視点・ビュー行列の管理
      ├── TextureCache : テクスチャの読み込みとキャッシュ（weak_ptr管理）
      ├── Lighting.h   : PointLight / DirectionalLight / SpotLight 構造体
      └── GeometryData.h : 頂点データ・インデックス・位置配列の定義
```

### 描画の流れ（Render() 内）

現在は Deferred Shading になっており、1フレームは以下の順で処理されます。

```
[Pass 1] ポイントシャドウ デプスパス（4灯ぶんループ）
   depthMapFBO_[j] にバインド → 光源視点で floor / cube / wall を描画
   → depthCubemap_[j] に「光源からの正規化距離」が焼かれる
 ↓
UBO に view / projection を書き込む
 ↓
[Pass 2] Geometry パス
   glDisable(GL_BLEND)  ← 必須。理由は「よくある落とし穴」参照
   gBuffer_ にバインド → gbuffer_*.frag で cube / floor / wall を描画
   → gPosition_ / gNormal_ / gAlbedoSpec_ の3枚に「幾何情報」だけを書き込む
      （この時点ではライティングもシャドウ判定も一切しない）
 ↓
gBuffer_ の深度を framebuffer_ へ glBlitFramebuffer でコピー
   ← これをしないと後続のスカイボックス等が正しく前後判定できない
 ↓
[Pass 3] Lighting パス
   framebuffer_ にバインド → G-Buffer 3枚 + depthCubemap_ 4枚をサンプラーにバインド
   → フルスクリーンクワッドを1回描画するだけで全ピクセルのライティングが完了
 ↓
[Pass 4] 前方描画（G-Buffer に載せられないもの）
   ライトキューブ → スカイボックス → 透過窓（この間だけ GL_BLEND を有効化）
 ↓
[Pass 5] Bloom
   brightColorBuffer_ を pingpongFBO_ で10回ガウシアンブラー
 ↓
デフォルトフレームバッファへ戻す
 ↓
hdr.frag でブラー結果を加算し、トーンマッピング＋ガンマ補正して画面へ
```

> **なぜ透過窓とスカイボックスだけ前方描画なのか**
> Deferred Shading は「1ピクセルにつき1つの面の情報しか G-Buffer に保持できない」方式です。
> 半透明の面は「奥の面と手前の面の両方の色」が必要なので、原理的に G-Buffer に載せられません。
> スカイボックスとライトキューブは、そもそもライティング計算が不要（自分で発光している）なので
> 前方描画のほうが素直です。

---

## UBO（Uniform Buffer Object）

view と projection は UBO（binding = 0）で全シェーダーに共有されています。

**C++ 側（Scene.cpp の Render()）:**
```cpp
glBindBuffer(GL_UNIFORM_BUFFER, matricesUBO_);
glBufferSubData(GL_UNIFORM_BUFFER, 0,                 sizeof(glm::mat4), glm::value_ptr(view));
glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(projection));
glBindBuffer(GL_UNIFORM_BUFFER, 0);
```

**GLSL 側（各 .vert ファイル）:**
```glsl
layout (std140, binding = 0) uniform Matrices {
    mat4 view;        // offset 0
    mat4 projection;  // offset 64
};
```

> **注意:** メンバの順序（view → projection）と C++ の書き込み順を必ず一致させること。
> 新しい .vert ファイルを作るときも同じブロック定義をコピーする。
> `glBufferSubData` の第3引数は必ず `sizeof(glm::mat4)`（64 bytes）を指定すること。
> 誤って `sizeof(配列)` を渡すとサイズが狂い、projection が正しく書き込まれない。

---

## ガンマ補正

ガンマ補正のかけ方には2通りあり、**必ずどちらか一方だけを使います。**

| 方法 | やり方 |
|---|---|
| OpenGL に任せる | `Window.cpp` で `glEnable(GL_FRAMEBUFFER_SRGB);` を呼ぶ |
| シェーダーで手動 | 最終出力の直前で `pow(color, vec3(1.0 / 2.2))` |

**このプロジェクトは後者（`hdr.frag` での手動補正）を採用しています。**
HDR + トーンマッピングを実装しており、トーンマッピングとガンマ補正を同じシェーダー内で
連続して行うほうが処理の流れを追いやすいためです。

### 二重にかけると画面全体が白っぽくなる（実際に踏んだ）

**症状:** 画面全体で黒が浮き、テクスチャの色が薄く、靄がかかったように眠い絵になる。
エラーは一切出ない。`Bloom` を 0 にしても `ambient` を 0 にしても `exposure` を下げても消えない。

**原因:** `glEnable(GL_FRAMEBUFFER_SRGB)` と `hdr.frag` の `pow(mapped, 1/2.2)` が両方有効になっていた。

**なぜそうなるか:** ガンマ補正は暗部を大きく持ち上げる操作です。2回かけると実効的に
`L^(1/4.84)` となり、本来ほぼ黒であるべき値が中間グレーまで浮き上がります。

| 元の値 | 1回補正 | 2回補正 |
|---|---|---|
| 0.05 | 0.25 | **0.51** |
| 0.1 | 0.35 | **0.61** |
| 0.2 | 0.48 | **0.71** |
| 0.5 | 0.73 | **0.87** |

`Bloom` や `ambient` や `exposure` をいくら下げても消えないのがポイントです。
**それらを 0 にしても、残ったわずかな値がガンマ2回でグレーまで持ち上げられる**ためで、
「パラメータをいくら触っても効かない」という形で現れます。

**`GL_FRAMEBUFFER_SRGB` は静かに効いたり効かなかったりする。**
この設定は「書き込み先のフレームバッファが sRGB 対応のときだけ」変換を行います。
GLFW は既定で `GLFW_SRGB_CAPABLE` を要求しないため、環境によっては効かず、
その場合は二重にならないので問題が表面化しません。**環境を変えた途端に画面が白くなる**
という形で発覚することがあります。

実際にどちらなのかは、コンテキスト作成後に問い合わせれば確定します。

```cpp
GLint encoding = 0;
glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_BACK_LEFT,
    GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING, &encoding);
// 0x8C40 = GL_SRGB  -> glEnable(GL_FRAMEBUFFER_SRGB) の自動変換が効く
// 0x2601 = GL_LINEAR -> 効かない
```

`Window.cpp` はこの値を起動時に出力するようにしてあります。

### 切り分けの手順

`hdr.frag` には「Bloom 合成・トーンマッピング・ガンマ補正をすべて飛ばす」デバッグ経路
（UI の `Raw output`）があります。

- **`Raw output` を ON にすると靄が消える** → 原因は Bloom / トーンマッピング / ガンマ補正のどれか
- そこから `Bloom` を 0、`Ambient` を 0、`Exposure` を最小、と順に潰していく
- **どれを 0 にしても消えないなら、残るのはガンマ補正**

なお `Raw output` の見え方も正しくはありません。リニア値をそのまま出すため、
ディスプレイの特性で本来より暗く・コントラストが強く表示されます。
**正しい状態は「Raw output」と「二重補正」のちょうど中間**です。

> **テクスチャ側の二重補正にも注意:** スカイボックスなど既に sRGB 空間の画像を
> リニア空間の計算に使うと、別の意味で二重になります。
> テクスチャロード時に内部フォーマットへ `GL_SRGB` / `GL_SRGB_ALPHA` を指定すると
> サンプリング時に自動でリニアへ変換されるため、こちらは正しく扱えます。

---

## ライトの変更

### PointLight の位置を変更する

`GeometryData.h` の `pointLights` 配列を直接編集します：

```cpp
inline const std::array<gl::PointLight, 1> pointLights = {{
    { glm::vec3(0.0f, 2.0f, 0.0f) },
}};
```

各 `PointLight` のメンバ（`Lighting.h` 参照）:
```cpp
struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient  = { 0.05f, 0.05f, 0.05f };
    glm::vec3 diffuse  = { 1.0f,  1.0f,  1.0f  };
    glm::vec3 specular = { 1.0f,  1.0f,  1.0f  };
    float constant  = 1.0f;
    float linear    = 0.09f;
    float quadratic = 0.032f;
};
```

> **ambient が小さすぎると影側の面が真っ黒になる。** 点光源1灯だと光の当たらない面は
> ambient 値しか明るさがないため、0.05 程度では暗くなりすぎることがある。シーンに合わせて調整する。

### 環境光は「ライトごと」ではなく「シーン全体で1つ」にしている

`PointLight` 構造体には `ambient` メンバがあるが、**このプロジェクトでは使っていない**（全て
`glm::vec3(0.0f)`）。代わりに `Scene` の `ambientStrength_` をシーン全体で1つだけ持ち、
`deferred_lighting.frag` / `shader.frag` の両方でライトのループの**外**から加算している。

```glsl
vec3 result = ambientStrength * Albedo;  // 距離減衰を掛けない
for (int i = 0; i < NR_LIGHTS; ++i) { result += CalcPointLight(...); }
```

LearnOpenGL では章によって扱いが変わるので、混乱しやすい箇所。

| 章 | 環境光の扱い |
|---|---|
| Basic Lighting | シーン全体の定数（`float ambientStrength = 0.1;`） |
| Light Casters / Multiple Lights | ライトごとのメンバ + `attenuation` |
| **SSAO** | **シーン全体の定数に戻る。`attenuation` なし** |

Light Casters の章には減衰について次の注意書きがある。

> We could leave the ambient component alone so ambient lighting is not decreased over distance,
> but if we were to use more than 1 light source all the ambient components will start to stack up.
> In that case we want to attenuate ambient lighting as well. Simply play around with what's best
> for your environment.

つまり「ライトごとに持たせて減衰させる」のは**複数光源で環境光が足し合わさって明るくなりすぎる
のを防ぐため**の措置であって、物理的な正しさから来ているわけではない。

一方 SSAO の章では、

```glsl
vec3 ambient = vec3(0.3 * Diffuse * AmbientOcclusion);
```

とシーン全体の定数に戻り、減衰も掛けていない。**SSAO が掛かる対象が ambient しかないため**で、
ライトごとに減衰させると光源から離れた場所で ambient が 0 に近づき、AO を掛ける相手そのものが
消えてしまう。実際このプロジェクトでも、Multiple Lights の形のまま SSAO を実装したときは
**光源のすぐ近くでしか AO が見えなかった。**

シーン全体で1つに持てば「足し合わさって明るくなりすぎる」問題も同時に解消されるので、
SSAO を使うならこちらの形が適している。

> **前方描画側の更新漏れに注意。** 環境光の方式を変えるときは `deferred_lighting.frag` と
> `shader.frag`（透過窓が使う）の**両方**を直すこと。片方だけ直すと、`light.ambient` が
> `(0,0,0)` のまま掛け算されて**そのオブジェクトだけ環境光が完全にゼロになる**。
> uniform は存在していて値が 0 なだけなのでエラーも警告も出ず、
> 「透過窓だけ不自然に暗い」「UI で ambient を動かしても窓だけ反応しない」という形でしか気づけない。
> 根本原因は前方描画と Deferred でライティングが二重管理になっていること。

### ライトをシェーダーに送る

`Render()` 内でシェーダーごとに `use()` した後に呼ぶ。`shader_`, `cubeShader_`, `transparentwindowShader_` それぞれに送る必要がある（同じ .frag を使っていても、シェーダープログラムオブジェクトが別なら uniform の設定も別々に行う）。

```cpp
for (const auto& pointLight : gl::pointLights) {
    pointLight.applyToShader(*shader_, "pointLights[" + std::to_string(&pointLight - gl::pointLights.data()) + "]");
}
```

---

## ライトキューブの追加

ライトの位置に小さなキューブを描画してライト位置を可視化する手順です。

### VAO の設定

`lightVAO_` を別途 `glGenVertexArrays` せずに、`cubeVAO_` をそのまま流用できます。
`cubeVAO_` には position / normal / uv すべての属性が設定済みですが、
`light_cube.vert` が使うのは `location = 0`（position）だけなので問題ありません。

### Render() での描画（FBO バインド中に行うこと）

```cpp
// ← glBindFramebuffer(GL_FRAMEBUFFER, 0) より前に置くこと
lightcubeShader_->use();
glBindVertexArray(cubeVAO_);
for (const auto& pointLight : gl::pointLights) {
    glm::mat4 lightModel = glm::translate(glm::mat4(1.0f), pointLight.position);
    lightModel = glm::scale(lightModel, glm::vec3(0.2f));
    lightcubeShader_->setMat4("model", lightModel);
    glDrawElements(GL_TRIANGLES, gl::cubeIndices.size(), GL_UNSIGNED_INT, 0);
}
```

### light_cube.vert について

`light_cube.vert` は UBO（binding=0）から `view` / `projection` を取得しています。
C++ 側から `setMat4("view", ...)` / `setMat4("projection", ...)` を呼んでも効果はありません（その uniform は存在しない）。

---

## シャドウマッピング：デプスマップ FBO

シャドウマッピングでは、ライト視点からのデプス情報をテクスチャに書き出す専用の FBO が必要です。

### デプステクスチャは画像ファイルから作らない

`TextureCache` / `Texture` クラスは画像ファイルのロード用です。デプスマップは GPU 上に空の領域を確保するだけでよいため、`glTexImage2D` の最後の引数（data）に `nullptr` を渡して作成します。

これは既存の `initFramebuffer()` 内で `textureColorbuffer_` を作っているパターンと同じ考え方です：

```cpp
// 既存のカラーテクスチャ（参考）
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

// デプスマップ用テクスチャ
glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT,
             0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
```

| 項目 | カラーテクスチャ | デプステクスチャ |
|---|---|---|
| 内部フォーマット | `GL_RGB` / `GL_RGBA` | `GL_DEPTH_COMPONENT` |
| データ型 | `GL_UNSIGNED_BYTE` | `GL_FLOAT` |
| FBO アタッチメント | `GL_COLOR_ATTACHMENT0` | `GL_DEPTH_ATTACHMENT` |
| カラー出力 | あり | `glDrawBuffer(GL_NONE)` で無効化 |
| カラー読み込み | あり | `glReadBuffer(GL_NONE)` で無効化 |

### デプスマップ FBO の初期化手順

1. `unsigned int depthMapFBO_, depthMap_` を Scene.h に追加
2. 新メソッド `initDepthMap()` をコンストラクタから呼び出す
3. FBO 生成 → デプステクスチャ生成 → FBO にアタッチ → `glDrawBuffer(GL_NONE)` / `glReadBuffer(GL_NONE)` → FBO の完全性確認（`glCheckFramebufferStatus`）

> **解像度:** シャドウマップの解像度（`SHADOW_WIDTH` / `SHADOW_HEIGHT`）はウィンドウ解像度と独立して設定できる。
> 高いほど精細な影になるが VRAM を消費する。1024×1024 程度が学習用途では一般的。

---

## Deferred Shading

### 考え方

Forward Shading（従来）は「オブジェクトを描くたびに、そのピクセルでライト全灯ぶんの計算をする」方式でした。
オブジェクトが重なると、最終的に見えないピクセルのライティングまで計算してしまい無駄が出ます。

Deferred Shading は処理を2段階に分けます。

1. **Geometry パス** — 画面の各ピクセルについて「そこに写っている面の位置・法線・色」だけを G-Buffer に記録する。**ライティングは一切しない**
2. **Lighting パス** — G-Buffer を読みながらフルスクリーンクワッドを1回描画し、そこで初めてライティングする

こうすると、ライティング計算は「実際に画面に見えているピクセル数」ぶんだけで済み、オブジェクトの重なりや数に影響されなくなります。

### G-Buffer の構成（`Scene::initGBuffer()`）

| アタッチメント | 変数 | 内部フォーマット | 中身 |
|---|---|---|---|
| COLOR_ATTACHMENT0 | `gPosition_` | `GL_RGBA16F` | ワールド座標 |
| COLOR_ATTACHMENT1 | `gNormal_` | `GL_RGBA16F` | ワールド法線（ノーマルマップ適用後） |
| COLOR_ATTACHMENT2 | `gAlbedoSpec_` | `GL_RGBA8` | rgb=アルベド, a=スペキュラ強度 |

**位置と法線に浮動小数点フォーマット（16F）が必須な理由**は、どちらも `[0,1]` に収まらない値だからです。
座標は 20 のような大きな値を取り、法線は `-1` のような負の値を取ります。
`GL_RGBA8` は `[0,1]` に丸められる固定小数点なので、ここに入れると情報が壊れます。

一方アルベドとスペキュラ強度はどちらも `[0,1]` なので `GL_RGBA8` で十分です。

> **`GL_RGB16F` ではなく `GL_RGBA16F` を使うこと。**
> OpenGL の必須フォーマット表では、RGB16F は「テクスチャとしては必須／レンダーターゲットとしては非必須」に
> 分類されています。3成分しか使わなくても RGBA を選ぶのが安全です。LearnOpenGL も RGBA16F を使っています。

### シャドウとの組み合わせ

シャドウマップの生成（Pass 1）は Deferred 化しても**まったく変わりません**。変わるのは「シャドウ判定をどこでやるか」だけです。

- **Geometry パスではシャドウ判定をしない。** ここでやってしまうと、結局オブジェクトの数だけ重い計算をすることになり、Deferred にした意味が消えます
- **Lighting パスで判定する。** G-Buffer から読んだ `gPosition` がそのまま `ShadowCalculation()` の `fragPos` 引数に使えます

したがって `shadowMap[4]` のサンプラーと `ShadowCalculation()` 関数は `gbuffer_*.frag` ではなく
`deferred_lighting.frag` 側に置きます。

### 深度バッファの引き継ぎ

Geometry パスは `gBuffer_` に、後続の前方描画は `framebuffer_` に描きます。この2つは**別々の深度バッファ**を持っています。

そのままだと `framebuffer_` の深度は空のままなので、スカイボックスが手前のキューブを無視して画面全体を覆ったり、ライトキューブが壁の裏にあるのに手前に描かれたりします。これを防ぐため、Lighting パスの前に深度だけをコピーします。

```cpp
glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer_);
glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer_);
glBlitFramebuffer(0, 0, scrWidth_, scrHeight_, 0, 0, scrWidth_, scrHeight_,
                  GL_DEPTH_BUFFER_BIT, GL_NEAREST);
```

`GL_FRAMEBUFFER` は読み込み先と書き込み先を同時に指定しますが、`glBlitFramebuffer` を使うときだけは
`GL_READ_FRAMEBUFFER` / `GL_DRAW_FRAMEBUFFER` で別々に指定できます。
最後の `GL_NEAREST` は、深度・ステンシルのコピーでは仕様上 `GL_LINEAR` が使えないため固定です。

> **コピー後の `glClear` に注意。** せっかくコピーした深度を、その後のパスで
> `glClear(GL_DEPTH_BUFFER_BIT)` してしまうと台無しになります。
> Lighting パスでクリアしてよいのはカラーだけです。

### Lighting パスで深度テストを無効にする理由

Lighting パスはフルスクリーンクワッドを描くだけなので、深度テストは不要です。
むしろ有効なままだとクワッド（画面手前に置かれる）が既存の深度と衝突して描画されないことがあります。

なお **OpenGL では `GL_DEPTH_TEST` を無効にすると深度書き込みも行われません。**
そのため `glDisable(GL_DEPTH_TEST)` しておけば、コピーしてきた深度がクワッドによって上書きされる心配もありません。

---

## 頂点属性 location の割り当て規約

**全 VAO・全頂点シェーダーで以下の割り当てに統一すること。**

| location | 意味 |
|---|---|
| 0 | position |
| 1 | normal |
| 2 | uv |
| 3 | tangent |
| 4 | bitangent |
| 5 | インスタンスごとの位置オフセット（`glVertexAttribDivisor(5, 1)`） |

### なぜ規約が必要か

VAO ごとに location の意味がバラバラだと、**複数のメッシュを同じシェーダーで描くパス**で必ず破綻します。
実際にこのプロジェクトでは、以前 `cubeVAO_` が location 3 を「インスタンス位置」に、
`wallVAO_` が location 3 を「タンジェント」に使っていました。

その状態でシャドウデプスパス（`point_shadow_depth.vert` は location 3 を `aOffset` として読む）から
壁を描くと、壁の各頂点が**タンジェントベクトルのぶんだけずれた位置**でシャドウマップに焼かれます。
エラーは一切出ず、影の位置だけが微妙にずれるという分かりにくい不具合になります。

**インスタンス用の属性を末尾（5）に置く**のが要点です。非インスタンス描画の VAO は 5 を使わないので、衝突しません。

### 使わない location はどうなるか

床（`planeVAO_`）は location 5 を `glEnableVertexAttribArray` していませんが、
`point_shadow_depth.vert` は `aOffset` を宣言しています。これは意図的に成立させています。

OpenGL では、**頂点属性配列が無効の場合、シェーダーは「カレント汎用頂点属性値」を読み**、その初期値は `(0, 0, 0, 1)` と規定されています。
つまり `aOffset` は `(0,0,0)` になり、`aPos + aOffset` は元の座標のままになります（加算の単位元）。

> **ただしこの値はコンテキストの状態であって VAO の状態ではありません。**
> どこかで `glVertexAttrib3f(5, ...)` を呼ぶと、location 5 を無効にしている全ての描画がその値を拾ってしまいます。
> このプロジェクトでは誰も呼んでいないので成立していますが、依存していることは意識しておいてください。

---

## 新しい3Dオブジェクトの追加

### 1. GeometryData.h に頂点データを追加

```cpp
inline const std::array<Vertex, 4> rawMyObjectVertices = { {
    {{ x, y, z }, {}, { u, v }},
    // ...
} };
inline const std::array<unsigned int, 6> myObjectIndices = { 0, 1, 2, 2, 3, 0 };
inline const std::array<Vertex, 4> myObjectVertices = calculateFaceNormals(rawMyObjectVertices);
```

法線は `calculateFaceNormals()` が自動計算します（4頂点/面の構造が前提）。

**平面の頂点巻き順と法線の関係:**

`calcNormal` は `cross(v1 - v0, v2 - v1)` で法線を計算します。
上向き法線（0, 1, 0）が欲しい床の場合、頂点を LF → RF → RB → LB の順に並べると正しくなります：

```
LB --- RB       巻き順: 0(LF) → 1(RF) → 2(RB) → 3(LB)
|      |        EBO:   { 0, 1, 2, 2, 3, 0 }
LF --- RF       法線:  cross(RF-LF, RB-RF) = (0,1,0) ✓
```

巻き順が逆（LF → LB → RB → RF）だと法線が下向きになり、上方からのライトが当たらなくなります。

### 2. Scene.h にVAO/VBO/EBOを追加

```cpp
unsigned int myObjectVAO_, myObjectVBO_, myObjectEBO_;
```

### 3. initMesh() でバインド設定

```cpp
int stride = sizeof(gl::Vertex);
glGenVertexArrays(1, &myObjectVAO_);
glGenBuffers(1, &myObjectVBO_);
glGenBuffers(1, &myObjectEBO_);
glBindVertexArray(myObjectVAO_);
glBindBuffer(GL_ARRAY_BUFFER, myObjectVBO_);
glBufferData(GL_ARRAY_BUFFER, sizeof(gl::myObjectVertices), gl::myObjectVertices.data(), GL_STATIC_DRAW);
glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, myObjectEBO_);
glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(gl::myObjectIndices), gl::myObjectIndices.data(), GL_STATIC_DRAW);
glEnableVertexAttribArray(0);
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(gl::Vertex, position));
glEnableVertexAttribArray(1);
glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(gl::Vertex, normal));
glEnableVertexAttribArray(2);
glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(gl::Vertex, uv));
glBindVertexArray(0);
```

### 4. Render() で描画

```cpp
shader_->use();
glBindVertexArray(myObjectVAO_);
myTexture_->bind(0);
shader_->setMat4("model", glm::mat4(1.0f));
glDrawElements(GL_TRIANGLES, gl::myObjectIndices.size(), GL_UNSIGNED_INT, 0);
```

### 5. デストラクタに削除を追加

```cpp
glDeleteVertexArrays(1, &myObjectVAO_);
glDeleteBuffers(1, &myObjectVBO_);
glDeleteBuffers(1, &myObjectEBO_);
```

---

## インスタンシングの追加

同じメッシュを複数の位置に描画するときに使います。

### 1. GeometryData.h に位置配列を追加

```cpp
inline const std::vector<glm::vec3> myObject_pos = {
    glm::vec3(0.0f, 0.0f, 0.0f),
    glm::vec3(3.0f, 0.0f, 0.0f),
};
```

### 2. Scene.h にインスタンスVBOを追加

```cpp
unsigned int myObjectInstanceVBO_;
```

### 3. initMesh() でインスタンスVBOを設定（VAO バインド中に行うこと）

```cpp
glBindBuffer(GL_ARRAY_BUFFER, myObjectInstanceVBO_);
glBufferData(GL_ARRAY_BUFFER, sizeof(gl::myObject_pos), gl::myObject_pos.data(), GL_STATIC_DRAW);
glEnableVertexAttribArray(5);  // location = 5（[頂点属性 location の割り当て規約](#頂点属性-location-の割り当て規約) 参照）
glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
glVertexAttribDivisor(5, 1);   // インスタンスごとに1回進む
glBindBuffer(GL_ARRAY_BUFFER, 0);
```

> **注意:** `glVertexAttribDivisor` は必ず VAO がバインドされている間（`glBindVertexArray(0)` の前）に呼ぶこと。
> また、インスタンス属性は必ず location 5 を使うこと。3 や 4 を使うと tangent / bitangent と衝突する。

### 4. .vert シェーダーに instance 属性を追加

```glsl
layout (location = 5) in vec3 aOffset;

void main() {
    mat4 instanceModel = mat4(1.0);
    instanceModel[3] = vec4(aOffset, 1.0);  // 平行移動列を設定
    gl_Position = projection * view * instanceModel * vec4(aPos, 1.0);
}
```

### 5. Render() で描画

```cpp
glDrawElementsInstanced(GL_TRIANGLES, gl::myObjectIndices.size(),
    GL_UNSIGNED_INT, 0, gl::myObject_pos.size());
```

### 毎フレーム位置が変わる場合（glBufferSubData パターン）

`GL_DYNAMIC_DRAW` で確保し、毎フレーム `glBufferSubData` で更新します：

```cpp
// initMesh(): GL_STATIC_DRAW → GL_DYNAMIC_DRAW に変更
glBufferData(GL_ARRAY_BUFFER, N * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);

// Render(): ソート済み配列を毎フレーム書き込む
glBindBuffer(GL_ARRAY_BUFFER, myObjectInstanceVBO_);
glBufferSubData(GL_ARRAY_BUFFER, 0, sortedPositions.size() * sizeof(glm::vec3), sortedPositions.data());
glBindBuffer(GL_ARRAY_BUFFER, 0);
```

---

## 新しいシェーダーの追加

### 1. shader_src/ にファイルを作成

`myshader.vert` / `myshader.frag` を `shader_src/` に作成します。
最低限のテンプレート（UBO 対応版）:

```glsl
// myshader.vert
#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 2) in vec2 aTexCoords;

layout (std140, binding = 0) uniform Matrices {
    mat4 view;
    mat4 projection;
};
uniform mat4 model;

out vec2 TexCoords;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    TexCoords = aTexCoords;
}
```

### 2. CMakeLists.txt にコピー設定を追加

`add_custom_command` のシェーダーコピーリストに追記します：

```cmake
${CMAKE_SOURCE_DIR}/shader_src/myshader.vert
${CMAKE_SOURCE_DIR}/shader_src/myshader.frag
```

### 3. Scene.h にメンバを追加

```cpp
std::unique_ptr<gl::Shader> myShader_;
```

### 4. コンストラクタで生成

```cpp
myShader_ = std::make_unique<gl::Shader>("myshader.vert", "myshader.frag");
```

---

## テクスチャの追加

### 通常テクスチャ

```cpp
// Scene.h
std::shared_ptr<Texture> myTexture_;

// initTextures()
myTexture_ = cache_.get("resources\\textures\\filename.png", true);
// 第2引数: true = 上下反転あり（通常はtrue）、アルファ付きPNGも自動判別

// Render()
myTexture_->bind(0);  // テクスチャユニット0にバインド
```

### キューブマップ（スカイボックス用）

```cpp
// 6面の画像パスを順番通りに渡す
std::vector<std::string> faces = {
    "resources\\textures\\skybox\\right.jpg",
    "resources\\textures\\skybox\\left.jpg",
    "resources\\textures\\skybox\\top.jpg",
    "resources\\textures\\skybox\\bottom.jpg",
    "resources\\textures\\skybox\\front.jpg",
    "resources\\textures\\skybox\\back.jpg",
};
cubemapTexture_ = cache_.loadCubemap(faces, false);  // false = 反転なし
```

---

## Linux (WSL2) で動かす

普段の開発は Windows (Visual Studio) ですが、WSL2 上でもビルド・実行できます。
ただし **Windows では何の問題もなく動くコードが Linux で弾かれたり落ちたりします。**
ドライバの違いによるもので、原因が分かりにくいのでここにまとめておきます。

### ビルドと実行

```bash
export VCPKG_ROOT=/path/to/vcpkg          # 初回のみ
cmake --preset linux-debug                 # configure（CMakeLists.txt を変えたときだけ）
cmake --build --preset linux-debug         # ビルド
cd build/linux-debug && GALLIUM_DRIVER=d3d12 ./glfwdojo
```

シェーダーと `resources/` を相対パスで読んでいるため、**実行時は必ず `build/linux-debug` に `cd` してから**起動します。

> **シェーダーだけ編集したときは反映されないことがある。**
> シェーダーのコピーは `add_custom_command(TARGET glfwdojo POST_BUILD ...)` で行われるため、
> C++ に変更がなくリンクが発生しないと、コピーごとスキップされます。
> `cp shader_src/* build/linux-debug/` で手動コピーするか、`touch src/main.cpp` してからビルドしてください。
> （これは Windows 側でも同じ挙動です）

### GPU を使う: `GALLIUM_DRIVER=d3d12`

これを付けないと **`llvmpipe`（CPU による純ソフトウェアレンダリング）にフォールバックして極端に遅くなります。**

```
（付けない）OpenGL renderer string: llvmpipe (LLVM 20.1.2, 256 bits)
（付ける）  OpenGL renderer string: D3D12 (AMD Radeon(TM) Graphics)
```

WSL2 では GPU が通常の DRM デバイスではなく `/dev/dxg` 経由の D3D12 マッピングレイヤーとして見えるため、
Mesa の自動ドライバ選択に乗りません。明示指定が必要です。

- **`MESA_LOADER_DRIVER_OVERRIDE=d3d12` は効きません。** これは DRI ドライバ名の指定で、Gallium のドライバ選択とは別系統です
- `GALLIUM_DRIVER=d3d12` にすると OpenGL 4.6 コアプロファイルがネイティブで出るので、`MESA_GL_VERSION_OVERRIDE` は不要です
- 恒久化するなら `~/.zshrc` に `export GALLIUM_DRIVER=d3d12`

環境が整っているかは以下で確認できます（すべて揃っていても自動選択されない点に注意）。

```bash
ls /dev/dxg                 # GPU パススルーのデバイス
ls /usr/lib/wsl/lib         # libd3d12.so があるか
ls /usr/lib/x86_64-linux-gnu/dri/d3d12_dri.so
GALLIUM_DRIVER=d3d12 glxinfo -B | grep -i renderer
```

### 罠1: サンプラー配列をループ変数で添字すると Mesa では弾かれる

```
error: sampler arrays indexed with non-constant expressions are forbidden in GLSL 1.30 and later
```

**症状:** Windows では通るのに Linux でシェーダーのコンパイルエラーになる。

**原因:** `shadowMap[i]` のように**サンプラー配列をループ変数で添字**していること。
GLSL 1.30〜3.30 の仕様では、サンプラー配列は**定数式でしか添字できません**。
GLSL 4.00 以降で「dynamically uniform expression」（全フラグメントで同じ値になる式。ループ変数はこれに該当）による添字が正式に許可されました。

NVIDIA / AMD の Windows ドライバはループを展開して**仕様違反でも黙って通してしまう**ため、Windows だけで開発していると気づけません。Mesa は仕様に厳密なので拒否します。

**対処:** そのシェーダーの `#version` を **400 以上**（このプロジェクトでは 460）にする。

### 罠2: struct のメンバに sampler があると d3d12 ドライバが segfault する

**症状:** シェーダーのコンパイルエラーは出ず、`GALLIUM_DRIVER=d3d12` で実行した瞬間に
`segmentation fault (core dumped)`。llvmpipe では正常に動く。

**原因:** 以下のように **struct のメンバに sampler を持たせている**こと。

```glsl
struct Material {
    vec3 ambient;
    sampler2D diffuse;    // ← これがあるだけで落ちる
    sampler2D specular;   // ← 同上
    float shininess;
};
uniform Material material;
```

Mesa の d3d12 ドライバは、このシェーダーを DXIL に変換する段階でクラッシュします。
**その sampler を実際に使っていなくても落ちます。** 実際このプロジェクトでは
`material.diffuse` を参照する `CalcDirLight()` / `CalcSpotLight()` は一度も呼ばれていない死にコードでしたが、
宣言が存在するだけでクラッシュしていました。

**対処:** struct から sampler メンバを外し、`uniform sampler2D` として単独で宣言する。
`float shininess` のような非 sampler メンバは struct に残して構いません。

> **`cube.frag` にも同じパターンが残っています。** 現在 `cubeShader_` はコメントアウトされているので
> 影響しませんが、再び有効化すると Linux で同じクラッシュが起きます。

### ドライバ内でクラッシュしたときの調べ方

セグフォがドライバ内部で起きると、`gdb` が無い環境では原因が全く分かりません。
`LD_PRELOAD` で `SIGSEGV` ハンドラを差し込むと、最低限のバックトレースが取れます。

```c
// segvtrace.c → gcc -shared -fPIC -o segvtrace.so segvtrace.c
#define _GNU_SOURCE
#include <execinfo.h>
#include <signal.h>
#include <unistd.h>
static void handler(int sig) {
    void *bt[64];
    backtrace_symbols_fd(bt, backtrace(bt, 64), STDERR_FILENO);
    _exit(1);
}
__attribute__((constructor)) static void init(void) {
    struct sigaction sa = {0};
    sa.sa_handler = handler;
    sa.sa_flags = SA_NODEFER | SA_RESETHAND;
    sigaction(SIGSEGV, &sa, NULL);
}
```

```bash
GALLIUM_DRIVER=d3d12 LD_PRELOAD=./segvtrace.so ./glfwdojo
```

スタックに `libgallium-*.so` が出れば**アプリではなくドライバ内でのクラッシュ**と確定します。
そこから先は、`Render()` の途中に `return;` を差し込んで**どのパスまでなら生き残るかを二分探索**するのが確実です。
今回はこの方法で `renderTransparentWindows()` → そこだけが使う `shader.frag` → `struct Material` の sampler、と絞り込みました。

> **`LD_PRELOAD` で GL 関数や GLFW 関数を横取りする方法は、このプロジェクトでは効きません。**
> vcpkg の Linux 向けトリプレットは既定で静的リンクなので、`glfwSwapBuffers` などはバイナリに埋め込まれており、
> 動的リンカの介入余地がありません。シグナルハンドラの差し込み（コンストラクタ属性）は静的リンクでも機能します。

---

## 画面が真っ黒・真っ白になったときの調べ方

グラフィックスの不具合は**エラーが一切出ないまま画面だけがおかしくなる**ことがほとんどです。
勘で直そうとすると延々と時間を溶かすので、以下の順で機械的に切り分けてください。

### 原則1: パスを1つずつ「中身を画面に出して」確認する

多段パス構成では、どのパスまで正しいかを1つずつ確認するのが最短です。
`deferred_lighting.frag` の先頭には、そのための `DEBUG_MODE` スイッチが用意してあります。

| 値 | 表示内容 |
|---|---|
| 0 | 通常のライティング |
| 1 | ライト0のシャドウ判定だけ |
| 2 | `shadowMap[0]` の生の深度値 |
| 3 / 4 / 5 | G-Buffer の Albedo / Normal / Position |
| 6 | 画面4分割で上記を一度に表示 |

同様に `gbuffer_floor.frag` には `GBUFFER_WRITE_TEST` があり、
1 にすると床を描くときに G-Buffer へ**位置や法線と無関係な固定色**を書き込みます。
これで「書き込んだ値が悪いのか、書き込み自体が届いていないのか」を切り分けられます。

### 原則2: デバッグ表示のときはトーンマッピングを必ず切る

**ここは非常に引っかかりやすいポイントです。**

`hdr.frag` は `mapped = 1 - exp(-color * exposure)` の後に `pow(mapped, 1/2.2)` をかけます。
exposure が 3.0 のとき、値の見え方はこうなります。

| 元の値 | 画面上 |
|---|---|
| 0.1 | 0.55（中間グレー） |
| 0.5 | 0.89（ほぼ白） |
| 1.0 | 0.98（白） |

つまり `normal * 0.5 + 0.5` のような**正常な値でも一律に真っ白に見えてしまい、判定に使えません。**

`hdr.frag` の `DEBUG_RAW_OUTPUT` を 1 にすると Bloom 合成・トーンマッピング・ガンマ補正をすべて飛ばし、
生の値をそのまま出力します。G-Buffer を可視化するときは**必ず 1 にしてください。**

### 原則3: 「一様な色」は入力が定数であることを意味する

出力にピクセルごとの変化が全くない場合、その計算に使われている入力のどれかが定数です。
例えば `texture(shadowMap[0], FragPos - lightPos)` が一様なら、
`FragPos` が全ピクセルで同じ（＝ `gPosition` が読めていない）か、シャドウマップの中身が一様か、の2択に絞れます。

### 原則4: シャドウの確認は必ず1灯ずつ

`ShadowCalculation()` は**ライトに背を向けている面でも 1.0 を返します**。
その面とライトの間には自分自身の裏側があるので、深度比較が成立してしまうためです。

したがって複数灯の shadow を `max()` でまとめて表示すると、
「どの面も4灯のうち最低1灯には背を向けている」ため**ほぼ全面が白くなるのが正常**で、何も切り分けられません。
必ず1灯だけを見てください。

### 原則5: 推測が2回外れたら、GL に直接問い合わせる

FBO の状態は `glGetFramebufferAttachmentParameteriv` や `glGetIntegerv(GL_DRAW_BUFFER0 + i, ...)` で
実際に問い合わせられます。「設定したつもり」ではなく「GL がどう認識しているか」を見るのが確実です。

```cpp
// 各カラーアタッチメントに、意図したテクスチャが実際に付いているか
GLint objName = 0;
glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i,
    GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &objName);

// フラグメントシェーダーの出力 location N がどのアタッチメントへ向いているか
GLint db = 0;
glGetIntegerv(GL_DRAW_BUFFER0 + i, &db);   // 0x8CE0 = GL_COLOR_ATTACHMENT0

// 溜まっているエラーを全部吐き出す
GLenum err;
while ((err = glGetError()) != GL_NO_ERROR)
    std::cout << "GL error: 0x" << std::hex << err << std::endl;
```

ここがすべて正常なら、原因は「設定」ではなく「**書き込んだ後に何かが打ち消している**」側にあります
（実際にそれが下記のブレンディングの問題でした）。

---

## よくある落とし穴

### G-Buffer への書き込みがブレンドで消える（最重要）

**症状:** G-Buffer の `gPosition` と `gNormal` だけが常にクリア値のまま。`gAlbedoSpec` は正常。
エラーもFBOの不完全も一切出ない。結果としてライティングもシャドウも全く効かない。

**原因:** `GL_BLEND` が有効なまま Geometry パスを実行していること。

`Window.cpp` では透過窓のために起動時に一度だけ有効化しています。

```cpp
glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
```

これを無効化せずに Geometry パスを走らせると、`最終値 = src.rgb * src.a + dst.rgb * (1 - src.a)` が適用されます。
ここで G-Buffer のシェーダーを見ると:

```glsl
layout (location = 0) out vec3 gPosition;   // vec3 → アルファ成分が「未定義」
layout (location = 1) out vec3 gNormal;     // vec3 → 同上
layout (location = 2) out vec4 gAlbedoSpec; // vec4 → アルファを明示的に書いている
```

**`vec3` で宣言した出力のアルファ成分は未定義**（実際には 0）です。したがって前2枚は
`src.rgb * 0 + dst.rgb * 1` となり、**書き込みが毎フレーム完全に消えます。**
`gAlbedoSpec` だけ生き残るのでアルベドは正常に見え、原因が非常に見つけにくくなります。

**対処:** Geometry パスの冒頭で `glDisable(GL_BLEND)`、透過窓の描画前後だけ有効化する。

そもそも G-Buffer に入るのは色ではなく座標や法線という**幾何情報**なので、
ブレンドという概念自体が意味を持ちません。Deferred Shading では必ず無効にします。

### MRT では有効な全ての draw buffer に書き込む

`glDrawBuffers` で複数のカラーアタッチメントを有効にしている場合、
フラグメントシェーダーが**書き込まなかったアタッチメントの値は「未定義」になります**。

このプロジェクトでは `framebuffer_` が `textureColorbuffer_`（location 0）と
`brightColorBuffer_`（location 1, Bloom用）の2枚を同時に有効にしています。
そこへ `skybox.frag` が `FragColor` しか書かないと、スカイボックスが覆う画面全域で
`brightColorBuffer_` にゴミが入り、それが10回のブラーで拡散して**画面全体に白い靄がかかります。**

Bloom させたくないシェーダーでも、必ず黒を明示的に書いてください。

```glsl
layout (location = 1) out vec4 BrightColor;
// ...
BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
```

### 影が見えないときは「明るすぎ」を疑う

影が出ない原因は、シャドウマップの不具合とは限りません。以下の3つが重なると影は簡単に見えなくなります。

1. **ライトの到達距離が長すぎる** — 4灯すべてがシーン全域に届くと、1灯が遮られても残り3灯が影を埋めます。減衰係数で意図的に影響範囲を絞るのが有効です
2. **トーンマッピングの飽和** — `1 - exp(-c * exposure)` は明るい領域ほど差が潰れます。HDR値4.0は0.86、8.0は0.98と、ほとんど区別できなくなります
3. **Bloom の滲み** — 輝度1.0を超える面が広いと、ブラー結果が影の上に加算されます

**全体を明るくしたいときは、ライトの `diffuse` ではなく `exposure_` を上げてください。**
exposure は HDR 値そのものを変えないので、Bloom の閾値や飽和に影響せず、影のコントラストを保てます。

### 減衰係数の `constant` を 0 にしない

`attenuation = 1.0 / (constant + linear * d + quadratic * d²)` で `constant = 0, linear = 0, quadratic = 1`
とすると `1/d²` になります。これは光源の至近距離で発散し、少し離れると急激にゼロへ落ちます
（距離3で0.11、距離10で0.01）。結果、**光源の真下だけ白飛びして他は真っ暗**という極端な絵になります。

`constant = 1.0` にしておくと attenuation が 1.0 を超えなくなり、扱いやすくなります。
到達距離は `linear` / `quadratic` で調整してください（LearnOpenGL の減衰テーブルが目安になります）。

### 未設定の uniform は 0 になる（単位行列ではない）

GLSL では uniform の初期値はゼロです。`mat4 model` を設定し忘れると**ゼロ行列**になり、
`model * vec4(aPos, 1.0)` が `(0,0,0,0)` になります。

これは「`FragPos` が全ピクセルで `(0,0,0)`」という症状として現れるので、
G-Buffer の Position が一様になったときはまずここを疑ってください。

### ライトキューブを FBO の外で描画してしまう

`glBindFramebuffer(GL_FRAMEBUFFER, 0)` でデフォルト FB に戻った後にライトキューブを描画すると：
- `glDisable(GL_DEPTH_TEST)` が有効な状態なので全オブジェクトの手前に描画される
- カスタム FB を通さないのでポストプロセス（ガンマ補正等）が適用されない
- 結果として「2D の板ポリ」に見える

**必ず `glBindFramebuffer(GL_FRAMEBUFFER, 0)` より前のブロックで描画すること。**

### `glDrawElements` のインデックス数をハードコードしない

```cpp
// NG: キューブ（12三角形 = 36インデックス）なのに 24 を指定すると 4 面しか描画されない
glDrawElements(GL_TRIANGLES, 24, GL_UNSIGNED_INT, 0);

// OK: cubeIndices のサイズ（= 36）を直接参照する
glDrawElements(GL_TRIANGLES, gl::cubeIndices.size(), GL_UNSIGNED_INT, 0);
```

### normalMatrix を各シェーダーの描画前に設定する

フラグメントシェーダー内で法線を正しくワールド空間に変換するには、C++ 側から `normalMatrix` を送る必要があります。設定を忘れるとゼロ行列になり、法線がすべて (0,0,0) になってライティングが真っ黒になります。

```cpp
shader_->setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
// モデル行列が単位行列の場合は glm::mat3(1.0f) でも同じ
```

複数のシェーダー（`shader_`, `cubeShader_`, `transparentwindowShader_`）それぞれに設定すること。

### 同じ .frag でも uniform はシェーダープログラムごとに設定する

`cubeShader_` と `shader_` が同じ `shader.frag` を使っていても、プログラムオブジェクトは別です。
`viewPos` や `pointLights[0]` などの uniform は、`use()` した各シェーダーに対して個別に `setVec3` / `setFloat` を呼ぶ必要があります。

### `std::vector` に `sizeof` を使ってはいけない

```cpp
// NG: vector オブジェクトのサイズ（24 bytes）が返る
glBufferData(GL_ARRAY_BUFFER, sizeof(gl::windows_pos), ...);

// OK: データの実際のバイト数
glBufferData(GL_ARRAY_BUFFER, gl::windows_pos.size() * sizeof(glm::vec3), ...);
```

`std::array` は `sizeof` でデータサイズが取れます。`std::vector` は必ず `.size() * sizeof(要素型)` を使うこと。

### VAO の外で `glVertexAttribPointer` を呼ばない

VAO はバインド中に呼ばれた `glVertexAttribPointer` と `glVertexAttribDivisor` の設定を記録します。`glBindVertexArray(0)` の後に呼んでも VAO に記録されません。

### ヘッダで変数定義するときは `inline` をつける

`GeometryData.h` のように複数の `.cpp` からインクルードされるヘッダで変数を定義するときは `inline` が必須です。ないと ODR（One Definition Rule）違反でリンクエラーになります：

```cpp
inline const std::vector<glm::vec3> myPositions = { ... };  // OK
const std::vector<glm::vec3> myPositions = { ... };         // NG（複数回定義エラー）
```

### UBO のメンバ順序を統一する

複数のシェーダーが同じ UBO（binding=0）を使う場合、すべてのシェーダーで **メンバの順序・型が完全一致** している必要があります。順序が違うと行列の値が入れ違いになります。

### 透明オブジェクトは必ず後から描画する

アルファブレンディングはフレームバッファに既にある色と合成します。透明オブジェクトを不透明オブジェクト・スカイボックスより先に描くと、背景が合成されません。

描画順: **不透明 → スカイボックス → 透明（後方から前方の順）**

### `unique_ptr` を初期化せずに使うとクラッシュ

コンストラクタでコメントアウトしたシェーダー生成を残したまま `Render()` で `->use()` を呼ぶと、`nullptr` 参照でメモリ違反になります。使わないシェーダーは `Render()` 側の呼び出しもコメントアウトするか、メンバ宣言ごと削除してください。

---

## 今後の課題

現状の実装で「動いてはいるが、より正確・発展的にするなら改善余地がある」と分かっている点をまとめておきます。学習を進める中でLearnOpenGLの該当章に戻って取り組む際の参考にしてください。

### Parallax Occlusion Mapping に自己遮蔽（セルフシャドウ）がない

`cube.frag` の `ShadowCalculation()` は、床など**他のオブジェクトが光を遮る**影（ポイントシャドウ／depth cubemapベース）しか計算していません。一方、レンガの凹み（モルタルの溝）自体が凹みの奥に光が届くのを遮るという**自己遮蔽**は未実装です。そのため、斜めから覗き込んだときに幾何学的には影になるはずの凹みの奥が、法線マップの傾き次第では明るく計算されてしまうことがあります。LearnOpenGLのParallax Mappingの章にある発展的な内容（ハイトマップに沿って光源方向へレイをマーチングし遮蔽を検出する手法）を実装すると解決できます。

### シャドウマッピングが基本的なPCFのみ

現在の点光源シャドウ（`ShadowCalculation()`）は、固定オフセット（0.05）・26方向サンプリングのPCFで縁をぼかしているだけの、比較的古典的な実装です。以下のような発展手法は未導入です:
- **PCSS（Percentage Closer Soft Shadows）**: 光源とオクルーダーの距離に応じて影の輪郭のぼけ方を可変にする
- **カスケードシャドウマップ**: 遠近でシャドウマップの解像度を分割し、シーン全体の精度を上げる（現状は directional light 自体が未使用なので該当なし）
- より高度な bias 制御（Peter panning対策など）

また、`DirectionalLight`/`SpotLight`（下記）にはそもそもシャドウの仕組み自体がありません。

### `DirectionalLight` / `SpotLight` が実装済みだが未使用

`Lighting.h`/`Lighting.cpp` には `PointLight` 以外に `DirectionalLight` と `SpotLight` の構造体・`applyToShader()` が実装済みですが、`Scene.h` 側のメンバ（`directionalLight_`, `spotlight_`）はコメントアウトされたままで、シェーダー側の `CalcDirLight()`/`CalcSpotLight()` の呼び出しも無効化されています。複数種類の光源を同時に使うシーンを試す際は、ここを有効化するところから始めることになります。

### `Material` クラスが実質使われていない

`Material` クラス(`material_.setUniforms(shader)`)は `shininess` などをシェーダーに送る役割を持つよう設計されていますが、実際には `Scene::initTextures()` 内の呼び出し(`material_.setUniforms(*shader_)`)がコメントアウトされており、代わりに `Render()` のあちこちで `shader.setFloat("material.shininess", 32.0f)` と値がハードコードされています。オブジェクトごとに異なる質感を持たせたくなったときに詰まりやすいポイントなので、`Material` を実際に活用する形に整理する余地があります。

### 環境光（ambient）が定数のみで、遮蔽を考慮していない

現在の `ambient` 項は `light.ambient`(定数値)をテクスチャ色に掛けているだけで、床や周囲のオブジェクトによる遮蔽は一切反映されません。そのため、物陰になっている箇所でも同じ明るさの環境光が乗ってしまいます。SSAO（Screen Space Ambient Occlusion）などを導入すると、隅や接地面が自然に暗くなり立体感が増します。

### Deferred Shading 化に伴い整理しきれていない箇所

- **`Material` クラスが Deferred でさらに宙に浮いた** — G-Buffer は `gAlbedoSpec.a` にスペキュラ強度を1チャンネルしか持てず、`shininess` は `deferred_lighting.frag` 内で `32.0` にハードコードされています。オブジェクトごとに質感を変えたい場合は、G-Buffer にもう1チャンネル用意するか、マテリアルIDを埋め込む設計が必要です
- **スペキュラ強度がアルベドの輝度で代用されている** — 専用のスペキュラマップがないため、`gbuffer_*.frag` では `dot(albedo, vec3(0.2126, 0.7152, 0.0722))` を代用値にしています。本来はマテリアルごとのスペキュラマップを読むべきところです
- **Forward 版のシェーダーが残っている** — `cube.frag` / `wall.frag` は Deferred 化で使われなくなりました。`shader.frag` は透過窓が引き続き使うので残す必要があります
- **`shadow_mapping_depth.vert` が旧 location 規約のまま** — directional shadow 用で現在ロードされていませんが、使う場合は `aOffset` を location 5 に直す必要があります

### G-Buffer の帯域を削減できる

現在は `gPosition` にワールド座標をそのまま `RGBA16F` で持っていますが、
**深度バッファから座標を復元**すれば、この1枚（画面サイズ × 8バイト）を丸ごと削減できます。
法線も、正規化されている前提で2成分に圧縮する手法（八面体エンコードなど）があります。
Deferred Shading の実用上のボトルネックは G-Buffer の帯域なので、次のステップとして自然な最適化です。

### ライトボリュームによる最適化が未実装

現在の Lighting パスは、全ピクセルについて必ず4灯ぶんのループを回しています。
LearnOpenGL の Deferred Shading の章の後半にある **ライトボリューム**（各ライトの影響半径ぶんの球を描き、
その内側のピクセルだけライティングする手法）を導入すると、ライトの数を大きく増やせるようになります。
