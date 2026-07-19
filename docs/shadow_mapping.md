# シャドウマッピング 変数リファレンス & 計算フロー

このドキュメントは、シャドウ生成に関わる主要な変数の役割・計算方法・使われ方を整理したものです。
現在このリポジトリで **実際にレンダリングに使われているのは Point Shadow（キューブマップ版）** です。
`shadow_mapping_depth.vert` / `shadow_mapping_depth.frag`（ディレクショナルライト用の2D shadow map版）は
`Scene.cpp` からは呼び出されておらず、比較用の参考実装として残っています。

---

## 1. Point Shadow Mapping（現在使用中）

点光源は全方向を照らすため、光源を中心とした立方体の6面（キューブマップ）に深度を焼き込み、
フラグメントごとに「光源からその点への方向ベクトル」でキューブマップをサンプリングして影を判定します。

### 1.1 全体の2パス構成

```mermaid
flowchart TD
    subgraph Pass1["Pass 1: デプスパス (depthMapFBO_ にレンダリング)"]
        A["Scene::Render()<br/>lightPos = pointLights[0].position"] --> B["shadowProj = glm::perspective(90°, 1:1, near, far)"]
        B --> C["shadowTransforms[0..5] =<br/>shadowProj * lookAt(lightPos, ±X/±Y/±Z, up)"]
        C --> D["pointDepthShader_ に<br/>shadowMatrices[6], farPlane, lightPos を送る"]
        D --> E["point_shadow_depth.vert<br/>ワールド座標のまま gl_Position へ"]
        E --> F["point_shadow_depth.geom<br/>面ごとに shadowMatrices[face] を掛けて<br/>gl_Layer で出力先の面を選択"]
        F --> G["point_shadow_depth.frag<br/>gl_FragDepth = length(FragPos-lightPos)/farPlane"]
        G --> H[("depthCubemap_<br/>各テクセル=光源からの正規化距離[0,1]")]
    end

    subgraph Pass2["Pass 2: メインパス (framebuffer_ にレンダリング)"]
        I["shader.frag / wall.frag<br/>ShadowCalculation(fragPos, normal, lightDir)"] --> J["fragToLight = fragPos - lightPos"]
        J --> K["currentDepth = length(fragToLight)"]
        K --> L["depthCubemap_ を fragToLight 方向でサンプル<br/>closestDepth = texture(shadowMap, dir).r * farPlane"]
        L --> M{"currentDepth - bias<br/> > closestDepth ?"}
        M -->|Yes| N["影 (shadow=1)"]
        M -->|No| O["影なし (shadow=0)"]
    end

    H -.depthCubemap_をシェーダーにバインド.-> L
```

### 1.2 変数リファレンス

#### C++側 (`Scene.h` / `Scene.cpp`)

| 変数 | 宣言場所 | 役割 |
|---|---|---|
| `SHADOW_WIDTH`, `SHADOW_HEIGHT` | `Scene.h` | `depthCubemap_` の各面の解像度（1024×1024）。大きいほど影の輪郭が精細になるが、GPU負荷とメモリが増える |
| `shadowNearPlane_` | `Scene.h` | 光源視点の透視投影の near plane |
| `shadowFarPlane_` | `Scene.h` | 光源視点の透視投影の far plane。**シェーダー側 `farPlane` uniform と必ず同じ値にする必要がある**（正規化・逆正規化の基準が食い違うと影が壊れる） |
| `depthMapFBO_` | `Scene.h` | 深度専用フレームバッファ。カラーバッファは持たず `depthCubemap_` のみをアタッチ |
| `depthCubemap_` | `Scene.h` | 6面ぶんの `GL_DEPTH_COMPONENT` キューブマップ。各テクセルには「光源からの正規化距離」が入る（色ではない） |
| `lightPos`（`Render()`内） | `Scene.cpp` | シャドウを落とす点光源のワールド座標。6方向すべての `lookAt` の視点原点になる |
| `shadowProj`（`Render()`内） | `Scene.cpp` | 立方体の1面をちょうど覆う画角90°の透視投影行列。6面で共通利用 |
| `shadowTransforms`（`Render()`内） | `Scene.cpp` | `shadowProj * lookAt(...)` を+X,-X,+Y,-Y,+Z,-Zの6方向ぶん計算した `view*projection` 行列の配列。`point_shadow_depth.geom` の `shadowMatrices[6]` にそのまま渡される |

#### シェーダー側（デプスパス）

| 変数                  | 宣言場所                                | 役割                                                                                                |
| ------------------- | ----------------------------------- | ------------------------------------------------------------------------------------------------- |
| `shadowMatrices[6]` | `point_shadow_depth.geom`           | C++側の `shadowTransforms` を受け取る uniform 配列                                                         |
| `gl_Layer`          | `point_shadow_depth.geom`           | 出力先を `depthCubemap_` の6面のうちどれにするか選ぶビルトイン変数                                                        |
| `FragPos`（out/in）   | `point_shadow_depth.geom` → `.frag` | ラスタライズされるフラグメントのワールド座標（透視除算前）。frag側で光源との距離計算に使う                                                   |
| `lightPos`          | `point_shadow_depth.frag`           | 光源のワールド座標                                                                                         |
| `farPlane`          | `point_shadow_depth.frag`           | 距離を `[0,1]` に正規化するための除数                                                                           |
| `gl_FragDepth`      | `point_shadow_depth.frag`           | 書き込み先。`length(FragPos - lightPos) / farPlane`（**投影のz値ではなく実距離**を使うのがポイント。面の継ぎ目で深度が不連続にならないようにするため） |

#### シェーダー側（メインパス `ShadowCalculation`, `shader.frag` / `wall.frag`）

| 変数 | 役割 |
|---|---|
| `shadowMap`（`samplerCube`） | `depthCubemap_` がバインドされる。UV座標ではなく**方向ベクトル**でサンプリングする点が2D版との最大の違い |
| `farPlane` | `shadowMap` から読んだ正規化距離を実距離スケールに戻すための係数 |
| `fragToLight` | `fragPos - lightPos`。正規化せずそのまま `samplerCube` のサンプリング方向として使う |
| `currentDepth` | `length(fragToLight)`。光源から現在のフラグメントまでの実距離 |
| `bias` | 法線とライト方向のなす角に応じて可変にするスロープバイアス。shadow acne（縞模様のノイズ）対策 |
| `closestDepth` | `fragToLight` 方向にある「光源から見た最も近い遮蔽物までの距離」。`shadowMap` から読んだ値に `farPlane` を掛けて実距離に戻したもの |
| `sampleOffsetDirections[26]` | `{-1,0,1}^3` から中心`(0,0,0)`を除いた26方向（立方体の頂点8+辺の中点12+面の中心6）。PCF（ソフトシャドウ）のために `fragToLight` を少しずつ散らす。サンプル数を増やすほど `shadow` が取り得る段階数（現在27段階: 0/26〜26/26）が増え、影の縁のグラデーションが細かくなる |
| `offset` | `sampleOffsetDirections` に掛ける係数。2D版の `texelSize` に相当する役割だが、UV空間ではなく方向ベクトル空間のオフセット量なので固定値になっている |

---

## 2. （参考・未使用）Directional Shadow Mapping — 2D shadow map版

`shadow_mapping_depth.vert/.frag` は、ディレクショナルライトのような「1方向だけを照らす光源」用の実装です。
現在の `Scene.cpp` では呼び出されていませんが、Point Shadow との違いを理解するための比較対象として整理します。

### 2.1 計算フロー

```mermaid
flowchart TD
    A["lightSpaceMatrix = lightProjection * lightView<br/>（光源を1台のカメラと見なした view*projection）"] --> B["shadow_mapping_depth.vert<br/>gl_Position = lightSpaceMatrix * model * vec4(aPos,1)"]
    B --> C["OpenGLが自動でgl_Position.z/wから<br/>正規化深度[0,1]をdepth mapに書き込む"]
    C --> D[("shadowMap (sampler2D)<br/>1枚の2Dテクスチャ")]

    E["ShadowCalculation(fragPosLightSpace)"] --> F["projCoords = xyz / w（透視除算）"]
    F --> G["projCoords = projCoords*0.5+0.5<br/>（NDC[-1,1] → UV[0,1]へリマップ）"]
    G --> H["closestDepth = texture(shadowMap, projCoords.xy).r"]
    H --> I["currentDepth = projCoords.z"]
    I --> J{"currentDepth-bias > closestDepth ?"}
    D -.shadowMapをバインド.-> H
```

### 2.2 Point Shadow 版との対応比較

| 観点 | Directional（2D shadow map） | Point Shadow（cubemap） |
|---|---|---|
| テクスチャ種別 | `sampler2D` 1枚 | `samplerCube` 6面 |
| 光源視点変換 | `lightSpaceMatrix`（CPU側で1つだけ計算） | `shadowMatrices[6]`（6方向ぶん計算し、geometry shaderで1回のdraw callで全面に描画） |
| サンプリング座標 | `projCoords.xy`（透視除算＋`*0.5+0.5`のリマップが必要） | `fragToLight`（方向ベクトルをそのまま渡すだけ） |
| 深度の保存形式 | 投影されたNDCのz（`gl_Position.z/w`、OpenGLが自動書き込み） | 光源からの実距離を`farPlane`で正規化した線形値（`gl_FragDepth`に明示的に書き込み） |
| PCFのオフセット | `texelSize = 1.0/textureSize(shadowMap,0)` でUV空間を1テクセル分ずらす | `sampleOffsetDirections[20] * offset` で3D方向ベクトル自体を散らす |
| フラスタム外の扱い | `if(projCoords.z > 1.0) shadow = 0.0;` で明示的にガード | 全方向をcubemapでカバーしているため同種のガードは基本的に不要 |

### 2.3 `textureSize` / `texelSize` について

- `textureSize(sampler, lod)`: 指定したmipmapレベルにおけるテクスチャの実解像度を返すGLSL組み込み関数。CPU側からuniformで解像度を渡さなくても、シェーダー自身が参照中のテクスチャサイズを取得できる。
- `texelSize = 1.0 / textureSize(shadowMap, 0)`: UV空間（`[0,1]`）で「1テクセル分だけ動くのに必要なオフセット量」。PCFのループで `projCoords.xy + vec2(x,y) * texelSize` とすることで、シャドウマップの解像度によらず常に隣接テクセルの中心を正しくサンプリングできる。
- Point Shadow版では方向ベクトルをサンプリングに使うため、この「UV空間でのテクセルサイズ」という概念がそのままでは使えず、固定値 `offset = 0.05` で代用している（`samplerCube` に対しても `textureSize` は呼び出せるが、現在の実装では使われていない）。

---

## 3. 用語ミニ解説

- **shadow acne**: `bias` が小さすぎるときに、影を落とす面自身に縞模様のノイズが出る現象。深度値の丸め誤差が原因。
- **PCF (Percentage Closer Filtering)**: 影判定を1点だけでなく周辺複数点で行い、結果を平均することで影の輪郭をぼかす手法。
- **shadow acneとpeter panningのトレードオフ**: `bias` を大きくしすぎると、今度は物体が地面から浮いて見える「peter panning」が起きる。`bias` の値は両者のバランスを見て調整する。
