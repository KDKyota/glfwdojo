# glfwdojo

リポジトリ名は気にしないでください．これは元々Publicにする予定ではなかったので，雑に命名したのでこうなっています．

[LearnOpenGL](https://learnopengl.com/) の内容を、モダンな C++（C++17）で一から実装し直しながら OpenGL とリアルタイムグラフィックスを学ぶための個人学習リポジトリです。

<table>
<tr>
<td><img src="images/Screenshot.png"></td>
<td><img src="images/Screenshot_skip.png"></td>
<td><img src="images/Screenshot_ssao.png"></td>
</tr>
</table>

---

## このプロジェクトについて

LearnOpenGL のサンプルコードをそのまま写経するのではなく、以下を意識して書き直しています。

- **チュートリアルの `main.cpp` 一枚岩を、責務ごとのクラスに分割する** — `Scene` / `Shader` / `Camera` / `Model` / `Mesh` / `TextureCache` など
- **RAII とスマートポインタでリソースを管理する** — 生の `new` / `delete` を使わない。VAO / VBO / テクスチャ / FBO / RBO といった GL オブジェクトも `src/GlHandle.h` のハンドル型に包み、`glDelete*` を手で並べない
- **章をまたいだ機能を1つのシーンに統合する** — 各章を独立したサンプルにせず、シャドウ・法線マッピング・HDR・Bloom・Deferred Shading が同時に動く1つのシーンとして積み上げる

GL ハンドルは CRTP（Curiously Recurring Template Pattern）で実装しています。「デストラクタから型ごとの `glDelete*` を呼ぶ」という要求が `virtual` では満たせない（基底のデストラクタからは派生の仮想関数に届かない）ためで、副産物としてオブジェクトサイズが生の `unsigned int` から増えず、`create()` が `glGen*` 1個へインライン化されます。詳細は [`docs/DEVELOPMENT.md`](./docs/DEVELOPMENT.md) の「GL リソースの持ち方」を参照してください。

そのため、章が進むごとに既存コードのリファクタリングが発生します。**動くコードを完成させること自体ではなく、なぜその設計になるのかを理解することを目的**にしています。

実装中に踏んだ落とし穴やデバッグの過程は [`docs/DEVELOPMENT.md`](./docs/DEVELOPMENT.md) にまとめています。

---

## 実装済みの機能

| 分類           | 機能                                                                        |
| -------------- | --------------------------------------------------------------------------- |
| 基礎           | カメラ操作、深度テスト、ステンシルテスト、ブレンディング、フェイスカリング  |
| ライティング   | Phong 反射モデル、点光源 / 平行光源 / スポットライト                        |
| モデル         | Assimp による OBJ 読み込み、テクスチャキャッシュ（`weak_ptr` 管理）         |
| テクスチャ     | キューブマップ（スカイボックス）、法線マッピング、視差遮蔽マッピング（POM） |
| 高度な機能     | フレームバッファ、インスタンシング、UBO によるユニフォーム共有              |
| 影             | ポイントシャドウ（キューブマップ + 26方向サンプリングの PCF）               |
| 環境光遮蔽     | SSAO（半球カーネル + 4x4 ノイズ回転 + ブラー）                              |
| 透過表現       | 半透明ガラス（前方描画 + 透過色の乗算ブレンド）、**色の付いた透過シャドウ** |
| 色管理         | sRGB テクスチャによるリニアワークフロー、ガンマ補正                         |
| ポストプロセス | HDR + 露出トーンマッピング、Bloom（2パス ガウシアンブラー）                 |
| デバッグ UI    | Dear ImGui によるパラメータ調整と G-Buffer / SSAO の可視化                  |
| **描画方式**   | **Deferred Shading（G-Buffer + ライトボリュームによる打ち切り）**           |

### 今後の予定

- PBR（物理ベースレンダリング）+ IBL — **現在作業中**
- ライトボリュームのジオメトリ描画による最適化
- G-Buffer の帯域削減（深度からの位置復元、法線の圧縮）

**未実装の項目は [`docs/plan.md`](./docs/plan.md) に集約しています。** 現在の作業計画（PBR の導入手順）と、それとは独立した長期の積み残しがそちらにあります。

---

## レンダリングパイプライン

1フレームは以下の順で処理されます。

```
[0] 透過窓の準備         カメラからの距離でソートし、インスタンス VBO を更新
[1] シャドウデプスパス   点光源4灯 × 6面のデプスキューブマップを生成
     + カラーサブパス    同じ FBO にガラスの透過色を焼き込む（色の付いた影）
[2] 行列 UBO の更新      view / projection を全シェーダーで共有する
[3] Geometry パス        G-Buffer に 位置 / 法線 / アルベド+スペキュラ を書き込む
[4] SSAO パス            G-Buffer から遮蔽率を計算し、4x4 ブラーをかける
[5] 深度のコピー         G-Buffer の深度を前方描画用のフレームバッファへ blit
[6] Lighting パス        フルスクリーンクワッド1枚で全ピクセルのライティングを計算
[7] 前方描画             ライトキューブ / スカイボックス / 半透明のガラス
[8] Bloom                輝度抽出結果をピンポン FBO で10回ブラー
[9] 合成                 トーンマッピング + ガンマ補正して画面へ
```

パス同士は FBO とテクスチャで繋がっているため、順序に意味があります（例: SSAO は G-Buffer が埋まっていないと計算できない）。各パスの設計意図と注意点は [`docs/DEVELOPMENT.md`](./docs/DEVELOPMENT.md) の「描画の流れ」を参照してください。

---

## 技術スタック

- **言語** — C++17
- **ビルド** — CMake（`CMakePresets.json`）+ Ninja
- **パッケージ管理** — vcpkg（manifest mode / `vcpkg.json`）
- **依存ライブラリ** — GLFW3, GLM, GLAD, Assimp, Dear ImGui

---

## 動作環境

### 必要なもの

| 項目           | 要件                                                                               |
| -------------- | ---------------------------------------------------------------------------------- |
| GPU / ドライバ | **OpenGL 4.6 コアプロファイル**（`src/Window.cpp` でコンテキストを要求しています） |
| コンパイラ     | C++17 対応（Windows: MSVC / Linux: GCC・Clang）                                    |
| ビルドツール   | CMake 3.20 以上 + Ninja                                                            |
| パッケージ管理 | vcpkg（manifest mode。環境変数 `VCPKG_ROOT` の設定が必要）                         |

依存ライブラリ（GLFW3 / GLM / GLAD / Assimp / Dear ImGui）は vcpkg が `vcpkg.json` を見て自動で解決するので、個別のインストールは不要です。

### 動作確認した環境

1台のノート PC 上で、Windows 側と WSL2 側の両方でビルド・実行しています。

|                    | Windows                                           | Linux (WSL2)                           |
| ------------------ | ------------------------------------------------- | -------------------------------------- |
| OS                 | Windows 11 (build 26200)                          | Ubuntu 24.04 LTS on WSL2 (kernel 6.18) |
| コンパイラ         | MSVC 19.51（Visual Studio 18.x Community）        | GCC 13.3                               |
| CMake / Ninja      | Visual Studio 同梱版                              | CMake 3.28 / Ninja 1.11                |
| CPU / GPU          | AMD Ryzen 7 7735HS + 内蔵 Radeon Graphics（iGPU） | 同じ iGPU を `d3d12` 経由で使用        |
| 使用するプリセット | `default` / `release`                             | `linux-debug`                          |

**専用 GPU は使っていません。** iGPU で 4灯のポイントシャドウ + Deferred Shading + SSAO + Bloom が30FPS動く程度の負荷です。

### WSL2 で動かす場合の注意

WSL2 はデフォルトだとソフトウェアレンダリング（llvmpipe）にフォールバックして極端に遅くなります。GPU を使うには環境変数の指定が必要です。

```bash
GALLIUM_DRIVER=d3d12 ./glfwdojo
```

環境によっては `GALLIUM_DRIVER` に別の値が必要だったり、他の環境変数を用意する必要があります。以下が参考になります。

- [Arch Wiki — OpenGL](https://wiki.archlinux.jp/index.php/OpenGL)
- [Qiita — WSL2 で OpenGL を動かす](https://qiita.com/HD_mount_Music/items/701559d57787a2f183c9)

WSL2 固有の問題（Mesa のシェーダーコンパイラが弾く書き方、d3d12 ドライバが segfault するケースなど）は [`docs/DEVELOPMENT.md`](./docs/DEVELOPMENT.md) の「Linux (WSL2) で動かす」にまとめてあります。

> **macOS は非対応です。** そもそも動作確認できていません（Macが高すぎて買えなかったので）が、仮に持っていても動きません。macOS の OpenGL は 4.1 で打ち止め（かつ 10.14 以降は deprecated）なので、本プロジェクトが要求する 4.6 コアプロファイルは原理的に取得できないためです。（OpenGL自体が古いから仕方ない）

---

## ビルドと実行

vcpkg が必要です。`VCPKG_ROOT` に vcpkg のパスを設定しておいてください。

```bash
# Windows
cmake --preset default
cmake --build --preset default
cd build/Debug && ./glfwdojo

# Linux (WSL2)
cmake --preset linux-debug
cmake --build --preset linux-debug
cd build/linux-debug && GALLIUM_DRIVER=d3d12 ./glfwdojo // 環境によっては異なる可能性があります
```

シェーダーと `resources/` は実行ファイルと同じディレクトリに相対パスで読み込まれるため、**実行時はビルドディレクトリに移動してから起動してください。**

ファイル追加時の CMake の設定方法など、詳しい手順は [`docs/BUILD.md`](./docs/BUILD.md) にあります。

> ちなみに，Linux 側はビルドがめちゃめちゃ早いです。ただし `-j` を付けたおかげではありません．
> **Ninja はデフォルトで並列ビルドします**（論理コア数 + 2）。この環境なら既定で 18 並列相当になるので，
> `-j16` はむしろ少し絞っていることになります．
>
> プリセットを使わずビルドディレクトリを直接指定するなら `cmake --build build/linux-debug` です．
> （`cmake --build linux-debug` はそのパスが存在しないので失敗します）

---

## 操作方法

| 入力           | 動作                                            |
| -------------- | ----------------------------------------------- |
| `W` / `S`      | 前進 / 後退                                     |
| `A` / `D`      | 左 / 右へ移動                                   |
| `Q` / `E`      | 下降 / 上昇                                     |
| 右ドラッグ     | 視点の回転                                      |
| マウスホイール | ズーム（FOV の変更）                            |
| `↑` / `↓`      | 視差遮蔽マッピングの深さ（`heightScale`）を調整 |
| `Esc`          | 終了                                            |

### デバッグパネル（Dear ImGui）

起動すると `Debug` ウィンドウが出ます。**再ビルドなしで**描画の中身を切り替えられるので、このプロジェクトで一番触って面白い部分です。

| 項目            | 内容                                                                                                                                                         |
| --------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `View`          | 表示するものを 9 種から選ぶ。通常のライティング / シャドウ / `shadowMap` の生値 / G-Buffer の Albedo・Normal・Position / 4分割表示 / SSAO / 透過シャドウの色 |
| `Raw output`    | Bloom・トーンマッピング・ガンマ補正を飛ばす。**G-Buffer や SSAO を見るときは必須**（切らないと正常な値でも一律に真っ白く見えて判定できない）                 |
| `SSAO strength` | 環境光遮蔽の効き具合                                                                                                                                         |
| `Ambient`       | 環境光の強さ                                                                                                                                                 |
| `Bloom`         | Bloom の合成量                                                                                                                                               |
| `Exposure`      | トーンマッピングの露出                                                                                                                                       |

パネルの上にマウスがある間は視点操作が無効になります。視点の回転が右ドラッグなので、ガードしないとスライダーを動かすたびに視界が回ってしまうためです。

---

## ディレクトリ構成

```
glfwdojo/
├── src/           C++ ソース（Scene が描画処理の中心）
├── shader_src/    GLSL シェーダー（ビルド後に実行ファイルの隣へコピーされる）
├── resources/     テクスチャ・3Dモデル
├── docs/          ドキュメント
├── third_party/   外部ライブラリ（stb_image など）
├── CMakeLists.txt
├── CMakePresets.json
└── vcpkg.json
```

シェーダーは `shader_src/` で編集しますが、実行時は `add_custom_command` でコピーされたビルドディレクトリ側が読み込まれます。**新しいシェーダーを追加したときは `CMakeLists.txt` の `SHADER_SOURCES` への追記も必要**です。

---

## ドキュメント

| ファイル                                             | 内容                                                                                            |
| ---------------------------------------------------- | ----------------------------------------------------------------------------------------------- |
| [`docs/DEVELOPMENT.md`](./docs/DEVELOPMENT.md)       | 実装ガイド。冒頭に**症状から原因を引く索引**あり。設計意図、作業手順、踏んだ罠、WSL2 固有の問題 |
| [`docs/plan.md`](./docs/plan.md)                     | 現在の作業計画と、未実装項目の一覧（**やることリストはここだけ**）                              |
| [`docs/BUILD.md`](./docs/BUILD.md)                   | ビルド手順、ファイル追加時の CMake 設定                                                         |
| [`docs/shadow_mapping.md`](./docs/shadow_mapping.md) | シャドウマッピングの学習メモ                                                                    |
| [`CLAUDE.md`](./CLAUDE.md)                           | Claude Code に作業させる際のルール                                                              |

---

## 3D モデルについて

**Assimp による 3D モデルの読み込み・描画機能は実装済みですが、このリポジトリにモデルデータは同梱していません。**

LearnOpenGL で使われている 3D モデル（backpack / cyborg / nanosuit / planet / rock）は、いずれも再配布が許諾されていない、あるいはライセンスが不明なものです。

| モデル     | 出典                                                                                                                                                     | ライセンス表記                         |
| ---------- | -------------------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------------------- |
| `backpack` | Berk Gedik（[Sketchfab](https://sketchfab.com/3d-models/survival-guitar-backpack-low-poly-799f8c4511f84fab8c3f12887f7e6b36)） / Joey de Vries により改変 | 出典表記のみ。ライセンス条項の記載なし |
| `cyborg`   | 3dregenerator（tf3dm.com） / Joey de Vries により改変                                                                                                    | **"For Personal Use Only."** と明記    |
| `planet`   | Gerhald3D（[TurboSquid](https://www.turbosquid.com/3d-models/realistic-mars-photorealistic-2k-3d-1277433)） / Joey de Vries により改変                   | 出典表記のみ。ライセンス条項の記載なし |
| `nanosuit` | Crysis（Crytek）由来                                                                                                                                     | 記載なし                               |
| `rock`     | 不明                                                                                                                                                     | 記載なし                               |

そのため、**モデル描画のコードは意図的にコメントアウトしてあります**（`src/Scene.cpp` の `Model` 生成箇所）。現在のシーンはプリミティブ（キューブ・平面）とテクスチャのみで構成されています。

### モデルを表示したい場合

1. [LearnOpenGL の公式リポジトリ](https://github.com/JoeyDeVries/LearnOpenGL) の `resources/objects/` からモデルを取得する
2. 本リポジトリの `resources/objects/` に配置する（このディレクトリは `.gitignore` 済みです）
3. `src/Scene.cpp` の `Model` 生成箇所と、`Render()` 内の描画呼び出しのコメントアウトを解除する

取得したモデルの利用は、各配布元のライセンスに従ってください。

> [!NOTE]
> テクスチャ（`resources/textures/`）にも LearnOpenGL 由来のものが含まれます。こちらは
> [LearnOpenGL のリポジトリ](https://github.com/JoeyDeVries/LearnOpenGL) の配布条件に準じます。

---

## 参考

- [LearnOpenGL](https://learnopengl.com/) — Joey de Vries
- 本リポジトリは上記チュートリアルを元にした学習用の再実装です
