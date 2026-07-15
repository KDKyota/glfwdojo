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
7. [新しい3Dオブジェクトの追加](#新しい3dオブジェクトの追加)
8. [インスタンシングの追加](#インスタンシングの追加)
9. [新しいシェーダーの追加](#新しいシェーダーの追加)
10. [テクスチャの追加](#テクスチャの追加)
11. [よくある落とし穴](#よくある落とし穴)

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

```
カスタムフレームバッファへバインド
 ↓
UBO に view / projection を書き込む
 ↓
不透明オブジェクト（cube, floor）を描画
 ↓
ライトキューブを描画（← FBO バインド中に行うこと）
 ↓
スカイボックスを描画（GL_LEQUAL で最遠描画）
 ↓
透明オブジェクトをカメラ距離でソートして描画
 ↓
デフォルトフレームバッファへ戻す
 ↓
スクリーンクワッドにカラーテクスチャを貼って描画
```

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

`Window.cpp` のコンテキスト初期化後に以下を呼ぶだけで有効化できます：

```cpp
glEnable(GL_FRAMEBUFFER_SRGB);
```

これにより、フレームバッファへの書き込み時に OpenGL がガンマ補正（linear → sRGB 変換）を自動で行います。
シェーダー側での手動補正（`pow(color, 1.0/2.2)`）は不要になります。

> **注意:** スカイボックスなど、すでに sRGB 空間の画像を使っている場合は二重補正になることがある。
> テクスチャロード時に `GL_SRGB` / `GL_SRGB_ALPHA` を指定した場合は自動でリニア変換されるため問題ない。

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
glEnableVertexAttribArray(3);  // location = 3
glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
glVertexAttribDivisor(3, 1);   // インスタンスごとに1回進む
glBindBuffer(GL_ARRAY_BUFFER, 0);
```

> **注意:** `glVertexAttribDivisor` は必ず VAO がバインドされている間（`glBindVertexArray(0)` の前）に呼ぶこと。

### 4. .vert シェーダーに instance 属性を追加

```glsl
layout (location = 3) in vec3 aOffset;

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

## よくある落とし穴

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
