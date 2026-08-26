# glfwdojo 開発ガイド

このドキュメントは、既存の機能を変更したり新しい要素を追加するときの手順と注意点をまとめたものです。

**頭から読む文書ではありません。** 辞書として引いてください。
画面が壊れたときは下の「症状から探す」、作業手順を知りたいときは「目次」から該当節へ飛ぶ想定です。

> コードの編集だけでかなり大変なのでこのドキュメントはClaudeが作っています．
> もしかすると正確じゃないことを書いているかもしれません．

---

## 症状から探す

グラフィックスの不具合は**エラーが一切出ないまま画面だけがおかしくなる**ことがほとんどなので、
「何が見えているか」から原因に辿れるようにしてあります。

**画面全体の色がおかしい**

| 症状 | 原因 |
| --- | --- |
| 黒が浮いて靄がかかったように眠い。`Bloom` も `Ambient` も `Exposure` も 0 にして消えない | [ガンマ補正の二重掛け](#二重にかけると画面全体が白っぽくなる実際に踏んだ) |
| 環境を変えた途端に画面が白くなった | [`GL_FRAMEBUFFER_SRGB` は環境によって効いたり効かなかったりする](#二重にかけると画面全体が白っぽくなる実際に踏んだ) |
| 全体的に明るく色が薄い。ただし `Ambient` / `Exposure` を下げれば「それらしい絵」にはなる | [アルベドが sRGB のままリニア値として計算されている](#アルベドがリニア空間になっていなかった実際に踏んだ) |
| 画面全体に白い靄がかかる | [MRT で書き込まなかったアタッチメントのゴミが Bloom で拡散している](#mrt-では有効な全ての-draw-buffer-に書き込む) |
| モデルが横に倒れた状態で読み込まれる | [ルートノードの軸変換（`Z_UP` など）を持つモデル](#軸の向きが違うモデルは倒れて読み込まれる) |
| 陰影がなんとなく変。エラーは出ない | [法線マップ・視差マップを `SRGB` で読んでいる](#アルベドがリニア空間になっていなかった実際に踏んだ) |

**G-Buffer / Deferred Shading**

| 症状 | 原因 |
| --- | --- |
| `gPosition` と `gNormal` だけクリア値のまま。`gAlbedoRoughness` は正常 | [`GL_BLEND` が有効なまま Geometry パスを実行している](#g-buffer-への書き込みがブレンドで消える最重要) |
| G-Buffer の Position が画面一様 | [`model` uniform の設定漏れでゼロ行列になっている](#未設定の-uniform-は-0-になる単位行列ではない) |
| 出力にピクセルごとの変化が全く無い | [入力のどれかが定数になっている](#原則3-一様な色は入力が定数であることを意味する) |
| デバッグ表示が正常な値でも真っ白で判定できない | [トーンマッピングを切っていない](#原則2-デバッグ表示のときはトーンマッピングを必ず切る) |

**ライティング・影**

| 症状 | 原因 |
| --- | --- |
| 影が全く見えない | [シャドウマップではなく「明るすぎ」を疑う](#影が見えないときは明るすぎを疑う) |
| 光源の真下だけ白飛びして他は真っ暗 | [減衰係数の `constant` が 0](#減衰係数の-constant-を-0-にしない) |
| ライティングが真っ黒。法線が全て `(0,0,0)` | [`normalMatrix` の設定漏れ](#normalmatrix-を各シェーダーの描画前に設定する) |
| 全灯まとめてシャドウを表示するとほぼ全面が白い | [背を向けた面でも 1.0 が返るので正常。1灯ずつ見る](#原則4-シャドウの確認は必ず1灯ずつ) |
| ライトキューブが 2D の板ポリに見える／全オブジェクトの手前に出る | [FBO の外で描画している](#ライトキューブを-fbo-の外で描画してしまう) |
| uniform を設定したのに効かない | [同じ `.frag` でもプログラムごとに設定が必要](#同じ-frag-でも-uniform-はシェーダープログラムごとに設定する) |

**リソース・テクスチャ**

| 症状 | 原因 |
| --- | --- |
| そのテクスチャだけ真っ黒／FBO へ描いたはずのものが画面に直接出る。GL エラーは出ない | [`create()` の呼び忘れ](#ハンドル型を新しく追加するときの罠) |
| 新しく足したハンドル型で `glBind*` が効かない | [CRTP の型引数の直し忘れ](#ハンドル型を新しく追加するときの罠) |
| 行列の値が入れ違いになる | [UBO のメンバ順序が一致していない](#ubo-のメンバ順序を統一する) |

**描画されない・一部しか出ない**

| 症状 | 原因 |
| --- | --- |
| キューブの 4 面しか描画されない | [`glDrawElements` のインデックス数をハードコードしている](#gldrawelements-のインデックス数をハードコードしない) |
| バッファのサイズがおかしい | [`std::vector` に `sizeof` を使っている](#stdvector-に-sizeof-を使ってはいけない) |
| 頂点属性の設定が効かない | [VAO の外で `glVertexAttribPointer` を呼んでいる](#vao-の外で-glvertexattribpointer-を呼ばない) |
| 透明オブジェクトの背景が合成されない | [描画順が違う](#透明オブジェクトは必ず後から描画する) |

**キャラクターの挙動・衝突判定**

| 症状 | 原因 |
| --- | --- |
| 壁や箱に近づくとキャラクターが小刻みに震える | [判定と押し出しで違う半径を使っている](#判定と押し出しで違う半径を使うと壁際で震える) |
| キャラクターが画面から消える／座標が `NaN` になる | [中心が矩形の内側でゼロベクトルを正規化している](#中心が矩形の内側に入ると押し出す向きが決まらない) |
| 壁に斜めに当たると壁沿いに向き直ってしまう | [向きは入力方向のまま保つのが仕様](#向きは実移動方向ではなく入力方向のまま保つ設計判断) |
| 壁に沿って滑らずベタッと止まる | [押し出しが効いていない。スライドは押し出しの副作用で成立する](#押し出しだけでスライドは成立する) |
| 2つの箱の角に挟まると弾かれる／抜ける | [反復回数が足りない](#反復回数は角のために要る) |

**クラッシュ・ビルドが通らない**

| 症状 | 原因 |
| --- | --- |
| Windows では通るのに Linux でシェーダーのコンパイルエラー | [サンプラー配列をループ変数で添字している](#罠1-サンプラー配列をループ変数で添字すると-mesa-では弾かれる) |
| `GALLIUM_DRIVER=d3d12` で実行した瞬間に segfault。llvmpipe では動く | [struct のメンバに sampler がある](#罠2-struct-のメンバに-sampler-があると-d3d12-ドライバが-segfault-する) |
| 起動直後にメモリ違反 | [未初期化の `unique_ptr` を使っている](#unique_ptr-を初期化せずに使うとクラッシュ) |
| リンクエラー（複数回定義） | [ヘッダの変数定義に `inline` が無い](#ヘッダで変数定義するときは-inline-をつける) |
| 環境を替えたらシェーダーが `syntax error`。`expecting "::"` が出る | [変数名が GLSL の予約識別子と衝突している](#glsl-の予約識別子を変数名に使わない) |
| 一括整形をかけた直後から `glad.h` が「OpenGL ヘッダが既に include されている」と `#error` を出す | [clang-format の include 自動ソートが glad と GLFW を入れ替えている](#clang-format-の-include-自動ソートが-glad-と-glfw-を入れ替える) |

症状が上に無いときは [画面が真っ黒・真っ白になったときの調べ方](#画面が真っ黒真っ白になったときの調べ方) の切り分け手順から入ってください。

---

## 目次

1. [アーキテクチャ概要](#アーキテクチャ概要)
2. [GL リソースの持ち方](#gl-リソースの持ち方)
3. [UBO（Uniform Buffer Object）](#ubouniform-buffer-object)
4. [ガンマ補正](#ガンマ補正)
5. [ライトの変更](#ライトの変更)
6. [ライトキューブの追加](#ライトキューブの追加)
7. [シャドウマッピング：デプスマップ FBO](#シャドウマッピングデプスマップ-fbo)
8. [Deferred Shading](#deferred-shading)
9. [頂点属性 location の割り当て規約](#頂点属性-location-の割り当て規約)
10. [新しい3Dオブジェクトの追加](#新しい3dオブジェクトの追加)
11. [インスタンシングの追加](#インスタンシングの追加)
12. [新しいシェーダーの追加](#新しいシェーダーの追加)
13. [テクスチャの追加](#テクスチャの追加)
14. [Linux (WSL2) で動かす](#linux-wsl2-で動かす)
15. [画面が真っ黒・真っ白になったときの調べ方](#画面が真っ黒真っ白になったときの調べ方)
16. [よくある落とし穴](#よくある落とし穴)
17. [カメラ](#カメラ)
18. [衝突判定](#衝突判定)
19. [モデル読み込みと単位系](#モデル読み込みと単位系)
20. [今後の課題](#今後の課題)

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

**`Render()` 自体はこの順番を並べるだけで、各パスの中身は同名のプライベートメソッドにあります。**
パス同士は FBO とテクスチャを介して繋がっているので、順序には意味があります
（例: SSAO は G-Buffer が埋まっていないと計算できない）。

```
updateTransparentInstances()  透過窓をカメラから遠い順に並べ、インスタンスVBOへ位置を送る
 ↓
renderShadowPasses()          4灯ぶんループ。1灯につきサブパスが2つ
   [Pass 1]   depthMapFBO_[j] に深度のみ書き込む（glDrawBuffer(GL_NONE)）
              光源視点で floor / cube / wall / 窓枠 を描画
              → depthCubemap_[j] に「光源からの正規化距離」が焼かれる
   [Pass 1.5] 同じFBOで glDrawBuffer(GL_COLOR_ATTACHMENT0) に切り替え、白でクリア
              深度書き込みを止めたままガラスだけを乗算ブレンドで描画
              → shadowColorCubemap_[j] に「ガラスを透過した光の色」が焼かれる
 ↓
updateMatricesUBO()           UBO に view / projection を書き込む
 ↓
renderGeometryPass()          [Pass 2] Geometry パス
   glDisable(GL_BLEND)  ← 必須。理由は「よくある落とし穴」参照
   gBuffer_ にバインド → gbuffer_*.frag で cube / floor / wall / 窓枠 を描画
   → gPosition_ / gNormal_ / gAlbedoRoughness_ の3枚に「幾何情報」だけを書き込む
      （この時点ではライティングもシャドウ判定も一切しない）
 ↓
renderSSAOPass()              G-Buffer から遮蔽率を計算し、4x4 ブラーまでかける
   → ssaoColorBufferBlur_
 ↓
blitGeometryDepth()           gBuffer_ の深度を framebuffer_ へ glBlitFramebuffer でコピー
   ← これをしないと後続のスカイボックス等が正しく前後判定できない
 ↓
renderDeferredLightingPass()  [Pass 3] Lighting パス
   framebuffer_ にバインド
   → G-Buffer 3枚 + depthCubemap_ 4枚 + AO 1枚 + shadowColorCubemap_ 4枚をバインド
   → フルスクリーンクワッドを1回描画するだけで全ピクセルのライティングが完了
 ↓
renderForwardPass()           [Pass 4] 前方描画（G-Buffer に載せられないもの）
   ライトキューブ → スカイボックス → ガラス（この間だけ GL_BLEND を有効化）
 ↓
renderBloomBlur()             [Pass 5] Bloom
   brightColorBuffer_ を pingpongFBO_ で10回ガウシアンブラー
 ↓
renderToScreen()              デフォルトフレームバッファへ戻し、
   hdr.frag でブラー結果を加算し、トーンマッピング＋ガンマ補正して画面へ
```

> **なぜ透過窓とスカイボックスだけ前方描画なのか**
> Deferred Shading は「1ピクセルにつき1つの面の情報しか G-Buffer に保持できない」方式です。
> 半透明の面は「奥の面と手前の面の両方の色」が必要なので、原理的に G-Buffer に載せられません。
> スカイボックスとライトキューブは、そもそもライティング計算が不要（自分で発光している）なので
> 前方描画のほうが素直です。

---

## GL リソースの持ち方

**VAO / VBO / EBO / テクスチャ / FBO / RBO は、生の `unsigned int` ではなく
`GlHandle.h` の `gl::*Handle` で持ちます。**

| 対象 | 型 |
| --- | --- |
| VAO | `gl::VertexArrayHandle` |
| VBO / EBO / UBO | `gl::BufferHandle` |
| テクスチャ（2D・キューブマップ共通） | `gl::TextureHandle` |
| FBO | `gl::FramebufferHandle` |
| RBO | `gl::RenderbufferHandle` |

```cpp
// Scene.h
gl::TextureHandle myTexture_;
std::array<gl::FramebufferHandle, 4> myFBO_;   // 配列は std::array で

// init 系
myTexture_.create();                            // glGenTextures(1, &myTexture_) の代わり
glBindTexture(GL_TEXTURE_2D, myTexture_);       // 暗黙変換があるのでそのまま渡せる

// 解放は書かない。デストラクタが自動で行う
```

### なぜこうしているか

以前は `Scene` のデストラクタが37個の `glDelete*` を手作業で並べた42行でした。
リソースを1つ増やすたびに「生成する場所」と「解放する場所」という**離れた2箇所を
必ず同時に直さないと静かにリークする**、という手動の約束事になっていました。
リークはエラーも警告も出ないので、増えても気づけません。

PBR / IBL で irradiance map・prefilter map・BRDF LUT の FBO とテクスチャが
さらに3セット増えることが分かっていたため、手で守りきれなくなる前に仕組みへ移しました。

### なぜ `virtual` ではなく CRTP なのか

5種類のハンドルで異なるのは `glGen*` / `glDelete*` の2つだけで、ムーブもコピー禁止も
`reset()` も共通です。共通部分を基底クラスに置きたくなりますが、**`virtual` では実現できません。**

```cpp
class HandleBase {
    virtual void del(GLuint id) = 0;
    ~HandleBase() { if (id_ != 0) del(id_); }   // コンパイルは通るが動かない
};
```

デストラクタは派生 → 基底の順に走るので、`~HandleBase()` が動く時点で派生クラス部分は
**すでに破棄されています**。C++ は「コンストラクタ／デストラクタの実行中、オブジェクトの
動的型はそのコンストラクタ／デストラクタのクラスである」と定めているため、`del()` の
仮想呼び出しは `HandleBase::del` に解決されます。純粋仮想なので未定義動作
（多くの処理系で `pure virtual method called` と出て abort）。
**コンパイルも警告も通ってから実行時に落ちる**のが厄介な点です。

そこで派生型を型引数として基底に渡し、呼び出しをコンパイル時に解決します。

```cpp
template <typename Derived> class HandleBase {
    void reset(GLuint id = 0) {
        if (id_ != 0) Derived::del(id_);   // 実行時ディスパッチではない
        id_ = id;
    }
};
class TextureHandle : public HandleBase<TextureHandle> {   // 自分自身を渡す
    static GLuint gen();
    static void del(GLuint id);
};
```

`Derived::del` はコンパイル時に `TextureHandle::del` へ置き換わるので、デストラクタの
動的型の問題が原理的に発生しません。`del` が `static` なのも、仮想である必要がない
（むしろあってはいけない）ためです。

> **自分自身をまだ定義し終わっていないのに基底へ渡せる理由**
> `class TextureHandle : public HandleBase<TextureHandle>` を書いている時点で
> `TextureHandle` は不完全型です。それでも通るのは、クラステンプレートの実体化で必要なのが
> **レイアウト**（= メンバ変数 `GLuint id_` だけ）であり、これが `Derived` に依存しないからです。
> `Derived::del` を含む `reset()` の**本体**が実体化されるのは実際に呼ばれる場所で、
> そのときには `TextureHandle` は完全型になっています。
> この「本体の実体化が遅延される」性質が CRTP を成立させています。
> 裏を返すと、基底のメンバ変数の型に `Derived` を使うことはできません（レイアウトが決まらないため）。

**`virtual` と比べて得られたもの**

| | `virtual` | CRTP |
| --- | --- | --- |
| デストラクタから呼べる | **呼べない** | 呼べる |
| オブジェクトのサイズ | vptr のぶん増える | 増えない（`sizeof` は 4、生の `GLuint` と同じ） |
| インライン化 | されない | される（`create()` は `glGenBuffers` 1個に潰れる） |
| 共通の基底型 | ある | **ない** |

サイズが増えないので、生の `unsigned int` から置き換えてもメモリ上のコストはゼロです。

最後の行が唯一の代償です。`HandleBase<TextureHandle>` と `HandleBase<BufferHandle>` は
名前が似ているだけの**無関係な別の型**なので、`std::vector<HandleBase*>` のように
異種のハンドルをまとめて持つことはできません。今回は各ハンドルを `Scene` のメンバとして
持つだけなので代償を払っていませんが、**異種のオブジェクトを共通のポインタで扱いたい
場面では `virtual` が正解**になります。

### 生成だけ明示的なのはなぜか

コンストラクタで `glGen*` せず `create()` を明示的に呼ぶ形にしています。
自動化して嬉しいのは解放のほうだけで、**「どこで GL オブジェクトが生まれるか」は
初期化コードの上に見えていたほうが読みやすい**ためです。

### 外部で作られた ID を受け取る場合

`TextureCache::loadCubemap()` のように生の ID を返す既存の関数と繋ぐときは
`reset()` で所有権を渡します。

```cpp
cubemapTexture_.reset(cache_.loadCubemap(faces, false, ColorSpace::SRGB));
```

### 移行時に踏んだ点

三項演算子で2つのハンドルを選ぶ書き方はコンパイルできません。
ハンドルはコピー禁止なので、`cond ? handleA : handleB` は結果をコピーで作れないためです。
`get()` で `GLuint` を取り出してから選びます。

```cpp
// NG: cond ? brightColorBuffer_ : pingpongColorbuffers_[i]
glBindTexture(GL_TEXTURE_2D, cond ? brightColorBuffer_.get() : pingpongColorbuffers_[i].get());
```

なお、この移行で**既存のリークは1件も見つかりませんでした**（37個すべて解放済みだった）。
入れ替えの目的は今あるバグを直すことではなく、これから増えるぶんを人手で守らなくて済むようにすることです。

### ハンドル型を新しく追加するときの罠

**症状:** 新しいハンドル型を足したら `glBind*` が効かない、GL エラーだけが出る。

**原因:** 既存クラスをコピーしたときに CRTP の型引数を直し忘れている。

```cpp
class RenderbufferHandle : public HandleBase<FramebufferHandle> {  // ← 直し忘れ
    static GLuint gen() { /* glGenRenderbuffers */ }               // 呼ばれない
    static void del(GLuint id) { /* glDeleteRenderbuffers */ }     // 呼ばれない
};
```

**なぜそうなるか:** `HandleBase<FramebufferHandle>` は完全に妥当な型なので**コンパイルが通ります**。
`create()` は `FramebufferHandle::gen()`（= `glGenFramebuffers`）を呼び、自分で定義した
`gen` / `del` は静かに無視されます。結果、RBO のつもりで FBO の名前を持つことになります。

**対処:** `HandleBase` のメンバ関数の本体に静的チェックを置く。

```cpp
static_assert(std::is_base_of_v<HandleBase<Derived>, Derived>, "CRTP の型引数が自分自身になっていない");
```

クラス直下に書いてはいけません。そこは `Derived` がまだ不完全型なので
`is_base_of` が使えないためです（上の「本体の実体化が遅延される」の裏返し）。
`create()` か `reset()` の中に置きます。

---

**症状:** そのテクスチャだけ真っ黒、あるいは FBO へ描いたつもりのものが画面に直接出る。GL エラーは出ない。

**原因:** `create()` の呼び忘れ。

**なぜそうなるか:** `id_` はゼロ初期化なので `glBindTexture(GL_TEXTURE_2D, 0)` が呼ばれます。
これは GL 的には「バインド解除」という**正当な操作**なので、`glGetError()` にも
FBO の完全性チェックにも一切引っかかりません。
以前の生の `unsigned int` は未初期化だと不正な ID になって `GL_INVALID_OPERATION` が
出る可能性がありましたが、ハンドルは必ず 0 から始まるぶん、
**呼び忘れたときにより静かになる**方向へ変わっています。

**対処:** `init*()` にリソースを足すときは `create()` と `glBind*` を必ずセットで書く。
既存のコードは例外なくこの並びになっているので、`glBind*` の直前に `create()` が
見当たらなければ疑う。

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

| 方法             | やり方                                                  |
| ---------------- | ------------------------------------------------------- |
| OpenGL に任せる  | `Window.cpp` で `glEnable(GL_FRAMEBUFFER_SRGB);` を呼ぶ |
| シェーダーで手動 | 最終出力の直前で `pow(color, vec3(1.0 / 2.2))`          |

**このプロジェクトは後者（`hdr.frag` での手動補正）を採用しています。**
HDR + トーンマッピングを実装しており、トーンマッピングとガンマ補正を同じシェーダー内で
連続して行うほうが処理の流れを追いやすいためです。

### 二重にかけると画面全体が白っぽくなる（実際に踏んだ）

**症状:** 画面全体で黒が浮き、テクスチャの色が薄く、靄がかかったように眠い絵になる。
エラーは一切出ない。`Bloom` を 0 にしても `ambient` を 0 にしても `exposure` を下げても消えない。

**原因:** `glEnable(GL_FRAMEBUFFER_SRGB)` と `hdr.frag` の `pow(mapped, 1/2.2)` が両方有効になっていた。

**なぜそうなるか:** ガンマ補正は暗部を大きく持ち上げる操作です。2回かけると実効的に
`L^(1/4.84)` となり、本来ほぼ黒であるべき値が中間グレーまで浮き上がります。

| 元の値 | 1回補正 | 2回補正  |
| ------ | ------- | -------- |
| 0.05   | 0.25    | **0.51** |
| 0.1    | 0.35    | **0.61** |
| 0.2    | 0.48    | **0.71** |
| 0.5    | 0.73    | **0.87** |

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

### アルベドがリニア空間になっていなかった（実際に踏んだ）

上の「二重補正」は出力側の話ですが、**入力側にも同じ問題があり、こちらは長く見逃していました。**

**症状:** 全体的に明るく、色が薄い。ただし出力側の二重補正と違って
`ambient` と `exposure` を下げれば「それらしい絵」にはなるので、
**バグだと気づかないまま基準を作ってしまう。**
決定的な症状が出るのは PBR を入れたときで、`roughness` をどう振っても
金属らしさも粗さも出ない、という形で表面化します。

**原因:** `TextureCache::get(path, bool)` の bool は **`flip`（上下反転）であって
gamma ではありません。** ところが呼び出し側の `cache_.get(path, true)` を見て
「ガンマ指定が `true` になっているからリニア化されている」と思い込んでいました。
実際には `Texture.cpp` が `glTexImage2D(..., GL_RGB, ...)` で読んでいるため、
**sRGB でエンコードされた画素値がそのままリニア値として計算に入っていました。**

**なぜそうなるか:** 画像ファイルの画素値は sRGB でエンコードされています
（人間の目に合わせて暗部に多くのビットを割いた非線形な曲線）。
ライティング計算は光の足し算・掛け算なので、リニアな値でなければ物理的に正しくなりません。
復号せずに使うと、中間調が実際より明るい値として扱われます。

| 画像の値 | 正しいリニア値 | 復号しない場合 |
| -------- | -------------- | -------------- |
| 0.2      | 0.033          | **0.2**        |
| 0.5      | 0.214          | **0.5**        |
| 0.8      | 0.604          | **0.8**        |

Blinn-Phong は経験則の寄せ集めなので「全体的に明るい」で済み、`ambient` や `exposure` で
辻褄を合わせられます。しかし **Cook-Torrance はアルベドがリニアであることを前提に
エネルギー保存を計算する**ので、金属/非金属の分岐もフレネルの効き方も狂います。

**対処:** 色として使うテクスチャだけ、内部フォーマットに `GL_SRGB8` / `GL_SRGB8_ALPHA8`
を指定します。サンプリング時に GPU が復号するので、シェーダー側は何も書かずに済みます。

```cpp
// 「色」か「データ」かを呼び出し側に必ず選ばせる
cubeTexture_   = cache_.get("resources/textures/bricks2.jpg",        true, ColorSpace::SRGB);
cubeNormalMap_ = cache_.get("resources/textures/bricks2_normal.jpg", true, ColorSpace::Linear);
```

> **法線マップ・視差マップ・roughness を `SRGB` にしてはいけません。**
> これらは色ではなく「値」なので、復号するとベクトルの成分や係数が非線形に歪みます。
> エラーは出ず「なんとなく陰影が変」にしかならないため、発見が非常に遅れます。

**アルファは復号の対象外**で常にリニアのまま扱われる規定です。
`window.png` の「アルファ 0.5 を閾値に窓枠とガラスを分ける」判定は、
`GL_SRGB8_ALPHA8` にしてもそのまま成立します。

**この修正を入れると画面全体が暗くなりますが、それが正しい状態です。**
今まで明るすぎただけなので、`Ambient` と `Exposure` のスライダーで基準を作り直してください。
「暗くなった＝失敗」と判断して元に戻さないこと。

**bool を並べたインターフェースが事故の原因だったので、色空間は `enum class ColorSpace`
にして呼び出し側に明示させる形にしました。** `TextureCache` のキーにも色空間を含めています。
パスだけをキーにすると、同じ画像を色とデータの両方で読んだときに
後から要求したほうが先に読まれた側の内部フォーマットを黙って受け取ってしまうためです。

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

| 章                              | 環境光の扱い                                       |
| ------------------------------- | -------------------------------------------------- |
| Basic Lighting                  | シーン全体の定数（`float ambientStrength = 0.1;`） |
| Light Casters / Multiple Lights | ライトごとのメンバ + `attenuation`                 |
| **SSAO**                        | **シーン全体の定数に戻る。`attenuation` なし**     |

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

| 項目               | カラーテクスチャ       | デプステクスチャ                 |
| ------------------ | ---------------------- | -------------------------------- |
| 内部フォーマット   | `GL_RGB` / `GL_RGBA`   | `GL_DEPTH_COMPONENT`             |
| データ型           | `GL_UNSIGNED_BYTE`     | `GL_FLOAT`                       |
| FBO アタッチメント | `GL_COLOR_ATTACHMENT0` | `GL_DEPTH_ATTACHMENT`            |
| カラー出力         | あり                   | `glDrawBuffer(GL_NONE)` で無効化 |
| カラー読み込み     | あり                   | `glReadBuffer(GL_NONE)` で無効化 |

### デプスマップ FBO の初期化手順

1. `gl::FramebufferHandle depthMapFBO_` と `gl::TextureHandle depthMap_` を Scene.h に追加
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

| アタッチメント    | 変数           | 内部フォーマット | 中身                                 |
| ----------------- | -------------- | ---------------- | ------------------------------------ |
| COLOR_ATTACHMENT0 | `gPosition_`   | `GL_RGBA16F`     | ワールド座標                         |
| COLOR_ATTACHMENT1 | `gNormal_`     | `GL_RGBA16F`     | rgb=ワールド法線（ノーマルマップ適用後）, a=metallic |
| COLOR_ATTACHMENT2 | `gAlbedoRoughness_` | `GL_RGBA8`  | rgb=アルベド, a=roughness            |

**位置と法線に浮動小数点フォーマット（16F）が必須な理由**は、どちらも `[0,1]` に収まらない値だからです。
座標は 20 のような大きな値を取り、法線は `-1` のような負の値を取ります。
`GL_RGBA8` は `[0,1]` に丸められる固定小数点なので、ここに入れると情報が壊れます。

一方アルベドと roughness はどちらも `[0,1]` なので `GL_RGBA8` で十分です。

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

| location | 意味                                                              |
| -------- | ----------------------------------------------------------------- |
| 0        | position                                                          |
| 1        | normal                                                            |
| 2        | uv                                                                |
| 3        | tangent                                                           |
| 4        | bitangent                                                         |
| 5        | インスタンスごとの位置オフセット（`glVertexAttribDivisor(5, 1)`） |

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
gl::VertexArrayHandle myObjectVAO_;
gl::BufferHandle myObjectVBO_, myObjectEBO_;
```

生の `unsigned int` ではなく `gl::*Handle` を使います。詳細は
[GL リソースの持ち方](#gl-リソースの持ち方) を参照。

### 3. initMesh() でバインド設定

```cpp
int stride = sizeof(gl::Vertex);
myObjectVAO_.create();
myObjectVBO_.create();
myObjectEBO_.create();
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

### 5. 解放は書かない

`gl::*Handle` のデストラクタが自動で解放するので、**`glDelete*` を書く必要はありません**
（`Scene` にはデストラクタ自体がありません）。

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
gl::BufferHandle myObjectInstanceVBO_;
```

### 3. initMesh() でインスタンスVBOを設定（VAO バインド中に行うこと）

```cpp
myObjectInstanceVBO_.create();
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
myTexture_ = cache_.get("resources\\textures\\filename.png", true, ColorSpace::SRGB);
// 第2引数 flip:       true = 上下反転あり（通常はtrue）、アルファ付きPNGも自動判別
// 第3引数 colorSpace: SRGB = アルベドなど「色」 / Linear = 法線マップなど「値」

// Render()
myTexture_->bind(0);  // テクスチャユニット0にバインド
```

**第3引数の選択を間違えるとエラーが出ないまま陰影だけが狂います。**
デフォルト値をあえて持たせていないので、追加のたびに「色かデータか」を判断してください。
理由と判断基準は [ガンマ補正](#ガンマ補正) の「アルベドがリニア空間になっていなかった」を参照。

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
// false = 反転なし、SRGB = 背景として「色」に使うので復号する
cubemapTexture_ = cache_.loadCubemap(faces, false, ColorSpace::SRGB);
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

| 値        | 表示内容                               |
| --------- | -------------------------------------- |
| 0         | 通常のライティング                     |
| 1         | ライト0のシャドウ判定だけ              |
| 2         | `shadowMap[0]` の生の深度値            |
| 3 / 4 / 5 | G-Buffer の Albedo / Normal / Position |
| 6         | 画面4分割で上記を一度に表示            |

同様に `gbuffer_floor.frag` には `GBUFFER_WRITE_TEST` があり、
1 にすると床を描くときに G-Buffer へ**位置や法線と無関係な固定色**を書き込みます。
これで「書き込んだ値が悪いのか、書き込み自体が届いていないのか」を切り分けられます。

### 原則2: デバッグ表示のときはトーンマッピングを必ず切る

**ここは非常に引っかかりやすいポイントです。**

`hdr.frag` は `mapped = 1 - exp(-color * exposure)` の後に `pow(mapped, 1/2.2)` をかけます。
exposure が 3.0 のとき、値の見え方はこうなります。

| 元の値 | 画面上             |
| ------ | ------------------ |
| 0.1    | 0.55（中間グレー） |
| 0.5    | 0.89（ほぼ白）     |
| 1.0    | 0.98（白）         |

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

**症状:** G-Buffer の `gPosition` と `gNormal` だけが常にクリア値のまま。`gAlbedoRoughness` は正常。
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
layout (location = 2) out vec4 gAlbedoRoughness; // vec4 → アルファを明示的に書いている
```

**`vec3` で宣言した出力のアルファ成分は未定義**（実際には 0）です。したがって前2枚は
`src.rgb * 0 + dst.rgb * 1` となり、**書き込みが毎フレーム完全に消えます。**
`gAlbedoRoughness` だけ生き残るのでアルベドは正常に見え、原因が非常に見つけにくくなります。

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

### GLSL の予約識別子を変数名に使わない

**症状:** ソースを何も変えていないのに、開発マシンを替えた途端に特定のシェーダーだけコンパイルが通らなくなる。
エラーは変数を宣言した行と、その変数を使った行に固まって出る。

```
0(40) : error C0000: syntax error, unexpected '=', expecting ';' or '(' at token "="
0(41) : error C0000: syntax error, unexpected '.', expecting "::" at token "."
```

**原因:** 変数名が GLSL の予約識別子と衝突している。実際に踏んだのは `gbuffer_model.frag` の `vec3 packed` で、
`packed` は `layout(packed)` などで使う**レイアウト修飾子の識別子**（`shared` / `std140` の仲間）です。

**なぜそうなるか:** 予約された名前をどこまで厳しく弾くかは**ドライバの実装依存**です。
緩いドライバは変数名として通してしまうため、そのマシンでは問題なく動き続けます。
厳しいドライバに移った瞬間に宣言が変数宣言として解釈されなくなり、

- 宣言行: 名前として受理されないので、続く `=` が「予期しないトークン」になる
- 使用行: その名前が型名・名前空間名として扱われるため、`.g` に対して `::` を期待するエラーになる

という 2 種類のエラーが連鎖します。`expecting "::"` が出たら、まず**その識別子が予約語ではないか**を疑ってください。
文法自体はどこも間違っていないので、行を睨んでも原因は見つかりません。

**対処:** 変数名を衝突しないものに変える（`packed` → `metallicRoughnessSample`）。
`shared` / `packed` / `filter` / `sample` / `input` / `output` / `common` / `active` / `resource` あたりは、
自然に変数名として書きたくなるうえに予約されているので特に危険です。

**この症状の見え方:** リンクに失敗したプログラムは描画に使えないので、
**そのシェーダーで描いているオブジェクトだけ**が壊れます。今回はモデルの Geometry パスだったため、
床や壁（別ファイル）は正常なままモデルだけが全部おかしくなりました。
「一部のオブジェクトだけ壊れている」ときは、起動時のシェーダーログを最初から読み直すのが最短です。

### clang-format の include 自動ソートが glad と GLFW を入れ替える

**症状:** ソースのロジックを一切変えていないのに、一括整形をかけた直後からビルドが通らなくなる。
`glad.h` が「OpenGL ヘッダが既に include されている」と言って `#error` を吐く。

**原因:** clang-format の `SortIncludes` が既定で有効で、同じブロック内の include を
アルファベット順に並べ替えます。ASCII では大文字が小文字より先に来るため、

```
#include <glad/glad.h>      →      #include <GLFW/glfw3.h>
#include <GLFW/glfw3.h>             #include <glad/glad.h>
```

と**必ず GLFW が先**に来てしまいます。`GLFW_INCLUDE_NONE` を定義していない `glfw3.h` は
`GL/gl.h` を自前で include するので、後から読まれた `glad.h` が衝突を検出して止まります。

**なぜ気付きにくいか:** 整形は「空白しか変わらないはず」という先入観があるため、
ビルドエラーが出ても整形と結び付けにくい。`git diff` も全ファイル真っ赤になっているので、
2行の入れ替えが埋もれて見えません。`git diff -w`（空白無視）を取ると、
残った差分＝空白以外の変更だけになるので、include の移動がすぐ見つかります。

**対処:** `.clang-format` で `SortIncludes: Never` を指定しています。
include の順序に意味があるのは glad だけではなく（`stb_image.h` の実装マクロなども同様）、
このプロジェクトでは自動ソートそのものを切る判断にしました。

順序を保ったままソートしたい場合は `IncludeCategories` で glad を `Priority: 1` に置く方法もありますが、
カテゴリを1つでも定義すると既定のカテゴリが丸ごと置き換わるため、全種類を書き切る必要があります。

**確認方法:** 整形後は必ず以下で glad が GLFW より前にあることを確かめてください。

```
grep -n "glad/glad.h\|GLFW/glfw3.h" src/*.h src/*.cpp
```

---

## カメラ

### クォータニオンで視点を回すとロールが溜まる

**症状:** マウスで視点を回しているだけなのに、だんだん水平線が傾いてくる。
特に「右に回す → 上を向く → 左に回す」のように往復させると顕著に出る。

**原因:** ヨーとピッチの両方を**カメラ自身のローカル軸**まわりに掛けているため。

回転は交換法則が成り立ちません。ローカルのヨーとローカルのピッチを交互に掛けると、
その積は「ヨー × ピッチ」ではなく**ロール成分を含んだ別の回転**になります。1回あたりは微小でも、
マウスを動かすたびに掛け続けるので誤差ではなく累積として溜まっていきます。

**対処:** **ヨーはワールドの上方向まわり、ピッチはローカルの右方向まわり**に掛ける。
掛ける順序（左からか右からか）で軸が変わります。

```cpp
// ワールド軸の回転は左から、ローカル軸の回転は右から掛ける
Orientation = glm::angleAxis(yaw, WorldUp) * Orientation * glm::angleAxis(pitch, xAxis);
```

`Camera::UpdateCameraVectors()` が `Right` を `cross(Front, WorldUp)` で作り直しているのも同じ理由で、
姿勢側にロールが残っていてもビュー行列には漏れないようにしています。

あわせて、掛け続けると数値誤差でクォータニオンの長さが 1 からずれるので毎回正規化します。

### ピッチの上限は「回転」ではなく「増分」に掛ける

真上・真下を越えると視界が反転します。オイラー角なら角度そのものを clamp すれば済みますが、
クォータニオンは角度を直接持っていません。

現在のピッチを `asin(Front.y)` で逆算し、**入力の増分の側を切り詰めて**から掛けます。

```cpp
pitch = glm::clamp(current + pitch, -limit, limit) - current;
```

姿勢を作った後から補正しようとすると、`Front` が真上を向いた瞬間に
`cross(Front, WorldUp)` が零ベクトルになって `Right` が NaN になります。

### 追従の補間はフレームレートに依存させない

**症状:** 追従カメラの追いつく速さが、FPS によって変わる。重いフレームで行き過ぎる。

**原因:** `mix(current, desired, 0.1f)` のように**毎フレーム固定の割合**で詰めていること。
これは「1フレームあたり」の速さなので、フレーム数が変われば結果も変わります。

**対処:** 経過時間から指数で割合を求める。

```cpp
const float blend = 1.0f - std::exp(-stiffness * deltaTime);
```

こうすると `deltaTime` が倍になれば残りも二乗で減るので、時間あたりの追従の速さが一定になります。
`deltaTime` がどれだけ大きくても `blend` は 1 を超えないため、行き過ぎません。

---

## 衝突判定

キャラクターは**円柱**（XZ 平面の円 + Y 方向の高さ）、障害物は **AABB**（軸に平行な直方体）で持ち、
`CollisionWorld::Resolve()`（`Collision.cpp`）が「めり込んでいたら押し戻す」方式で解決しています。

### 判定と押し出しで違う半径を使うと壁際で震える

**症状:** 壁や箱に近づくとキャラクターが小刻みにカタカタ震える。エラーは一切出ない。

**原因:** `pushOutXZ()` の中で `radius` を使う箇所が3つあり、そのうち一部だけをスキン幅ぶん太らせている。

**なぜそうなるか:** 判定と押し戻し量が食い違うと、以下が毎フレーム繰り返されます。

1. 太った半径で「重なっている」と判定される
2. 細い半径ぶんしか押し戻さない
3. 次のフレームでもまだ太った判定の内側にいる → また押される

押し戻しても判定から抜けられないので、振動が収まりません。

**対処:** 関数の入口で実効半径を1つ作り、以降は必ずそれだけを使う。

```cpp
// 注意: 判定と押し出し量で違う半径を使うと壁際で震える
const float skinRadius = radius + kSkinWidth;
```

`radius` が出てくるのは以下の3箇所で、**1つでも取り残すと再発します。**

| 箇所 | 式 |
| --- | --- |
| 重なり判定 | `distanceSq > skinRadius * skinRadius` |
| 外側ケースの押し出し量 | `skinRadius - distance` |
| 内側ケースの押し出し量 | `toMinX + skinRadius` など4方向すべて |

なおスキン幅そのものは「判定を見た目より少し太らせ、接触する手前で止める」ための余白です。
人型メッシュは肩や腕が円柱から張り出すので、円柱がぴったり壁に接した時点では
**見た目上すでに肩がめり込んでいます。** 数センチ隙間が空くほうが自然に見えます。

### 中心が矩形の内側に入ると押し出す向きが決まらない

**症状:** キャラクターが画面から消える、または座標が `NaN` になる。

**原因:** 押し出し方向を「矩形上の最近接点 → 円の中心」のベクトルから作っているが、
中心が矩形の**内側**にあるとそのベクトルがゼロになる。

**なぜそうなるか:** 最近接点は `std::clamp(center.x, box.min.x, box.max.x)` で求めています。
`center.x` が既に矩形の範囲内にあると `clamp` は何もせず `center.x` をそのまま返すため、
X も Z も内側なら最近接点＝中心となり `offset` がゼロベクトルになります。
これを長さで割って正規化すると 0 除算で `NaN` が出ます。長さゼロのベクトルに向きは定義できません。

**対処:** `pushOutXZ()` が2つの分岐に分かれているのはこのためです。

- `distanceSq > kCenterEpsilonSq`（中心が外側）→ 最近接点からの方向へ押す
- それ以外（中心が内側）→ 4辺までの距離を比べ、**最も近い辺**を突き破る向きへ押す

閾値をちょうど 0 ではなく `1e-8f` にしているのは、境界上で計算誤差により
ぴったり 0 にならないケースを「実質ゼロ」として拾うためです。

### 押し出しだけでスライドは成立する

**仕様上は正しいが直感に反する部分です。**

「壁に沿って滑る」は普通「移動量から壁の法線成分を引く」と実装しますが、
**軸に平行な平らな壁に対しては、自由に動かしてから押し戻すだけで数学的に同じ結果になります。**

```cpp
position_ += direction * CharacterDefaults::MOVE_SPEED * deltaTime;
// 動かしてから押し戻す 面に沿った成分は残るので壁沿いに滑る
position_ = world.Resolve(position_, CharacterDefaults::RADIUS, height_);
```

押し出しは壁に**垂直な成分だけ**を打ち消すので、壁沿いの成分はそのまま残ります。
明示的に法線成分を除去する実装が要るのは、押し出しでは足りない場面
（高速移動による貫通、複雑な形状の角）に限られます。

### 向きは実移動方向ではなく入力方向のまま保つ（設計判断）

**将来変更すると問題になる前提条件です。**

`Character::Move()` は `Resolve()` で位置を補正した**後**に、補正前の `direction`（入力方向）で
`turnTowards()` を呼んでいます。「実際に動いた方向を向くべきでは」と直したくなりますが、
**意図的にこうしています。**

壁に斜めに当たったときの違いは以下の通りで、実際に両方を実装して比較した上での判断です。

| 向きの決め方 | 見え方 |
| --- | --- |
| 入力方向（採用） | 壁に体を押し付けたまま横へ滑る |
| 実際に動いた方向 | 壁沿いに向き直ってしまい、入力と体の向きがずれる |

`Resolve()` は位置だけを返し、接触面の法線を返さない設計になっているのも同じ理由です。
向きの計算に衝突結果を使わないので、`CollisionWorld` のインターフェースを太らせずに済んでいます。

### 反復回数は角のために要る

`kResolveIterations = 4` は `Resolve()` が押し出しを繰り返す上限です。

1つの箱から押し出した結果、**隣の箱に新しくめり込む**ことがあるため、
「全障害物を1周して誰にも押されなくなる」まで繰り返します。収束したら `break` で早期に抜けます。
壁が平面2枚だけなら1回で済みますが、立方体を障害物に含めると角のケースが実際に発生します。

---

## モデル読み込みと単位系

### ワールド座標は 1.0 = 1 メートル（プロジェクト規約）

寸法を決める定数は `src/SceneUnits.h` の `gl::units` に集約しています。glTF が既定でメートル・Y-up なので、
読み込んだモデルを無変換で置けるよう合わせたものです。

床の広さ・壁の高さ・テクスチャの貼り密度をコード中に直接書かず、必ずここの定数から導いてください。
特にテクスチャの繰り返し回数は「繰り返し数」ではなく **「タイル1枚あたりの実寸」** で持たせています。
繰り返し数で持つと、床を広げた瞬間にテクセル密度が変わってしまうためです。

### 単位を変えると明るさが変わる

**症状:** オブジェクトの配置やスケールを実寸に直しただけなのに、シーン全体が暗く（または明るく）なる。

**原因:** ライティングが逆二乗減衰なので、光源と面の距離が変われば明るさがそのまま変わるため。

距離を2倍にすれば明るさは 1/4 になります。実装をミスったように見えますが正常な挙動です。

**対処:** 単位を変えたら `hdr.frag` の露出と `PointLight::diffuse` を再調整する。
`PointLight::calcRadius()` の返す影響半径も距離に依存するので、あわせて確認してください。

### 頂点属性 location 5 は空けておく（プロジェクト規約）

頂点属性の割り当ては以下で固定しています。

| location | 内容 |
| -------- | ---- |
| 0 | position |
| 1 | normal |
| 2 | uv |
| 3 | tangent |
| 4 | bitangent |
| 5 | **インスタンスごとの位置（`aOffset`）専用。他の用途で使わない** |
| 6 | ボーンID（`ivec4`） |
| 7 | ボーンの重み（`vec4`） |

`point_shadow_depth.vert` は location 5 を `aOffset` として読み、
**「有効化していない VAO では既定値 (0,0,0) が読まれるので実質無効化される」** という挙動に依存しています。
そのおかげで、インスタンス描画するキューブと、インスタンス描画しないモデルを同じ深度シェーダーで描けます。

ここにボーンIDを割り当ててしまうと、モデルをシャドウパスで描いたときに
**整数のボーンIDが座標オフセットとして解釈され、メッシュがばらばらに飛び散ります。**
コンパイルエラーにも GL エラーにもなりません。ボーン属性を 6 / 7 から始めているのはこのためです。

### ノード階層を捨てたローダにはスキニングが乗らない

**症状:** 静止メッシュは正しく出るのに、ボーンを入れようとすると設計ごと作り直しになる。

**原因:** `aiNode::mTransformation` を無視してメッシュを平坦に積むと、ノードの親子関係が失われるため。

スキニングは「ルートから対象ノードまでの変換を掛け合わせる」処理そのものなので、階層が無いと計算できません。
`Model` が `ModelNode` の木をそのまま保持しているのはこのためです。

たちが悪いのは、**メッシュが1個のモデル（DamagedHelmet など）では階層を捨てても正しく表示される**ことです。
その状態で「動いた」と判断すると、キャラクターを入れた時点で作り直しになります。

### aiMatrix4x4 は行優先。転置しないと壊れる

Assimp の行列は行優先、glm は列優先です。`memcpy` や単純な代入で詰め替えると、
**転置された行列が入り、モデルが妙な向き・位置・スケールで出ます。** `Model.cpp` の `toGlm()` で転置しています。

同じ理由で、スキニングを実装するときは**ルートノードの変換の逆行列**（`Model::GlobalInverseTransform()`）を
掛け忘れないでください。掛け忘れるとモデル全体が変な位置とスケールになります。

### glb の埋め込みテクスチャは `mWidth` がバイト数

`.glb` はテクスチャの実体をファイルとして持たず、`aiScene::mTextures` に埋め込みます。
マテリアルから返るパスは `*0` のような**参照文字列**なので、
ディレクトリと連結してファイルを開こうとしても必ず失敗します。`aiScene::GetEmbeddedTexture()` で実体を引いてください。

引いた `aiTexture` の読み方に罠があります。

- `mHeight == 0` のとき、中身は **PNG / JPEG のまま**（圧縮済み）で、`mWidth` は**ピクセル数ではなくバイト数**
- `mHeight != 0` のときだけ生のピクセル配列

つまり通常の `.glb` では `stbi_load_from_memory` に `mWidth` をバイト数として渡すのが正しい呼び方です。

またテクスチャキャッシュはパス文字列をキーにしているので、埋め込みテクスチャには
「モデルのパス + 参照文字列」で一意なキーを作って渡しています。

### glTF は metallic と roughness を1枚にパックしている

`metallicRoughnessTexture` は **G チャンネルが roughness、B チャンネルが metallic** です。
別々のテクスチャとして 2 回読み込むと同じ画像が二重に GPU へ載るので、1枚として持ってシェーダー側で分けます。

**症状:** モデルが不自然にてかる、あるいは全体が金属のように見える。

**原因:** metallicRoughness テクスチャを取得できておらず、`metallicFactor` / `roughnessFactor` だけが効いている。
glTF のこれらの既定値はどちらも **1.0**（＝完全な金属・完全にラフ）なので、
取得に失敗すると「全部メタル」になります。

Assimp はこのテクスチャを版によって `aiTextureType_GLTF_METALLIC_ROUGHNESS` / `METALNESS` /
`DIFFUSE_ROUGHNESS` のどれで返すかが違うため、`Model::loadMaterial()` では順にフォールバックしています。

### 軸の向きが違うモデルは倒れて読み込まれる

**症状:** COLLADA（`.dae`）由来など、ルートノードに軸変換（例: `Z_UP`）を持つ glTF/glb モデルを読み込むと、横に倒れた状態で表示される。

**対策:** `Scene::modelSpawns_` の `ModelSpawn::rotationDegrees` に、そのモデル用の補正回転を設定する。ボーン行列やローダー側は変更しない — 軸の向きは配置の問題であり、スキニングの計算とは別レイヤーで対処する。

### 新しい3Dモデルを追加する（実用手順）

1. **モデルファイルを `resources/` 以下に置く。** 推奨は `.glb`（glTF バイナリ、
   テクスチャも1ファイルに埋め込まれる）。キャラクターなら `resources/characters/`、
   静物なら `resources/publishable-objects/` に既存ファイルがあるので同じ並びに置く
2. **`Scene.h` の `modelSpawns_` に1行足す。これだけでシーンに出る。**
   `Model`・`Scene.cpp`・シェーダーは何も変更しなくてよい（[新しい3Dオブジェクトの追加](#新しい3dオブジェクトの追加)
   や [新しいシェーダーの追加](#新しいシェーダーの追加) と違い、頂点データやVAOを自分で書く必要が無い。
   Assimp 経由で読み込む形式は完全にデータ駆動になっている）

   ```cpp
   // Scene.h
   const std::vector<ModelSpawn> modelSpawns_ = {
       // path, position, rotationDegrees, scale, followTarget(省略可、trueなら1体だけ)
       {"resources/characters/MyModel.glb", glm::vec3(0.0f, gl::units::floorY, 0.0f), glm::vec3(0.0f), 1.0f},
   };
   ```

3. **ビルドして起動するだけで表示される。** `Scene::initModels()`（`Scene.cpp`）が
   `modelSpawns_` を1行ずつループして `Model` を構築する

**うまく表示されないときに確認すること**

| 症状 | 確認箇所 |
| --- | --- |
| コンソールに `Skipped model: ...` と出て表示されない | パスが間違っている、または Assimp が対応していない形式。読み込み失敗は `try/catch` で握りつぶして読み飛ばすだけで、**起動は止まらない**（`Scene.cpp` の `initModels()`） |
| モデルが横に倒れる・変な向きに立つ | `rotationDegrees` を設定する。[軸の向きが違うモデルは倒れて読み込まれる](#軸の向きが違うモデルは倒れて読み込まれる) 参照 |
| モデルが極端に大きい／小さい | `scale` を調整する。このプロジェクトは 1.0 = 1メートル規約（[モデル読み込みと単位系](#モデル読み込みと単位系)）なので、glTF 側が正しくメートルでエクスポートされていればほぼ 1.0 のままで合う |
| ボーンが動かずバインドポーズのまま固まる | モデル自体にスキニング・アニメーションが入っていない。次項参照 |

**キャラクターとして操作したい場合**

`followTarget: true` を追加する。三人称カメラの注視対象になり、`Character`（`Scene::character_`）が
そのモデルの spawn 位置・高さで生成される（`initModels()` 内、`spawn.followTarget` の分岐）。
**`true` は1体だけを想定している。** 2体以上に付けると `playerModelIndex_` が後勝ちで上書きされ、
カメラと `Character` は配列内で最後に `true` を付けたモデルだけを追従する。

**読み込み時の Assimp フラグ（`Model::LoadModel()` / `Model.cpp`）**

```cpp
importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs |
    aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_LimitBoneWeights);
```

四角面や法線・タンジェントが無いモデルでもこのフラグで自動生成されるので、通常は
モデル側を事前加工する必要はない。`aiProcess_LimitBoneWeights` により1頂点あたりの
ボーン影響は自動的に4本へ切り詰められる（`gl::Vertex::m_BoneIDs` の枠と一致させるため）。

---

### 3Dモデルのアニメーションを再生する（実用手順）

**手順は無い。モデルにアニメーションが埋め込まれていれば、追加のコードなしで自動的に再生される。**

上の手順でモデルを `modelSpawns_` に足しただけで、そのモデルにスキニング・アニメーション
（glTF の `animations` セクション）が入っていれば、`Model` コンストラクタ内で自動的に
アニメーションが読み込まれ、**1本目のクリップがロードした瞬間から再生され続ける**（ループ再生）。
`Character` や `Scene` 側で「このモデルのアニメーションを再生開始する」ような呼び出しを
どこかに書く必要は無い。

- **止める／再開する:** `Scene::Render()` が毎フレーム `models_[i]->UpdateAnimation(freeze ? 0.0f : deltaTime)`
  を呼んでいる（`Scene.cpp`）。`deltaTime` に `0.0f` を渡すだけで、その瞬間の姿勢のまま止まる。
  ポーズ機能（UI の一時停止）はこれを使っている
- **アニメーションが無いモデル:** バインドポーズ（`T` ポーズ等）のまま静止して描画される。
  エラーにはならない（`Model::HasAnimation()` が `false` を返す経路にそのまま乗るだけ）
- **複数のモーション（例: 歩行 + ジャンプ）を切り替えたい場合:** **現状は未対応。**
  `.glb` に何本クリップが入っていても、常に1本目（`animations_[0]`）しか再生されない。
  切り替えの仕組みそのものを実装する必要がある（[#31](https://github.com/KDKyota/glfwdojo/issues/31) 参照）

**内部でどう動いているか（実装の参考）**

以下は「なぜ自動再生されるのか」の実装詳細。手を加える予定が無ければ読み飛ばしてよい。

**読み込み（`Model::loadAnimations()`）**

`aiScene::mAnimations[]` を全部 `Model::animations_`（`std::vector<Animation>`）へ読み込みます。
1本の `Animation` は「ノード名 → そのノードの位置／回転／スケールのキー列」という
`unordered_map` (`channels`) を持ちます（`aiAnimation::mChannels[]` を1本ずつ読み替えたもの。
キー列の構造は `Model.h` の `Animation` / `NodeAnimation` / `AnimationKey` を直接参照してください）。

読み込みの最後で `if (!animations_.empty()) activeAnimation_ = 0;` としているため、
2本目以降のクリップは読み込まれてはいるものの**一切使われません**。

**毎フレームの再生（`Model::UpdateAnimation()`）**

```cpp
void Model::UpdateAnimation(float deltaTime) {
    if (activeAnimation_ < 0 || boneMatrices_.empty()) return;
    const Animation &animation = animations_[activeAnimation_];
    animationTime_ += deltaTime * animation.ticksPerSecond;
    if (animation.duration > 0.0f)
        animationTime_ = std::fmod(animationTime_, animation.duration);
    updateBoneMatrices(root_, glm::mat4(1.0f), animationTime_);
    uploadBoneMatrices();
}
```

- `animationTime_` は「tick」単位で進む（秒ではない）。`ticksPerSecond` を掛けて秒→tickに変換している
- `std::fmod` でループさせている。**単発再生（1回きりで終わるモーション）を作る場合はここを変える必要がある** —
  現状は全アニメーションが無条件にループする作りになっている
- `updateBoneMatrices()` がノード階層を再帰的に辿り、各ノードについて `nodeTransform()` で
  現在時刻のローカル変換を作ってボーン行列 `boneMatrices_` を更新する
- `Scene::Render()` 側では `models_[i]->UpdateAnimation(freeze ? 0.0f : deltaTime)`（`Scene.cpp`）のように、
  ポーズ中は `deltaTime` を渡さないことでアニメーションを止めている

**キーフレームの補間（`Model::nodeTransform()`）**

```cpp
glm::mat4 Model::nodeTransform(const ModelNode &node, float time) const {
    if (activeAnimation_ < 0) return node.localTransform;
    const Animation &animation = animations_[activeAnimation_];
    const auto found = animation.channels.find(node.name);
    if (found == animation.channels.end()) return node.localTransform;
    // 位置・スケールは線形補間、回転は quaternion の slerp
    ...
}
```

- そのノード名に対応するチャンネルが**無ければ** `node.localTransform`（バインドポーズの変換）をそのまま返す。
  アニメーションが動かさないノード（例えばメッシュ本体を吊るす親ノード）はこの経路を通る
- 位置とスケールは前後のキーを線形補間（`sampleVec3`）、回転は `glm::slerp` で球面線形補間（`sampleQuat`）する。
  回転を線形補間ではなく slerp にしているのは、クォータニオンを単純に線形補間すると
  途中で回転速度が不自然に変化する（球面上を通らない）ため
- `ticksPerSecond` が glTF ファイル側で 0 になっていることがある。`Animation::ticksPerSecond` の
  既定値 `25.0f`（`Model.h`）はこの場合のフォールバックで、`loadAnimations()` は
  `source->mTicksPerSecond != 0.0` のときだけ上書きしている

---

## 今後の課題

**このドキュメントは TODO を持ちません。** 未実装の項目・改善の余地は
[`plan.md`](./plan.md) の「長期の積み残し」へ集約しています。

ここに書くのは確定した知見（設計の理由・作業手順・踏んだ罠）だけにしてください。
やることリストを併記すると、README・plan.md と三重管理になって必ずどれかが古くなります。
