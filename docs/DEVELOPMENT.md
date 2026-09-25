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
| SSAO が光源のすぐ近くでしか見えない | [環境光を光源ごとに減衰させている](#環境光を光源ごとに減衰させると-ao-が光源の近くでしか見えない) |
| ライティングが真っ黒。法線が全て `(0,0,0)` | [`normalMatrix` の設定漏れ](#normalmatrix-を各シェーダーの描画前に設定する) |
| 全灯まとめてシャドウを表示するとほぼ全面が白い | [背を向けた面でも 1.0 が返るので正常。1灯ずつ見る](#原則4-シャドウの確認は必ず1灯ずつ) |
| ライトキューブが 2D の板ポリに見える／全オブジェクトの手前に出る | [FBO の外で描画している](#ライトキューブを-fbo-の外で描画してしまう) |
| uniform を設定したのに効かない | [同じ `.frag` でもプログラムごとに設定が必要](#同じ-frag-でも-uniform-はシェーダープログラムごとに設定する) |
| SDF 遮蔽で水平線あたりに横一本の明るい帯が出る | [面と平行に進むレイがステップを使い切っている](#sphere-tracing-は面に漸近するとステップを使い切る) |
| 光源やIBLの遮蔽が距離で滑らかに変化せず、くっきり境目ができる。`SDF_MAX_STEPS` を変えても直らない | [レイマーチの前進量に下限を設けると補間式の前提が崩れる](#レイマーチの最小前進距離はソフトシャドウの補間式と衝突する見送った案) |
| 壁だけ IBL も直接光も一切当たらず完全な黒になる | [SDF の箱の中心が描画される板ポリと重なっている](#sdf-のプロキシ形状は描画メッシュの表面と面を揃える) |
| パスを半解像度にした途端に床へ縦筋が出る | [ノイズの倍率を入力テクスチャの解像度から作っている](#半解像度パスで入力テクスチャの解像度からスケールを作ると壊れる) |
| 壁際の箱を浅い角度で見ると IBL の鏡面反射に同心円状の縞（波紋）が出る。`SDF occlusion` を 0 にすると消える(Issue #67) | [sphere tracing の歩幅が画素ごとに揃わず 位相がずれる](#sphere-tracing-の歩幅は画素ごとに位相がずれ同心円状の縞になるissue-67) |
| 3Dモデルの SDF 遮蔽がモデルの形ではなく直方体になる。`debugMode 14` では正しいのに本描画では箱型 | [距離場のサンプラーを設定し忘れている](#距離場のサンプラーを設定し忘れると-aabb-全体が遮蔽物になるissue-69) |
| 平面反射の中だけ面が虫食いのように消える。本描画は正常(Issue #2) | [`gl_ClipDistance` を書かないシェーダーが混ざっている](#クリップ距離を書かないシェーダーを混ぜると面がランダムに消えるissue-2) |
| 平面反射のテクスチャが床の色一色になり 空も物体も映らない(Issue #2) | [反射面ちょうどの断片は距離 0 でクリップされない](#反射面ちょうどの断片はクリップされず-反射像を覆うissue-2) |

**リソース・テクスチャ**

| 症状 | 原因 |
| --- | --- |
| そのテクスチャだけ真っ黒／FBO へ描いたはずのものが画面に直接出る。GL エラーは出ない | [`create()` の呼び忘れ](#ハンドル型を新しく追加するときの罠) |
| 新しく足したハンドル型で `glBind*` が効かない | [CRTP の型引数の直し忘れ](#ハンドル型を新しく追加するときの罠) |
| 行列の値が入れ違いになる | [UBO のメンバ順序が一致していない](#ubo-のメンバ順序を統一する) |
| UBO で送った座標が 1 要素ずつずれる | [`std140` の配列に `vec3` を使っている](#std140-の配列に-vec3-を使ってはいけない) |

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
| リンクエラー（複数回定義） | [ヘッダの変数定義に `inline` が無い](#ヘッダで変数定義するときは-inline-をつける) |
| 環境を替えたらシェーダーが `syntax error`。`expecting "::"` が出る | [変数名が GLSL の予約識別子と衝突している](#glsl-の予約識別子を変数名に使わない) |
| 一括整形をかけた直後から `glad.h` が「OpenGL ヘッダが既に include されている」と `#error` を出す | [clang-format の include 自動ソートが glad と GLFW を入れ替えている](#clang-format-の-include-自動ソートが-glad-と-glfw-を入れ替える) |

**GPU デバッグ・計測**

| 症状 | 原因 |
| --- | --- |
| RenderDoc から起動すると真っ黒／即落ちする。`preset run` では動く | [Working Directory を指定していない](#renderdoc-で起動するときは-working-directory-を必ず指定する) |
| `Vertex shader ... is being recompiled based on GL state` が出る | [1本のシェーダーで頂点レイアウトの違う VAO を描いている](#未使用の-location-はドライバにシェーダーを作り直させる) |
| `The driver allocated storage for renderbuffer N` でログが埋まる | [NVIDIA は LOW で出すので NOTIFICATION 切りでは落ちない](#ドライバのノイズは-id-で個別に切る) |
| RenderDoc のイベント一覧が名前無しの `Draw()` の羅列になる | [デバッググループを Push していない](#パス名は-gpuprofiler-から出している) |
| ベンチで `std::vector` だけ極端に遅く出て、最適化が効きすぎに見える | [Debug ビルドのイテレータデバッグ](#計測は必ず-release-で行う) |
| ベンチの結果が 0 ns になる | [結果を誰も読まないのでコンパイラが処理ごと削除している](#計測は必ず-release-で行う) |
| 倍率は大きいのに実際のフレームレートが変わらない | [絶対値がフレーム予算に対して桁違いに小さい](#測定結果とそこから分かったこと) |
| フェッチ数や演算量を大きく削ったのに時間がほとんど変わらない | [メモリ帯域で頭打ちになっている](#なぜ下がらないかメモリ帯域で頭打ち) |
| Compute の結果がちらつく／前フレームの残像が出る | [`glMemoryBarrier` のビットが読み方と合っていない](#メモリバリアは読み方でビットが変わる) |

症状が上に無いときは [画面が真っ黒・真っ白になったときの調べ方](#画面が真っ黒真っ白になったときの調べ方) の切り分け手順から入ってください。

---

## 目次

1. [アーキテクチャ概要](#アーキテクチャ概要)
2. [GL リソースの持ち方](#gl-リソースの持ち方)
3. [UBO（Uniform Buffer Object）](#ubouniform-buffer-object)
4. [ガンマ補正](#ガンマ補正)
5. [Deferred Shading](#deferred-shading)
6. [頂点属性 location の割り当て規約](#頂点属性-location-の割り当て規約)
7. [新しいシェーダーの追加](#新しいシェーダーの追加)
8. [テクスチャの追加](#テクスチャの追加)
9. [GPU デバッグ（RenderDoc / KHR_debug）](#gpu-デバッグrenderdoc--khr_debug)
10. [画面が真っ黒・真っ白になったときの調べ方](#画面が真っ黒真っ白になったときの調べ方)
11. [よくある落とし穴](#よくある落とし穴)
12. [カメラ](#カメラ)
13. [衝突判定](#衝突判定)
14. [モデル読み込みと単位系](#モデル読み込みと単位系)
15. [フレームアリーナと計測](#フレームアリーナと計測)
16. [Bloom の Compute Shader 化](#bloom-の-compute-shader-化)

---

## アーキテクチャ概要

```
src/
 ├── main.cpp       メインループと ImGui のパネル
 ├── app/           Window（GLFW とコンテキスト） 入力 GUI
 ├── scene/         Camera Character Collision
 │                  SceneLayout.h（ライト・モデル・オブジェクトの配置） SceneModels（読み込んだモデルと操作キャラ）
 ├── render/        Scene（1フレームの描画を統括） SceneGeometry（床・キューブ・壁・窓のメッシュ）
 │    ├── pass/     1パス = 1クラス（ShadowPass GeometryPass … TonemapPass）
 │    ├── targets/  パス同士を繋ぐ FBO とテクスチャ（GBuffer HdrTarget ShadowCubeTargets …）
 │    └── ibl/      起動時に IBL のマップを焼く IblBaker
 ├── asset/         Model Mesh MeshDistanceField（Assimp での読み込みと SDF の焼き込み）
 ├── gl/            Shader Texture TextureCache GlHandle などの GL ラッパー
 ├── core/          FrameArena SceneUnits.h（寸法の定数）
 └── debug/         GlDebug GpuProfiler CollisionDebugDraw

shader_src/        common/ gbuffer/ shadow/ ssao/ lighting/ forward/ post/ ibl/ debug/
```

### 描画の流れ（`Scene::Render()`）

**`Render()` は `src/render/pass/` のパスを順に `Execute()` するだけで、各パスの中身はそれぞれのクラスにあります。**
パス同士は `src/render/targets/` の FBO とテクスチャを介して繋がっているので、順序には意味があります
（例: SSAO は G-Buffer が埋まっていないと計算できない）。
各パスは `GpuProfiler::Measure()` で囲まれており、GPU 時間の計測と RenderDoc のデバッググループを兼ねています。

```
updateTransparentInstances()  透過窓をカメラから遠い順に並べ、インスタンスVBOへ位置を送る
 ↓
ShadowPass            [1] 4灯ぶんループ。1灯につきサブパスが2つ
   深度サブパス        光源視点で深度だけを書く → 光源からの正規化距離のキューブマップ
                      床・キューブ・壁は既定では描かない（SDF が影を担当する）
   透過色サブパス      白でクリアし 深度書き込みを止めてガラスだけを乗算ブレンド → ガラスを透過した光の色
 ↓
updateMatricesUBO()   [2] UBO に view / projection を書き込む
 ↓
ReflectionPass        [2.5] 床で折り返した鏡像カメラから Geometry / Lighting / Forward を半解像度で借りて描く
                      （Planar reflection が有効なときだけ。終わったら UBO を戻す）
 ↓
GeometryPass          [3] GL_BLEND を切り 不透明物の位置・法線・アルベド・マテリアルを GBuffer へ書く
                      （この時点ではライティングもシャドウ判定も一切しない）
 ↓
SsaoPass              [4] G-Buffer から近距離の遮蔽率を求め ブラーまでかける
 ↓
SdfOcclusionPass      [4.5] SSAO が届かない数m規模の遮蔽を SDF のレイマーチで求める（半解像度）
 ↓
GBuffer::BlitDepthTo  [5] G-Buffer の深度を HdrTarget へコピーする（前方描画の深度テスト用）
 ↓
DeferredLightingPass  [6] G-Buffer + 影 + AO + IBL + 床の反射を合成し HdrTarget へ書く
 ↓
ForwardPass           [7] ライトキューブ → スカイボックス → ガラス（この間だけ GL_BLEND を有効化）
 ↓
BloomPass             [8] HdrTarget の明るい部分を Compute Shader でぼかす
 ↓
TonemapPass           [9] Bloom を加算し トーンマッピング＋ガンマ補正してデフォルト FBO へ
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
myCubemap_.reset(cache.loadCubemap(faces, false, ColorSpace::SRGB));
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

**C++ 側（`Scene::updateMatricesUBO()`）:**

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
| OpenGL に任せる  | `app/Window.cpp` で `glEnable(GL_FRAMEBUFFER_SRGB);` を呼ぶ |
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

`app/Window.cpp` はこの値を起動時に出力するようにしてあります。

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
cubeTexture_   = cache.get("resources/textures/bricks2.jpg",        true, ColorSpace::SRGB);
cubeNormalMap_ = cache.get("resources/textures/bricks2_normal.jpg", true, ColorSpace::Linear);
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

## Deferred Shading

### G-Buffer の構成（`GBuffer`）

| アタッチメント    | アクセサ            | 内部フォーマット | 中身                                                  |
| ----------------- | ------------------- | ---------------- | ----------------------------------------------------- |
| COLOR_ATTACHMENT0 | `Position()`        | `GL_RGBA16F`     | ワールド座標                                          |
| COLOR_ATTACHMENT1 | `Normal()`          | `GL_RGBA16F`     | rgb=ワールド法線（ノーマルマップ適用後）, a=metallic |
| COLOR_ATTACHMENT2 | `AlbedoRoughness()` | `GL_RGBA8`       | rgb=アルベド, a=roughness                             |

**metallic を法線の a に、roughness をアルベドの a に相乗りさせているのはこのプロジェクト独自の割り当て**です。
シェーダー側で `gNormal.a` / `gAlbedoRoughness.a` を読むときはこの表と揃えてください。

> **`GL_RGB16F` ではなく `GL_RGBA16F` を使うこと。**
> OpenGL の必須フォーマット表では、RGB16F は「テクスチャとしては必須／レンダーターゲットとしては非必須」に
> 分類されています。3成分しか使わなくても RGBA を選ぶのが安全です。

### シャドウ判定は Lighting パスでする

**Geometry パスではシャドウ判定をしません。** ここでやってしまうと、結局オブジェクトの数だけ重い計算をすることになり、
Deferred にした意味が消えます。G-Buffer から読んだ位置がそのまま `ShadowCalculation()` の `fragPos` に使えるので、
`shadowMap[]` のサンプラーと `ShadowCalculation()` は `shadow_common.glsl` に置き、
`deferred_lighting.frag`（とガラスの `glass.frag`）から include しています。

### 深度バッファの引き継ぎ

Geometry パスは `GBuffer` に、後続の前方描画は `HdrTarget` に描きます。この2つは**別々の深度バッファ**を持っています。
そのままだと `HdrTarget` の深度は空なので、スカイボックスが手前のキューブを無視して画面全体を覆ったり、
ライトキューブが壁の裏にあるのに手前に描かれたりします。
これを防ぐため、Lighting パスの前に `GBuffer::BlitDepthTo()` で深度だけをコピーしています。

> **コピー後の `glClear` に注意。** せっかくコピーした深度を、その後のパスで
> `glClear(GL_DEPTH_BUFFER_BIT)` してしまうと台無しになります。
> `DeferredLightingPass` でクリアしてよいのはカラーだけです。

`DeferredLightingPass` は `glDisable(GL_DEPTH_TEST)` してフルスクリーンクワッドを描きます。
**OpenGL では `GL_DEPTH_TEST` を無効にすると深度書き込みも行われない**ので、
コピーしてきた深度がクワッドによって上書きされることもありません。

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
| 6        | boneIds（`ivec4`）。整数なので `glVertexAttribIPointer` で送る。`glVertexAttribPointer` だと float に変換されて壊れる |
| 7        | boneWeights（`vec4`）                                             |

### なぜ規約が必要か

VAO ごとに location の意味がバラバラだと、**複数のメッシュを同じシェーダーで描くパス**で必ず破綻します。
実際にこのプロジェクトでは、以前 `cubeVAO_` が location 3 を「インスタンス位置」に、
`wallVAO_` が location 3 を「タンジェント」に使っていました。

その状態でシャドウデプスパス（`point_shadow_depth.vert` は location 3 を `aOffset` として読む）から
壁を描くと、壁の各頂点が**タンジェントベクトルのぶんだけずれた位置**でシャドウマップに焼かれます。
エラーは一切出ず、影の位置だけが微妙にずれるという分かりにくい不具合になります。

**インスタンス用の属性を 5 に置き、他の用途で使わない**のが要点です。非インスタンス描画の VAO は 5 を使わないので、衝突しません。

ボーン属性を 6 / 7 から始めているのも同じ理由です。5 にボーンIDを割り当てると、モデルをシャドウパスで描いたときに
**整数のボーンIDが座標オフセットとして解釈され、メッシュがばらばらに飛び散ります。**
コンパイルエラーにも GL エラーにもなりません。

### 使わない location はどうなるか

床（`planeVAO_`）は location 5 を `glEnableVertexAttribArray` していませんが、
`point_shadow_depth.vert` は `aOffset` を宣言しています。これは意図的に成立させています。

OpenGL では、**頂点属性配列が無効の場合、シェーダーは「カレント汎用頂点属性値」を読み**、その初期値は `(0, 0, 0, 1)` と規定されています。
つまり `aOffset` は `(0,0,0)` になり、`aPos + aOffset` は元の座標のままになります（加算の単位元）。

> **ただしこの値はコンテキストの状態であって VAO の状態ではありません。**
> どこかで `glVertexAttrib3f(5, ...)` を呼ぶと、location 5 を無効にしている全ての描画がその値を拾ってしまいます。
> このプロジェクトでは誰も呼んでいないので成立していますが、依存していることは意識しておいてください。

### 未使用の location はドライバにシェーダーを作り直させる

上は「**結果は正しくなる**」という話でした。ただし**性能上は代償があります。**

**症状:** 起動直後にコンソールへ以下が並ぶ（`GlDebug.cpp` のデバッグ出力を入れて初めて見えた）。

```
[GL MEDIUM] API / Performance (id 131218)
  Vertex shader in program 16 is being recompiled based on GL state.
```

**原因:** シャドウデプスパスが、**1本のシェーダーで5種類の頂点レイアウトを描いている。**

`point_shadow_depth.vert` は location 0 / 2 / 5 / 6 / 7 を宣言していますが、
`ShadowPass::Execute()` が渡す VAO は毎回違います。

| 描画 | VAO | 有効な location | 宣言に対して欠けるもの |
| --- | --- | --- | --- |
| `SceneGeometry::DrawFloor` | `planeVAO_` | 0,1,2 | 5,6,7 |
| `SceneGeometry::DrawCubes` | `cubeVAO_` | 0,1,2,3,4,5 | 6,7 |
| `SceneGeometry::DrawWalls` | `wallVAO_` | 0,1,2,3,4 | 5,6,7 |
| `SceneModels::Draw` | Mesh の VAO | 0,1,2,3,4,6,7 | 5 |
| `SceneGeometry::DrawWindows` | `transparentVAO_` | 0,1,2,5 | 6,7 |

（床・キューブ・壁は ImGui の `Static casters in shadow map` を有効にしたときだけ描かれる）

**なぜそうなるか:** 属性配列が無効だとシェーダーはカレント頂点属性値を読みますが、
これは**頂点ごとに配列から読む**のとは機械語のレベルで別物です。
ドライバは「その属性を定数として読む版」の頂点シェーダーを組み合わせごとに作り直します。
`program 16` の再コンパイルが4回出るのは、上の表の組み合わせ数と一致します。

**影響:** パッチ後の版はキャッシュされるので**毎フレームの負荷にはなりません。**
初めてその組み合わせを踏んだ瞬間だけヒッチ（一瞬の引っかかり）が出ます。

**対処（現状は直していない）:** 製品のエンジンは**シェーダー順列**で解決します。
`SKINNED` / `INSTANCED` などのフラグでバリアントを事前にビルドし、
実行時にドライバへパッチさせません。

このプロジェクトは**シェーダー本数を増やさないことを優先**して現状を選んでいます。
直すならデプスパスを「静的 / インスタンス / スキニング」の3本に分けることになりますが、
影響が起動時のヒッチに限られるため、優先度は低いと判断しています。

---

## 新しいシェーダーの追加

1. **`shader_src/` の用途に合うサブディレクトリにファイルを作る**（`gbuffer/` `lighting/` `forward/` `post/` など）
2. **CMakeLists.txt の `SHADER_SOURCES` に追記する。** ビルド時に `copy_assets` ターゲットが exe の隣へコピーする
3. **使うパスのクラスにメンバとして持ち、コンストラクタの初期化子で生成する**

```cpp
// src/render/pass/MyPass.h
Shader myShader_;

// src/render/pass/MyPass.cpp
MyPass::MyPass() : myShader_("my_shader.vert", "my_shader.frag") {}
```

> **注意: ファイル名はサブディレクトリをまたいで一意にすること。**
> `SHADER_SOURCES` は exe の隣へ**フラットに**コピーされるので、`Shader` にはディレクトリ無しのファイル名だけを渡します。
> 別のディレクトリに同名のファイルがあると、後からコピーされたほうが黙って上書きします。
> `#include "pbr_common.glsl"` がディレクトリ無しで書けるのも同じ理由です。

`SHADER_SOURCES` に書き忘れると exe の隣にファイルが無いので、起動時に `ERROR::SHADER::FILE_NOT_FOUND` が出ます。

---

## テクスチャの追加

```cpp
// SceneGeometry::initTextures()
myTexture_ = cache.get("resources/textures/filename.png", true, ColorSpace::SRGB);
// 第2引数 flip:       true = 上下反転あり（通常はtrue）、アルファ付きPNGも自動判別
// 第3引数 colorSpace: SRGB = アルベドなど「色」 / Linear = 法線マップなど「値」
```

**第3引数の選択を間違えるとエラーが出ないまま陰影だけが狂います。**
デフォルト値をあえて持たせていないので、追加のたびに「色かデータか」を判断してください。
理由と判断基準は [ガンマ補正](#ガンマ補正) の「アルベドがリニア空間になっていなかった」を参照。

バインドするユニット番号は直書きせず、[TextureUnits.h](../src/render/TextureUnits.h) の `texunit::*` を使います。
パスをまたいで同じサンプラーを使うシェーダー（Deferred とガラスなど）で割り当てを揃えるためです。

---

## GPU デバッグ（RenderDoc / KHR_debug）

`KHR_debug` は OpenGL 4.3 以降のコア機能なので、拡張の存在確認は不要です（このプロジェクトは 4.6 コア）。

### RenderDoc で起動するときは Working Directory を必ず指定する

**症状:** RenderDoc の `Launch Application` から起動すると、真っ黒な画面になるか即座に落ちる。
`cmake --build --preset run` では正常に動く。

**原因:** シェーダーと `resources/` は exe の隣にコピーされ、コード側は**相対パス**で開きます。
作業ディレクトリが exe のフォルダでないとファイルが見つかりません。

**対処:** `Working Directory` に exe のあるフォルダ（`build/Debug`）を入れる。
`add_custom_target(run)` が `WORKING_DIRECTORY` を指定しているのと同じ理由です。

### パス名は `GpuProfiler` から出している

`glPushDebugGroup` / `glPopDebugGroup` は `GpuProfiler::begin()` / `end()` の中にあります。
`Measure()` が既に全パスを囲んでいるため、**描画コード側には1行も無い**のが要点です。

Push を `measuring_` のガードより**後**に置くこと。前に置くと入れ子で呼ばれたときに
Pop が足りず、RenderDoc のツリーが崩れて GL エラーになります。
入れ子防止のフラグが、そのまま Push/Pop の対応も保証しています。

デバッググループはツール未接続時のコストがほぼゼロなので、Release でも有効のままにしています。

### デバッグ出力は Debug ビルドだけで有効にしている

`GL_DEBUG_OUTPUT_SYNCHRONOUS` は GL 呼び出しごとにドライバが同期を取るため性能に影響します。
**Release は GPU 時間計測とベンチに使う構成なので、計測基盤そのものを歪めないよう
`GLFW_OPENGL_DEBUG_CONTEXT` を Debug ビルドでのみ要求**しています（`app/Window.cpp`）。
`EnableDebugOutput()` は `GL_CONTEXT_FLAG_DEBUG_BIT` を見て、無ければ黙って何もしません。

同期を有効にする理由は、**原因となった呼び出しのその場でコールバックが走る**ためです。
コールバックにブレークポイントを置けばコールスタックが犯人を直接指します。
無効だと後から通知が来るだけで、どの呼び出しが原因か分かりません。

### ドライバのノイズは ID で個別に切る

**症状:** `The driver allocated storage for renderbuffer N` が大量に出てログが読めない。

**原因:** NVIDIA は確保の報告を `GL_DEBUG_SEVERITY_LOW` で出します。
`NOTIFICATION` を切るだけでは落ちません。

**対処:** `glDebugMessageControl` に ID の配列を渡して個別に無効化します（`GlDebug.cpp` の `noisyIds`）。
`131169` / `131185` / `131204` を切り、**性能警告の `131218` は残しています。**
severity でまとめて切ると、[頂点シェーダーの再コンパイル警告](#未使用の-location-はドライバにシェーダーを作り直させる)のような
本物の指摘まで消えます。

> ID 指定で呼ぶときは `source` と `type` を `GL_DONT_CARE` にできず、`severity` は
> `GL_DONT_CARE` でなければならないという制約があります。

---

## 画面が真っ黒・真っ白になったときの調べ方

グラフィックスの不具合は**エラーが一切出ないまま画面だけがおかしくなる**ことがほとんどです。
勘で直そうとすると延々と時間を溶かすので、以下の順で機械的に切り分けてください。

### 原則1: パスを1つずつ「中身を画面に出して」確認する

多段パス構成では、どのパスまで正しいかを1つずつ確認するのが最短です。
ImGui の `View` で `deferred_lighting.frag` の `debugMode` を切り替えると、中間バッファをそのまま画面に出せます。
番号と表示内容の対応は `main.cpp` の `kDebugModes` が正です。

| 値          | 表示内容                                                      |
| ----------- | ------------------------------------------------------------- |
| 0           | 通常のライティング                                            |
| 1 / 2 / 8   | ライト0のシャドウ判定 / `shadowMap[0]` の生の深度 / 透過色    |
| 3〜5, 9, 10 | G-Buffer の Albedo / Normal / Position / Metallic / Roughness |
| 6           | 画面4分割で G-Buffer を一度に表示                             |
| 7           | SSAO                                                          |
| 11〜13      | IBL の Irradiance / Prefilter / BRDF LUT                      |
| 14〜19      | SDF 遮蔽の可視性とステップ数（拡散・ソフトシャドウ・鏡面）    |

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

ImGui の `Raw output (skip tonemap/bloom)`（`hdr.frag` の `debugRawOutput`）を ON にすると
Bloom 合成・トーンマッピング・ガンマ補正をすべて飛ばし、生の値をそのまま出力します。
G-Buffer を可視化するときは**必ず ON にしてください。**

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

`app/Window.cpp` では透過窓のために起動時に一度だけ有効化しています。

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

**対処:** `GeometryPass::Execute()` の冒頭で `glDisable(GL_BLEND)` し、`ForwardPass` で透過窓を描く間だけ有効化する。

そもそも G-Buffer に入るのは色ではなく座標や法線という**幾何情報**なので、
ブレンドという概念自体が意味を持ちません。Deferred Shading では必ず無効にします。

### MRT では有効な全ての draw buffer に書き込む

`glDrawBuffers` で複数のカラーアタッチメントを有効にしている場合、
フラグメントシェーダーが**書き込まなかったアタッチメントの値は「未定義」になります**。

このプロジェクトでは `HdrTarget` が `color_`（location 0）と
`brightColor_`（location 1, Bloom用）の2枚を同時に有効にしています。
そこへ `skybox.frag` が `FragColor` しか書かないと、スカイボックスが覆う画面全域で
`brightColor_` にゴミが入り、それが `BloomPass` のブラーで拡散して**画面全体に白い靄がかかります。**

Bloom させたくないシェーダーでも、必ず黒を明示的に書いてください。

```glsl
layout (location = 1) out vec4 BrightColor;
// ...
BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
```

### 影が見えないときは「明るすぎ」を疑う

影が出ない原因は、シャドウマップの不具合とは限りません。以下の3つが重なると影は簡単に見えなくなります。

1. **ライトの到達距離が長すぎる** — 4灯すべてがシーン全域に届くと、1灯が遮られても残り3灯が影を埋めます。`diffuse` を下げて影響範囲（`PointLight::calcRadius()`）を絞るのが有効です
2. **トーンマッピングの飽和** — `1 - exp(-c * exposure)` は明るい領域ほど差が潰れます。HDR値4.0は0.86、8.0は0.98と、ほとんど区別できなくなります
3. **Bloom の滲み** — 輝度1.0を超える面が広いと、ブラー結果が影の上に加算されます

**全体を明るくしたいときは、ライトの `diffuse` ではなく ImGui の `Exposure` を上げてください。**
exposure は HDR 値そのものを変えないので、Bloom の閾値や飽和に影響せず、影のコントラストを保てます。

### 環境光を光源ごとに減衰させると AO が光源の近くでしか見えない

**症状:** SSAO を入れたのに、光源のすぐ近くでしか AO が見えない。エラーは出ない。

**原因:** 環境光を LearnOpenGL の Multiple Lights の章の形のまま、ライトごとのメンバとして持ち
`attenuation` を掛けていた。

**なぜそうなるか:** SSAO が掛かる対象は環境光の項だけです。環境光を距離で減衰させると、
光源から離れた場所では環境光そのものが 0 に近づき、AO を掛ける相手が消えてしまいます。
LearnOpenGL でも SSAO の章では環境光がシーン全体の定数に戻っています。

**対処:** 環境光はシーン全体で1つ（`RenderSettings::ambientStrength`、ImGui の `Ambient`）だけ持ち、
ライトのループの**外**で `AO` と掛け合わせて加算する。`PointLight::ambient` は使っていません。
`ambientStrength` は `lighting_common.glsl` で宣言しているので、Deferred とガラスで二重管理になりません。

### 未設定の uniform は 0 になる（単位行列ではない）

GLSL では uniform の初期値はゼロです。`mat4 model` を設定し忘れると**ゼロ行列**になり、
`model * vec4(aPos, 1.0)` が `(0,0,0,0)` になります。

これは「`FragPos` が全ピクセルで `(0,0,0)`」という症状として現れるので、
G-Buffer の Position が一様になったときはまずここを疑ってください。

### ライトキューブを FBO の外で描画してしまう

`TonemapPass` がデフォルト FB に戻した後にライトキューブを描画すると：

- `glDisable(GL_DEPTH_TEST)` が有効な状態なので全オブジェクトの手前に描画される
- カスタム FB を通さないのでポストプロセス（ガンマ補正等）が適用されない
- 結果として「2D の板ポリ」に見える

**ライトキューブは `ForwardPass` が `HdrTarget` をバインドしている間に描くこと。**

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
shader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
// モデル行列が単位行列の場合は glm::mat3(1.0f) でも同じ
```

`GeometryPass` の各シェーダーと `ForwardPass` のガラスのシェーダー、それぞれに設定すること。

### 同じ .frag でも uniform はシェーダープログラムごとに設定する

`deferred_lighting.frag` と `glass.frag` が同じ `lighting_common.glsl` を include していても、プログラムオブジェクトは別です。
`viewPos` や `pointLights[0]`、`shadowMap[]` のユニット番号などの uniform は、
`DeferredLightingPass` と `ForwardPass` のそれぞれで個別に設定する必要があります。
`ShadowPass` の深度用と透過色用のように、同じ `.vert` / `.geom` を共有するプログラム同士も同様です。

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

### 半解像度パスで「入力テクスチャの解像度」からスケールを作ると壊れる

**症状:** SDF 遮蔽を半解像度にした途端、床に縦方向の筋が出る。エラーは出ない。

**原因:** ノイズを敷き詰める倍率を、**入力テクスチャ（フル解像度）** から作っていたこと。

```glsl
// 壊れていた版
vec2 noiseScale = vec2(textureSize(gPosition, 0)) / 4.0;  // gPosition はフル解像度
float angle = texture(texNoise, TexCoords * noiseScale).x * PI;
```

`TexCoords` は 0〜1 なので、**描画先が半分になっても倍率は変わりません。**
その結果 4x4 のノイズが「4 画素ごと」ではなく「2 画素ごと」に繰り返し、

- 16 個ある乱数のうち **4 個しか使われない**（回転角の種類が 1/4 に減る）
- 後段の 4x4 ブラーとタイル周期が噛み合わない

**対処:** 描画先の座標から作れば解像度に依存しません。

```glsl
float angle = texture(texNoise, gl_FragCoord.xy / 4.0).x * PI;
```

**一般化:** `textureSize()` は入力の寸法、`gl_FragCoord` は出力の寸法です。
**画面上の周期**が欲しいときは必ず後者から作ること。SSAO のように入出力が同解像度だと
どちらでも動いてしまうので、解像度を変えた瞬間に初めて壊れます。

### sphere tracing は面に漸近するとステップを使い切る

**症状:** ガラスの反射で、水平線あたりに横一本の明るい帯が出る。そこだけ遮蔽が効かない。

**原因:** 歩幅が「最も近い面までの距離」そのものなので、**面とほぼ平行に進むレイは進めません。**

高さ 0.2m から水平より 0.5° 下へ進むレイは、床まで 23m あります。1 歩ごとに
高さが `1 - sin(0.5°) = 0.9913` 倍にしかならないので、

```
必要な歩数 = ln(0.002 / 0.2) / ln(0.9913) ≒ 527 歩
```

| 水平からの角度 | 必要な歩数 |
| --- | --- |
| 2° | 約 130 |
| 0.5° | 約 527 |
| 0.1° | 約 2640 |

**角度が 0 に近づくほど発散するので、`SDF_MAX_STEPS` をいくつにしても足りません。**
「歩数を増やす」では解決しない種類の問題です。

**対処:** 「抜けきった」と「使い切った」を区別し、後者は遮蔽として扱います。

```glsl
        t += d;
        if (t > sceneParams.w) return res;  // 抜けきった
    }
    return 0.0;  // 使い切った = 面に漸近していた
```

開けた空間なら歩幅が指数的に伸びて 30 歩ほどで抜けます。**使い切ったということは
面にへばりついていたということ**なので、遮蔽と答えるのが妥当です。逆に `res`（≒1.0）を
返すのは「一度も抜けられなかったレイは完全に見えている」と主張することになります。

**副作用:** 水平から約 3° 以内の**上向き**レイも歩数が足りず、誤って遮蔽扱いになります。
範囲が狭く、誤りが暗い側に出るので許容しています。

### std140 の配列に `vec3` を使ってはいけない

**症状:** UBO で送った座標がずれる。エラーは出ない。

**原因:** `std140` は**配列の要素を 16 バイト境界に置く**ので、`vec3` の配列は
1 要素あたり 4 バイトのパディングが入ります。C++ 側で `glm::vec3` を詰めて送ると
1 要素ずつずれていきます。

**対処:** 全メンバを `vec4` にすれば、GLSL と C++ の並びが自動的に一致します。

```glsl
layout(std140, binding = 2) uniform SdfScene {
    vec4 boxCenters[SDF_MAX_BOXES];  // xyz だけ使う w は捨てる
    vec4 boxHalfSize;
};
```

```cpp
struct SdfSceneBlock {
    glm::vec4 boxCenters[kSdfMaxBoxes];
    glm::vec4 boxHalfSize;
};
```

スカラー（`float` / `int`）を混ぜる場合も同じ理由でパディングを読み違えやすいので、
`vec4` にまとめて成分に意味を割り当てるほうが安全です。

### include される側に無い uniform は、コンパイルエラーにもならず黙って捨てられる

**症状:** 新しい `uniform float` を1つ足してスライダーを繋いだのに、
片方のシェーダー（例: ガラス）だけ効かない。

**原因:** [`lighting_common.glsl`](../shader_src/common/lighting_common.glsl) の
コメントにある通り、Deferred とガラスで共有する uniform は
**include される側（`lighting_common.glsl`）で1回だけ宣言する**規約です。
新しい uniform を `deferred_lighting.frag` 側にだけ書くと、
それを include していない `glass.frag` には存在しません。

存在しない uniform 名で `Shader::setFloat()` を呼んでも、
`glGetUniformLocation()` が `-1` を返すだけで **GL エラーは出ません**。
送信側は「送ったつもり」のまま気付けません。

**対処:** Deferred とガラスの両方で使う値は、必ず `lighting_common.glsl` に
宣言する。片方にしか要らない値だけをそのシェーダー内で宣言すること。

### シャドウマップと SDF ソフトシャドウは `max` で合成する（`+` ではない）
SDF ソフトシャドウの出典：https://iquilezles.org/articles/rmshadows/

**症状の元になる設計判断:** ある形状（床・キューブ・壁）の影を
シャドウマップと SDF の両方が同時に落とすと、半影の柔らかさが消えて
常にシャドウマップ側のハードな輪郭が勝ってしまう。

**理由:** どちらも「1.0 = 完全な影」を返す独立した遮蔽判定です。
片方が遮っていればもう一方の値に関わらず影であるべきなので、
[`deferred_lighting.frag`](../shader_src/lighting/deferred_lighting.frag) では
`shadow = max(shadowMapの結果, sdfの結果)` で合成しています。`+` で足すと
1.0 を超えて意味が壊れます。

**対処（設計）:** 同じ形状を両方に描かせて `max` に頼るのではなく、
[`ShadowPass::Execute()`](../src/render/pass/ShadowPass.cpp) 側で
**SDF が担当する静的形状（床・キューブ・壁）をシャドウマップの深度パスから外す**
ことで、そもそも二重に遮蔽しないようにしています
（`shadowMapStaticCasters_` フラグで比較のため元へ戻せる）。

### SDF のプロキシ形状は描画メッシュの表面と面を揃える

**症状:** 壁だけ IBL の拡散・鏡面も直接光のソフトシャドウも一切当たらず、完全な黒になる。
床やキューブは正常。エラーは出ない。

**原因:** 描画される壁は厚さゼロの板ポリで、[`GeometryData.h`](../src/render/GeometryData.h)
の頂点は `z = ±floorHalfExtent` にあります。一方 [`SdfOcclusionPass`](../src/render/pass/SdfOcclusionPass.cpp)
が SDF 用に置いた箱は、その板を**中心**として厚みを持たせていました。

```
SDF の箱      z ∈ [-25.1, -24.9]
描画される壁   z = -25.0            ← 箱のど真ん中
```

壁の表面ピクセルは常に SDF の内側 0.1m に埋まっているため、`sdfConeVisibility()` は
最初のステップで `d < SDF_HIT_EPSILON` に落ち、レイの種類（拡散・鏡面・光源方向）を
問わず即座に `0.0`（完全遮蔽）を返します。`SDF_NORMAL_BIAS`（0.02m）は
箱の半分の厚み（0.1m）を押し出すには足りません。

床とキューブが無事だったのは、SDF の形状（無限平面 / `halfSize = 0.5`）が
**描画メッシュの表面とそのまま一致する**寸法だったからです。

**対処:** 板ポリの位置は動かさず、箱の中心を外側へオフセットして
**箱の内側の面が板の位置に一致する**ようにします。

```cpp
// 描画される壁は厚さゼロの板なので 箱の内側の面がそこに重なるよう外へずらす
block.wallCenters[0] =
    glm::vec4(0.0f, wallCenterY, -gl::units::floorHalfExtent - wallHalfThickness, 0.0f);
```

**一般化:** 厚みを持たせた解析形状で実メッシュを近似するときは、
「中心を合わせる」のではなく「表面を合わせる」こと。中心合わせは
薄い形状ほど誤差の割合が大きくなり、ここでは 100% 埋まる結果になりました。

### レイマーチの最小前進距離はソフトシャドウの補間式と衝突する（見送った案）

**症状:** ライトキューブによる減光も IBL の遮蔽も、本来は光源・遮蔽物から
離れるほど滑らかに変化するはずが、くっきりした境目ができる。
`SDF_MAX_STEPS` をいくつに変えても症状は変わらない。

**やろうとしたこと:** [#68](https://github.com/KDKyota/glfwdojo/issues/68) のグレージングレイ対策として、
`sdfConeVisibility()` の前進量に下限を設けた。

```glsl
t += max(d, SDF_MIN_STEP);  // d が小さくても最低 SDF_MIN_STEP は前進する
```

**原因:** ソフトシャドウの補間式（Sebastian Aaltonen 版、出典は上記と同じ
https://iquilezles.org/articles/rmshadows/ ）は、

```glsl
float y = d * d / (2.0 * prevD);
```

**「連続する2サンプルの間隔が正確に `prevD`（前ステップの `d`）である」**という前提の上に
成り立っている。前進量に下限を設けると、`d < SDF_MIN_STEP` の区間だけ
実際に進んだ距離が `prevD` と食い違うようになり、この前提が崩れる。

**なぜ閾値を小さくしても直らないか:** `SDF_MIN_STEP` が効くのは `d` が小さい場面、
つまり**面のすぐ近く＝グレージングレイ対策として最適化したい場面そのもの**。
`SDF_MIN_STEP` を半分にしても、`d` がその近辺でさらに小さければ
「本来の前進量に対する強制前進量の倍率」は縮まらない。

```
SDF_MIN_STEP = 0.1,  d = 0.005 → 実際の 20 倍
SDF_MIN_STEP = 0.05, d = 0.005 → 実際の 10 倍   ← 半分にしても大きく違わない
```

速く進ませたい領域と、補間式の前提が崩れる領域が完全に一致しているため、
**「ちょうどいい値」が存在しない**（`SDF_HIT_EPSILON` まで小さくすれば誤差は消えるが、
そのときには速度改善効果もほぼ消える）。

**対処:** この最適化は見送り、`t += d;` に戻した。半球平均を取る AO
（`sdfSkyVisibility`）限定であれば、1 方向の歪みが 8 本の平均とブラーでならされて
目立たない可能性は残っているが未検証。

### sphere tracing の歩幅は画素ごとに位相がずれ、同心円状の縞になる(Issue #67)

**症状:**  壁際の箱を床すれすれの浅い角度で見ると、IBL の鏡面反射に同心円状の
縞（波紋）が出る。`SDF occlusion` を 0 にすると消える。`SDF_MAX_STEPS` を
増やしても値は変わらない（後述する打ち切りの問題を直した後の話）。

**切り分け：** `debugMode` に鏡面コーントレースの生値（可視性そのもの）と
使用ステップ数をそのまま出す表示を追加して確認した。**両方に同じ位置で
同じ波紋が出る** — これが決め手になった。「ステップ数が周期的に変わっている」
ことを直接示しているので、原因はコーントレースの外側（Bloom やトーンマッピング）
ではなく、トレースそのものに絞り込める。

**原因１（打ち切り位置の跳ね）:** 鏡面は `tMax = sceneParams.w`（部屋の幅）
まで飛ばす。コーンの半径は `closest / (tClosest * coneTangent)` で
**t に比例して太り続ける**ので、`tMax` 付近では反対側の壁が必ずコーンに入る。
ところがそこで打ち切られると、結果は「最後のサンプルが `tMax` の手前に
落ちたかどうか」という位相依存の値になり、隣の画素と大きく食い違う。

さらに、`SDF_RES_THRESHOLD` による早期リターンにも同種の問題があった。
壁に正面から近づくレイは 1 歩ごとに距離が指数的に縮むため、閾値を割った
瞬間のサンプル距離が開始位置の端数によって `0.003〜0.023 m` の間を
周期的に動く。本来は次の 1 歩で確実に衝突する（`res` は事実上 0）距離なのに、
打ち切りがその 1 歩手前で発動し、位相依存のノイズをそのまま返していた。

**対処１:** 打ち切り距離 `tMax` の手前 3 割をフェード区間とし、そこに近い
遮蔽ほど寄与を 1.0（遮蔽なし）に近づける重み `sdfRangeWeight()` を導入した。
早期リターンも `res` をそのまま返さず `0.0` に倒すよう変更した。

```glsl
float sdfRangeWeight (float t, float tMax, float fadeRange) {
    return 1.0 - smoothstep(tMax - max(fadeRange, 1e-4), tMax, t);
}
// ...
res = min(res, mix(1.0, visibility, sdfRangeWeight(tClosest, tMax, fadeRange)));
if (res <= SDF_RES_THRESHOLD) return 0.0;  // res ではなく 0.0
```

**原因２（歩幅そのものの位相ずれ）:** 対処１だけでは波紋が残った。
`sceneSDF()` が返す距離＝歩幅は「その画素の開始位置と方向」だけで決まる。
隣り合う画素はわずかに開始位置がずれるだけで、「何歩目でどの距離を測るか」
という位相がまったく揃わない。最接近点の推定（Sebastian Aaltonen 法）は
サンプル**点**の間を補間するだけなので、この位相ずれ自体は解消できない。

**対処２:** 最初の一歩だけ歩幅を意図的に伸縮させ、位相をずらした2本の
トレースを平均する。1本だと「たまたま良い位相を踏んだ画素」と「悪い位相を
踏んだ画素」が隣り合って段差になるが、2本の平均を取ると片方が他方を均す。

```glsl
t += (steps == 0) ? d * startPhase : d;  // startPhase を 0.25 / 0.75 で2回呼ぶ
```

密サンプリングした真値と比較すると、隣接画素の段差の平均が壁領域で
`0.0082 → 0.0043` に減った。ゼロにはならない — sphere tracing の歩幅が
距離そのものに依存する以上、位相のずれは原理的に残る。

**副作用（別 Issue で対応）:** コーントレースが2本になった分、これを
半球4方向でサンプルする拡散 AO（`sdfSkyVisibility`）は実質8本のトレースに
なり、GPU 負荷が SdfOcclusion だけで約3倍に増えた。拡散側は `tMax` が
短く位相ずれの影響が小さいので、2本平均は鏡面だけに絞る余地がある。

**切り分けの型:** 縞が等距離の輪なら、レイの開始位置で決まる「歩幅の刻み」
を疑う。`SDF_MAX_STEPS` を変えても症状が変わらないなら、歩数不足ではなく
歩幅の位相か打ち切り位置が原因。`debugMode` で可視性の生値とステップ数を
並べて見比べると、両方に同じパターンが出るかどうかで切り分けられる。

---

### 距離場のサンプラーを設定し忘れると AABB 全体が遮蔽物になる(Issue #69)

**症状:** 3Dモデルの SDF 遮蔽が、モデルの形ではなく**直方体の形**で出る。
`debugMode 14`（SDF visibility）では正しくモデルの形に見えるのに、
`debugMode 17`（SDF spec visibility）と本描画では箱型になる。モデル自身も真っ黒になる。

**原因:** `sdf_common.glsl` を include しているシェーダーのうち、
`modelDistanceFields[]`（`sampler3D` の配列）を設定していたのは `sdf_occlusion.frag` だけだった。
`deferred_lighting.frag` と `glass.frag` は未設定のままだった。

**なぜそうなるか:** 設定されていない sampler uniform は**既定値の 0**、つまり
テクスチャユニット 0 を指す。そこには G-Buffer の position（2Dテクスチャ）がバインドされており、
`sampler3D` としては不完全なので `texture()` は **`(0,0,0,1)` を返す**。距離が 0 なので

```glsl
float d = texture(modelDistanceFields[i], uvw).r;   // 常に 0.0
if (d < SDF_HIT_EPSILON)                            // 必ず真
```

となり、**AABB の中に入ったレイが無条件で衝突扱い**になる。GL エラーも
シェーダーのコンパイルエラーも一切出ない。

**対処:** `sdf_common.glsl` を include する全てのシェーダーで `modelDistanceFields[]` を設定する。
ユニット番号は [TextureUnits.h](../src/render/TextureUnits.h) の `kSdfModelBase` から連番で確保している。

**切り分けの型:** 同じ距離場を使う表示なのに**パスによって結果が違う**なら、
距離場ではなくシェーダーごとの uniform 設定を疑う。
サンプラー配列は要素ごとに設定が必要で、書き忘れても何も言われない。

### クリップ距離を書かないシェーダーを混ぜると面がランダムに消える（Issue #2）

**症状:** 平面反射の中だけ、壁やキューブの面が虫食いのように消える。本描画は正常でエラーも出ない。

**原因:** `GL_CLIP_DISTANCE0` を有効にしたまま、`gl_ClipDistance[0]` を書かない頂点シェーダーで描いている。

**なぜそうなるか:** クリップ距離は有効化した時点で「シェーダーが値を書く」前提になる。
書かなければ値は未定義で、たまたまそこにあった値が使われる。
**負なら断片が捨てられる**ので、消えるかどうかが描画のたびに変わる。

**対処:** そのパスで描く頂点シェーダー全てに書く。ワールド座標を使うこと。

```glsl
uniform vec4 clipPlane;
gl_ClipDistance[0] = dot(vec4(FragPos, 1.0), clipPlane);
```

通常のパスは `GL_CLIP_DISTANCE0` を無効にしているのでこの行は無視される。
有効なまま何も切りたくない場合は `clipPlane` に `(0, 0, 0, 1)` を入れる（距離が常に 1 になる）。

### 反射面ちょうどの断片はクリップされず 反射像を覆う（Issue #2）

**症状:** 平面反射のテクスチャが床のテクスチャ一色になる。空も物体も映らない。

**原因:** 鏡像カメラは床を**下から見上げる**位置にいるので、床そのものが視界を覆う。
クリップ平面を床ちょうど（`y = floorY`）に置いても、床の断片は距離が **0** になる。
捨てられるのは**負のとき**だけなので、床は残ってしまう。

**対処:** 平面を数ミリだけ持ち上げて、床自身を切る側に入れる。

```cpp
constexpr float kFloorClipBias = 0.001f;
const glm::vec4 kFloorClipPlane(0.0f, 1.0f, 0.0f, -(units::floorY + kFloorClipBias));
```

物が床と接する部分が 1mm ぶん反射で欠けるが、平面反射では定番の割り切り。

### 平面反射は専用シェーダーを作らない（プロジェクト規約）

反射パスは `GeometryPass` と `DeferredLightingPass` を**そのまま借りて**、
view 行列とクリップ平面だけを差し替えて半解像度で走らせている（`ReflectionPass.cpp`）。

簡易シェーダーを別に用意すると、本体と反射像で見た目が食い違い、
マテリアルを足すたびに二重に実装することになる。

このとき注意が要るのは次の3点。

| 項目 | 扱い | 理由 |
| --- | --- | --- |
| 巻き順 | `glFrontFace(GL_CW)` に切り替える | 鏡像は三角形の向きが反転する。戻さないとカリングで面が消える |
| SSAO / SDF 遮蔽 | 強度 0 にして無効化 | メインビューの G-Buffer から作った画面空間の結果なので鏡像には合わない |
| `viewPos` | 鏡像側の位置を渡す | `Camera` は位置と姿勢しか持てず、反転した視点を表せない（`RenderView` を使う） |

**参考:**

- 反射行列の導出: <http://bcnine.com/articles/water/water.md.html>
- クリップ平面で平面反射を作る: <https://ii.uni.wroc.pl/~anl/cgfiles/Sig99AdvOpenGLnotes/node161.html>
- 実装の流れ（水面だが手順は同じ）: <https://trederia.blogspot.com/2014/09/water-in-opengl-and-gles-20-part3.html>


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

寸法を決める定数は `src/core/SceneUnits.h` の `gl::units` に集約しています。glTF が既定でメートル・Y-up なので、
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

**対策:** `layout::modelSpawns`（`src/scene/SceneLayout.h`）の `ModelSpawn::rotationDegrees` に、そのモデル用の補正回転を設定する。ボーン行列やローダー側は変更しない — 軸の向きは配置の問題であり、スキニングの計算とは別レイヤーで対処する。

### 新しい3Dモデルを追加する（実用手順）

1. **モデルファイルを `resources/` 以下に置く。** 推奨は `.glb`（glTF バイナリ、
   テクスチャも1ファイルに埋め込まれる）。キャラクターなら `resources/characters/`、
   静物なら `resources/publishable-objects/` に既存ファイルがあるので同じ並びに置く
2. **`src/scene/SceneLayout.h` の `modelSpawns` に1行足す。これだけでシーンに出る。**
   `Model`・`Scene`・シェーダーは何も変更しなくてよい（頂点データやVAOを自分で書く必要が無い。
   Assimp 経由で読み込む形式は完全にデータ駆動になっている）

   ```cpp
   // src/scene/SceneLayout.h
   inline const std::vector<ModelSpawn> modelSpawns = {
       // path, position, rotationDegrees, scale, followTarget(省略可、trueなら1体だけ)
       {"resources/characters/MyModel.glb", glm::vec3(0.0f, gl::units::floorY, 0.0f), glm::vec3(0.0f), 1.0f},
   };
   ```

3. **ビルドして起動するだけで表示される。** `SceneModels` のコンストラクタが
   `modelSpawns` を1行ずつループして `Model` を構築する

**うまく表示されないときに確認すること**

| 症状 | 確認箇所 |
| --- | --- |
| コンソールに `Skipped model: ...` と出て表示されない | パスが間違っている、または Assimp が対応していない形式。読み込み失敗は `try/catch` で握りつぶして読み飛ばすだけで、**起動は止まらない**（`SceneModels` のコンストラクタ） |
| モデルが横に倒れる・変な向きに立つ | `rotationDegrees` を設定する。[軸の向きが違うモデルは倒れて読み込まれる](#軸の向きが違うモデルは倒れて読み込まれる) 参照 |
| モデルが極端に大きい／小さい | `scale` を調整する。このプロジェクトは 1.0 = 1メートル規約（[モデル読み込みと単位系](#モデル読み込みと単位系)）なので、glTF 側が正しくメートルでエクスポートされていればほぼ 1.0 のままで合う |
| ボーンが動かずバインドポーズのまま固まる | モデル自体にスキニング・アニメーションが入っていない。次項参照 |

**キャラクターとして操作したい場合**

`followTarget: true` を追加する。三人称カメラの注視対象になり、`Character`（`SceneModels::character_`）が
そのモデルの spawn 位置・高さで生成される（`SceneModels` のコンストラクタ内、`spawn.followTarget` の分岐）。
**`true` は1体だけを想定している。** 2体以上に付けると `playerModelIndex_` が後勝ちで上書きされ、
カメラと `Character` は配列内で最後に `true` を付けたモデルだけを追従する。

**読み込み時の Assimp フラグ（`Model` のコンストラクタ / `Model.cpp`）**

```cpp
importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs |
    aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_LimitBoneWeights);
```

四角面や法線・タンジェントが無いモデルでもこのフラグで自動生成されるので、通常は
モデル側を事前加工する必要はない。`aiProcess_LimitBoneWeights` により1頂点あたりの
ボーン影響は自動的に4本へ切り詰められる（`gl::Vertex::boneIds` の枠と一致させるため）。

---

### 3Dモデルのアニメーションを再生する（実用手順）

**手順は無い。モデルにアニメーションが埋め込まれていれば、追加のコードなしで自動的に再生される。**

上の手順でモデルを `modelSpawns` に足しただけで、そのモデルにスキニング・アニメーション
（glTF の `animations` セクション）が入っていれば、`Model` コンストラクタ内で自動的に
アニメーションが読み込まれ、**1本目のクリップがロードした瞬間から再生され続ける**（ループ再生）。
`Character` や `Scene` 側で「このモデルのアニメーションを再生開始する」ような呼び出しを
どこかに書く必要は無い。

- **止める／再開する:** `SceneModels::UpdateAnimation()` が毎フレーム各モデルの `UpdateAnimation()` を呼んでいる。
  `deltaTime` に `0.0f` を渡すだけで、その瞬間の姿勢のまま止まる。
  操作キャラクターが立ち止まっている間はこれで止めている（待機モーションが無いため）
- **アニメーションが無いモデル:** バインドポーズ（`T` ポーズ等）のまま静止して描画される。
  エラーにはならない（`Model::HasAnimation()` が `false` を返す経路にそのまま乗るだけ）
- **複数のモーション（例: 歩行 + ジャンプ）を切り替えたい場合:** **現状は未対応。**
  `.glb` に何本クリップが入っていても、常に1本目（`animations_[0]`）しか再生されない。
  切り替えの仕組みそのものを実装する必要がある（[#31](https://github.com/KDKyota/glfwdojo/issues/31) 参照）

**変更するときに知っておくこと**

実装は `Model::loadAnimations()` / `Model::UpdateAnimation()` / `Model::nodeTransform()` にあります。

- **全クリップを読み込むが、再生するのは `animations_[0]` だけ。** 切り替えを作るなら `activeAnimation_` を外から変えられるようにする
- **再生時間は秒ではなく tick。** `ticksPerSecond` を掛けて進めている。glTF 側で `mTicksPerSecond` が 0 のことがあり、
  そのときは `Animation::ticksPerSecond` の既定値 `25.0f` が使われる
- **無条件にループする。** `std::fmod` で巻き戻しているので、単発再生を作るならここを変える
- **チャンネルが無いノードはバインドポーズのまま。** アニメーションが動かさないノードは `node.localTransform` がそのまま使われる

---

## フレームアリーナと計測

### 毎フレームのヒープ確保をアリーナへ置き換えた（実際に測った）

**変更前:** `Scene::Render()` の透過窓ソート用配列がローカルの `std::vector` でした。

```cpp
std::vector<gl::TransparentDraw> sorted;   // ここで毎フレーム確保
updateTransparentInstances(sorted);
```

スコープを抜ければ自動的に片付きますが、それは**確保と解放が毎フレーム律儀に往復している**ことの
言い換えでしかありません。`push_back` のたびにヒープ確保が走り、`Render()` の閉じ括弧で
デストラクタが解放します。

**変更後:** `gl::FrameArena`（`FrameArena.h`）から借りる形にしました。

```cpp
// Scene::Render()
frameArena_.Reset();   // 前フレームぶんをまとめて無効化

// Scene::updateTransparentInstances()
auto sorted = frameArena_.Allocate<gl::TransparentDraw>(windowPositions.size());  // ArraySpan（借り物）
```

アリーナは**起動時に確保した固定バッファ 1 本と `offset_` 1 個**だけを持ちます。
確保は `offset_` を進めるだけ、解放は用意せず、フレーム先頭で `offset_ = 0` に戻します。
戻り値が `std::vector` ではなく `ArraySpan<T>`（ポインタ + 要素数）なのは、
データを所有しているのがアリーナ側で、呼び出し側は借りているだけだからです。

### アリーナを書くときに外せない3点

- **アライメント。** `offset_` を素直にサイズぶん進めると型の要求する境界に乗らない。
  x86 では乗っていなくても動いてしまうことが多く、**別アーキテクチャで初めて落ちる**
- **デストラクタが呼ばれない。** `Reset()` は `offset_` を戻すだけなので、非 POD を置くと解放漏れになる。
  `static_assert(std::is_trivially_destructible_v<T>)` でコンパイル時に弾いている
- **リセットはフレーム末尾ではなく先頭。** アリーナから借りたデータを後段のパスへ渡すと、
  末尾でリセットしたときに使用中のデータを無効化しうる

### 計測は必ず Release で行う

**症状:** Debug ビルドでベンチを取ると `std::vector` 側だけが極端に遅く出て、
アリーナが実際よりずっと有利に見える。

**原因:** MSVC のイテレータデバッグが `std::vector` にだけ重いチェックを挿入するため。
アリーナは生ポインタなのでチェックの対象外になり、**比較の前提が揃わない。**

**対処:** `cmake --preset release` で構成した `frame_arena_bench`（`bench/frame_arena_bench.cpp`）を使う。
結果を必ず読む変数（`sink`）へ足し込むこと。これが無いとコンパイラが処理ごと削除して 0 ns が出ます。

### 測定結果と、そこから分かったこと

1フレームぶん（距離計算 → `std::sort` → 読み出し）の時間。カッコ内はアリーナを 1.00 とした倍率。

| instances | local vector | reserved | reused (`clear()`) | frame arena |
| --- | --- | --- | --- | --- |
| 6（現状） | 113.5 ns (x11.94) | 31.9 ns | 10.3 ns | **9.5 ns** |
| 64 | 525.0 ns (x2.18) | 286.8 ns | 272.1 ns | **240.3 ns** |
| 1024 | 30348.7 ns (x1.02) | 29703.8 ns | 29845.1 ns | **29762.5 ns** |

**① 比率が大きいことと効果が大きいことは別。**
6 要素での 11.94x は全ケース中で最大ですが、絶対差は 104 ns。
16.6ms の予算に対して **0.0006%** で体感差はありません。
比率が跳ねたのは、6 要素ではソートがほぼ無料で分母が確保コストだけになったからです。

**② 差分がそのままコストの正体を示す。**

| 差分 | 値 | 正体 |
| --- | --- | --- |
| `reserved` − `reused` | 21.6 ns | `malloc`/`free` **1往復** |
| `local` − `reserved` | 81.6 ns | **再確保**（容量が足りず確保し直してコピー） |
| `reused` − `arena` | 0.8 ns | `push_back` の容量チェック分岐 |

MSVC の `vector` は容量を 1.5 倍ずつ伸ばすので、6 要素の `push_back` では容量が
**0→1→2→3→4→6** と遷移し再確保が4回起きます。`81.6 ÷ 4 = 20.4 ns` は
1往復ぶんの `21.6 ns` とほぼ一致し、**独立に測った2つの値が同じコストモデルへ収束します。**

**③ `reused` とアリーナはほぼ同着（1.09x）。**
`transparentPositions_` が元々メンバ + `clear()` だったのは、
「一度確保して使い回す」が**1つの配列にだけ手作業で適用されていた**状態です。
アリーナの価値は速さではなく、**それを書く場所ごとに手作業でやらなくて済むこと**にあります。

**④ 1024 ではソートが全てを飲み込む。**
確保方式の差は 586 ns（全体の 2%）まで落ち、`reserved` がアリーナを上回る逆転すら起きます（測定ノイズ）。
ソート自体は要素 16 倍に対して 124 倍に伸びており、O(n log n) の予想（約 27 倍）を大きく超えます。
6 要素は挿入ソート、1024 要素はクイックソートの分割になり、ランダムな比較のたびに
**分岐予測ミス**でパイプラインが破棄されるためです（1比較あたり約 2.9 ns ≒ 9 サイクル）。

### 透過描画のボトルネックは CPU ではない（設計判断の根拠）

上の結果をフレーム予算に対する比率へ直すと、結論が変わります。

| ケース | CPU コスト | 16.6ms に対して |
| --- | --- | --- |
| 現状（6枚）の確保削減量 | 104 ns | 0.0006% |
| 1024枚でのソート全体 | 29.7 µs | 0.18% |

**1024 枚まで増やしても透過描画の CPU 側は予算の 0.2% に届きません。**
インスタンス数を増やしたときに最初に壊れるのは GPU 側のフィルレートです。理由は3つあります。

- 透過窓は**乗算パスと加算パスの2回**描いている（`ForwardPass::renderTransparentWindows()`）
- ブレンドが有効なのでオーバードローが積み上がる
- `glDepthMask(GL_FALSE)` で深度書き込みを切っているため**早期 Z で間引けない**

これは既存の `GpuProfiler` の `Forward` パスでそのまま観測できます。
インスタンス数を増やす検証を行う場合は、CPU 側のソートではなくここを見てください。

---

## Bloom の Compute Shader 化

### Compute のパス（`Shader` クラス）

`.comp` 単体を受け取るコンストラクタを足しています。`#include` の展開は既存の `expandIncludes()`
をそのまま通すので、共通 `.glsl` を Compute からも使えます。

出力先は FBO ではなく `glBindImageTexture` で `image2D` として結びます。
`GL_RGBA16F` は image load/store 可能な形式なので、既存のピンポンバッファをそのまま使えました。

### `barrier()` の前に return してはいけない

`barrier()` は**ワークグループの全スレッドが通る**必要があります。
画面外スレッドを先に `return` させると未定義動作になるため、範囲外の切り捨ては `barrier()` の後に置きます。

### メモリバリアは「読み方」でビットが変わる

**症状:** Bloom がちらつく、前フレームの残像が出る。エラーは出ない。

**原因:** `glMemoryBarrier` のビットは「**書いた側**」ではなく「**次にどう読むか**」で決まります。
`imageStore` した結果を `sampler2D` で読むなら `GL_SHADER_IMAGE_ACCESS_BARRIER_BIT` では足りません。

| 場所 | ビット | 次の読み方 |
| --- | --- | --- |
| ループ前 | `SHADER_IMAGE_ACCESS` | FBO へ書かれた `HdrTarget` の `brightColor_` を `imageLoad` |
| ループ内 | `SHADER_IMAGE_ACCESS` | 前パスの `imageStore` を `imageLoad` |
| ループ後 | **`TEXTURE_FETCH`** | `TonemapPass` が `sampler2D` で読む |

### 測定結果：4.4% しか速くならなかった

| | Bloom |
| --- | --- |
| フラグメント版 | 0.317 ms |
| **Compute + 共有メモリ版** | **0.303 ms** |

テクスチャフェッチは理論上 90回 → 約10回（9分の1）に減っているのに、時間は 4.4% しか減りません。
**フェッチ回数は律速ではありませんでした。**

### なぜ下がらないか：メモリ帯域で頭打ち

1600×900 の `GL_RGBA16F` は 1枚 **11.52 MB**。1パスで読み書きするので 23.04 MB、10パスで **230.4 MB/frame**。
RTX 5070 の帯域 約 670 GB/s で割ると **約 0.34 ms** となり、実測 0.303 ms とほぼ一致します
（L2 がパス間の受け渡しを一部吸収するぶん下回る）。

**共有メモリが減らせるのは「同じデータを何度も読む」コストだけで、
「最低1回は読んで1回は書く」コストは減りません。**同じバイト数がメモリバスを通るためです。
そして重複読みのほうは、既に L1/L2 がほぼ吸収していました。

### この構造の見分け方

**「理論上の演算量・フェッチ数を大きく削ったのに、時間がほとんど変わらない」**ときは、
帯域律速を疑ってください。切り分けは掛け算だけでできます。

```
(バッファのバイト数) × (読み書きの回数) ÷ (GPU のメモリ帯域)
```

これが実測に近ければ帯域律速で、**演算を減らす最適化はもう効きません。**
効くのは動かすバイト数を減らす手だけです。

- 半解像度でぼかす（バイト数 1/4。ぼかし後は低周波なので見た目はほぼ変わらない）
- パス数を減らす
- `R11F_G11F_B10F` にして 8 バイト → 4 バイト

### SSAO に共有メモリが効かない理由も同じ枠組み

共有メモリが効く条件は **隣接スレッドが同じ場所を読む**ことです。

| | 読む場所 | 共有メモリ |
| --- | --- | --- |
| Bloom のブラー | 横に連続した9テクセル | 重なるが、**L2 が既に吸収済み** |
| SSAO | 半球状にばらけた64サンプル | 隣と揃わず**効かない** |

SSAO は `SsaoPass.cpp` の `kKernelSize = 64` のサンプル数そのものがコストなので、
速くするならサンプル数を減らすか半解像度にするのが正攻法です。

---

## 今後の課題

今後の課題については GitHub の Issue でまとめています。
