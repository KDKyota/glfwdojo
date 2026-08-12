#version 460 core
out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
// 1.0=遮蔽なし, 0.0=完全遮蔽
uniform sampler2D ssao;

struct PointLight {
  vec3 position;

  float constant;
  float linear;
  float quadratic;

  vec3 ambient;
  vec3 diffuse;
  vec3 specular;

  // PointLight::calcRadius() が減衰式から逆算して送ってくる
  float radius;
};

const int NR_LIGHTS = 4;
uniform PointLight pointLights[NR_LIGHTS];
uniform vec3 viewPos;
uniform samplerCube shadowMap[NR_LIGHTS];
// ガラスを透過した光の色。ガラスを通らない方向は白
uniform samplerCube shadowColor[NR_LIGHTS];
uniform float farPlane;

// ライトごとに持たせると光源から離れるほど AO を掛ける対象が消えるため定数に一本化
uniform float ambientStrength;

// 対応表は main.cpp の kDebugModes。hdr.frag の debugRawOutput も要有効
uniform int debugMode;

uniform float ssaoStrength;

// ambient は扱わない。この関数が返すのはこの光源からの直接光だけ
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir,
                    vec3 albedo, float specularStrength, float shadow);
float ShadowCalculation(vec3 fragPos, vec3 normal, vec3 lightDir, vec3 lightPos,
                        samplerCube shadowMap);

void main() {
  vec3 FragPos = texture(gPosition, TexCoords).rgb;
  vec3 Normal = texture(gNormal, TexCoords).rgb;
  vec3 Albedo = texture(gAlbedoSpec, TexCoords).rgb;
  float Specular = texture(gAlbedoSpec, TexCoords).a;

  if (debugMode == 0) {
  // ---- 通常のライティング ----
  float AmbientOcclusion =
      mix(1.0, texture(ssao, TexCoords).r, ssaoStrength);

  vec3 viewDir = normalize(viewPos - FragPos);

  // 環境光はループの外で1回だけ。距離減衰を掛けないので遠くでも AO が効く
  vec3 result = ambientStrength * Albedo * AmbientOcclusion;

  for (int i = 0; i < NR_LIGHTS; ++i) {
    // ライトボリューム: 26回サンプリングする ShadowCalculation() の手前で弾く
    float dist = length(pointLights[i].position - FragPos);
    if (dist >= pointLights[i].radius)
      continue;

    vec3 lightDir = normalize(pointLights[i].position - FragPos);
    float shadow = ShadowCalculation(FragPos, Normal, lightDir,
                                     pointLights[i].position, shadowMap[i]);
    // 窓枠は shadow≈1 で黒い影、ガラスは shadow=0 のままここで色付きに減衰する
    vec3 transmit =
        texture(shadowColor[i], FragPos - pointLights[i].position).rgb;
    result += transmit * CalcPointLight(pointLights[i], Normal, FragPos,
                                        viewDir, Albedo, Specular, shadow);
  }

  FragColor = vec4(result, 1.0);

  float brightness = dot(result, vec3(0.2126, 0.7152, 0.0722));
  if (brightness > 1.0)
    BrightColor = vec4(result, 1.0);
  else
    BrightColor = vec4(0.0, 0.0, 0.0, 1.0);

  return; // 通常描画はここで終わり。以降はデバッグ表示のみ
  }

  // ---- デバッグ表示 ----
  // Bloom が乗ると判定できなくなるので、デバッグ中は BrightColor を常に黒にする
  BrightColor = vec4(0.0, 0.0, 0.0, 1.0);

  if (debugMode == 1) {
  // 必ず1灯だけで見ること。4灯を max() でまとめるとほぼ全面が白くなり判定できない
  vec3 lightDir0 = normalize(pointLights[0].position - FragPos);
  float shadow0 = ShadowCalculation(FragPos, Normal, lightDir0,
                                    pointLights[0].position, shadowMap[0]);
  FragColor = vec4(vec3(shadow0), 1.0);

  } else if (debugMode == 2) {
  // 添字が定数なのでサンプラー配列の動的添字の問題も同時に切り分けられる
  vec3 fragToLight0 = FragPos - pointLights[0].position;
  float closest = texture(shadowMap[0], fragToLight0).r;
  FragColor = vec4(vec3(closest), 1.0);

  } else if (debugMode == 3) {
  FragColor = vec4(Albedo, 1.0);

  } else if (debugMode == 4) {
  FragColor = vec4(Normal * 0.5 + 0.5, 1.0);

  } else if (debugMode == 5) {
  FragColor = vec4(abs(FragPos) / farPlane, 1.0);

  } else if (debugMode == 6) {
  // 4分割して G-Buffer とシャドウマップを同時に見る。各象限に全体が縮小表示される
  vec2 uv = TexCoords * 2.0;
  vec2 quad = floor(uv); // (0,0)=左下 (1,0)=右下 (0,1)=左上 (1,1)=右上
  vec2 localUV = fract(uv);

  vec3 qPos = texture(gPosition, localUV).rgb;
  vec3 qNormal = texture(gNormal, localUV).rgb;
  vec4 qAlbedo = texture(gAlbedoSpec, localUV);

  vec3 debugColor;
  if (quad.y > 0.5 && quad.x < 0.5)
    debugColor = qAlbedo.rgb; // 左上: Albedo
  else if (quad.y > 0.5)
    debugColor = qNormal * 0.5 + 0.5; // 右上: Normal
  else if (quad.x < 0.5)
    debugColor = abs(qPos) / 25.0; // 左下: Position
  else {
    // 右下: shadowMap[0] の生の深度値
    vec3 dir = qPos - pointLights[0].position;
    debugColor = vec3(texture(shadowMap[0], dir).r);
  }

  if (abs(TexCoords.x - 0.5) < 0.001 || abs(TexCoords.y - 0.5) < 0.001)
    debugColor = vec3(1.0, 0.0, 0.0);

  FragColor = vec4(debugColor, 1.0);

  } else if (debugMode == 7) {
  // 真っ黒なら FBO・サンプラー・パスの配線が繋がっていない
  float ao = texture(ssao, TexCoords).r;
  FragColor = vec4(vec3(ao), 1.0);

  } else if (debugMode == 8) {
  // 白地にガラスのシルエットが色付きで写れば配線は正しい
  vec3 dir0 = FragPos - pointLights[0].position;
  FragColor = vec4(texture(shadowColor[0], dir0).rgb, 1.0);

  } else if (debugMode == 9) {
  // オブジェクトごとに一様な明るさで見えれば gNormal.a への書き込みは正しい
  float metallic = texture(gNormal, TexCoords).a;
  FragColor = vec4(vec3(metallic), 1.0);

  } else {
  FragColor = vec4(1.0, 0.0, 1.0, 1.0); // 未定義の debugMode（マゼンタ）
  }
}

// この光源からの直接光だけを返す（環境光は main() 側で一括して足している）
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir,
                    vec3 albedo, float specularStrength, float shadow) {
  vec3 lightDir = normalize(light.position - fragPos);
  float diff = max(dot(normal, lightDir), 0.0);
  vec3 diffuse = light.diffuse * diff * albedo;

  vec3 reflectDir = reflect(-lightDir, normal);
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
  vec3 specular = light.specular * spec * specularStrength;

  // 逆二乗減衰。変更したら PointLight::calcRadius() も必ず合わせる
  float distance = length(light.position - fragPos);
  float attenuation = 1.0 / (distance * distance);

  diffuse *= attenuation;
  specular *= attenuation;

  // 直接光の遮蔽はシャドウマップが担当する（SSAO ではない）
  return (1.0 - shadow) * (diffuse + specular);
}

float ShadowCalculation(vec3 fragPos, vec3 normal, vec3 lightDir, vec3 lightPos,
                        samplerCube shadowMap) {
  // samplerCube は方向でテクセルを解決するので正規化しない
  vec3 fragToLight = fragPos - lightPos;
  float currentDepth = length(fragToLight);
  // 角度に応じて可変にするスロープバイアス（shadow acne 対策）
  float bias = max(0.15 * (1.0 - dot(normal, lightDir)), 0.05);

  // PCF: 26方向サンプリング
  float shadow = 0.0;
  // UV空間ではなく方向ベクトル空間でのオフセット量なので固定値
  float offset = 0.05;


  vec3 sampleOffsetDirections[26] =
      vec3[](vec3(1, 1, 1), vec3(1, -1, 1), vec3(-1, -1, 1), vec3(-1, 1, 1),
             vec3(1, 1, -1), vec3(1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
             vec3(1, 1, 0), vec3(1, -1, 0), vec3(-1, -1, 0), vec3(-1, 1, 0),
             vec3(1, 0, 1), vec3(-1, 0, 1), vec3(1, 0, -1), vec3(-1, 0, -1),
             vec3(0, 1, 1), vec3(0, -1, 1), vec3(0, -1, -1), vec3(0, 1, -1),
             vec3(1, 0, 0), vec3(-1, 0, 0), vec3(0, 1, 0), vec3(0, -1, 0),
             vec3(0, 0, 1), vec3(0, 0, -1));
  for (int i = 0; i < 26; ++i) {
    float closestDepth =
        texture(shadowMap, fragToLight + sampleOffsetDirections[i] * offset).r;
    // [0,1] 正規化されていた値を実距離スケールに戻す
    closestDepth *= farPlane;
    if (currentDepth - bias > closestDepth)
      shadow += 1.0;
  }
  return shadow / 26.0;
}
