# ビルドガイド

CMake / Ninja によるビルド手順と、ファイル追加時の設定方法をまとめたものです。

> OpenGL コードの変更手順は [DEVELOPMENT.md](./DEVELOPMENT.md) を参照してください。

---

## 目次

1. [ビルドの基本ルール](#ビルドの基本ルール)
2. [新しい .cpp ファイルの追加](#新しい-cpp-ファイルの追加)
3. [新しいシェーダーファイルの追加](#新しいシェーダーファイルの追加)
4. [リソースの変更・追加](#リソースの変更追加)

---

## ビルドの基本ルール

変更内容によって必要な操作が異なります：

| 変更内容 | `cmake --build build` は必要か | 備考 |
|----------|-------------------------------|------|
| `.cpp` / `.h` を変更 | **必要** | C++ はコンパイル言語のため必ずビルドが必要 |
| `.vert` / `.frag` を変更 | **必要** | C++ のコンパイルはスキップされ、コピーのみ実行される |
| `resources/` 内の画像等を変更 | **必要** | コピーのみ実行される |
| 変更なし | 不要 | 何もしない（冪等） |

### シェーダーとリソースが自動コピーされる仕組み

`CMakeLists.txt` の `copy_shaders` ターゲット（`ALL` キーワード付き）が、
毎回のビルド呼び出しで必ず実行されます。

```
cmake --build build
 ├── glfwdojo（C++ 変更があればコンパイル・リンク、なければスキップ）
 └── copy_shaders（毎回実行）
      ├── SHADER_SOURCES のファイルを実行ディレクトリへコピー
      └── resources/ ディレクトリを実行ディレクトリへコピー
```

`copy_if_different` を使っているため、内容が変わっていないシェーダーは実際には書き込まれません。

---

## 新しい .cpp ファイルの追加

`CMakeLists.txt` の `add_executable` のソースリストに追記します：

```cmake
add_executable(glfwdojo
    src/main.cpp
    src/Scene.cpp
    src/MyNewFile.cpp  # ← 追記
    ...
)
```

追記後に `cmake --build build` を実行すると、cmake が自動で再構成してコンパイルします。

> **追記を忘れると:** そのファイルはコンパイルされず、定義した関数・クラスはリンクエラーになります。

---

## 新しいシェーダーファイルの追加

`CMakeLists.txt` の `SHADER_SOURCES` リストに追記します：

```cmake
set(SHADER_SOURCES
    ...
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/mynewshader.vert  # ← 追記
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/mynewshader.frag  # ← 追記
)
```

> **追記を忘れると:** ビルドディレクトリにコピーされないため、実行時に
> `ERROR::SHADER::FILE_SUCCESSFULY_READ` エラーが発生します。

シェーダーファイルを Scene で使うための C++ 側の手順は [DEVELOPMENT.md の「新しいシェーダーの追加」](./DEVELOPMENT.md#新しいシェーダーの追加) を参照してください。

---

## リソースの変更・追加

### 画像ファイルの追加

`resources/textures/` 以下に配置するだけでOKです。CMakeLists.txt への追記は不要です。

```
resources/
 └── textures/
      ├── 既存ファイル.png
      └── 新しいファイル.png  ← ここに置くだけ
```

次の `cmake --build build` 時に `resources/` ディレクトリ全体が自動でコピーされます。

### コードからの参照パス

```cpp
// Windows のパス区切りは \\ を使う
myTexture_ = cache_.get("resources\\textures\\新しいファイル.png", true);
```

### スカイボックス画像の差し替え

`resources/textures/skybox/` 内の 6 枚の画像を差し替えます。
ファイル名はコード側（`initTextures()` の `faces` 配列）と一致させてください：

```
right.jpg / left.jpg / top.jpg / bottom.jpg / front.jpg / back.jpg
```
