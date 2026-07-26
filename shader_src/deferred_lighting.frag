#version 330 core
out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;
  
in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;

struct PointLight {
	vec3 position;

	float constant;
	float linear;
	float quadratic;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

const int NR_LIGHTS = 4;
uniform PointLight pointLights[NR_LIGHTS];
uniform vec3 viewPos;
uniform samplerCube shadowMap[4];
uniform float farPlane;

// functions
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 albedo, float specularStrength, float shadow);
float ShadowCalculation(vec3 fragPos, vec3 normal, vec3 lightDir, vec3 lightPos, samplerCube shadowMap);

void main()
{             
    // retrieve data from G-buffer
    vec3 FragPos = texture(gPosition, TexCoords).rgb;
    vec3 Normal = texture(gNormal, TexCoords).rgb;
    vec3 Albedo = texture(gAlbedoSpec, TexCoords).rgb;
    float Specular = texture(gAlbedoSpec, TexCoords).a;
    
    // then calculate lighting as usual
    //vec3 lighting = Albedo * 0.1; // hard-coded ambient component
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 result = vec3(0.0);
    for(int i = 0; i < NR_LIGHTS; ++i)
    {
        // diffuse
        vec3 lightDir = normalize(pointLights[i].position - FragPos);
		float shadow = ShadowCalculation(FragPos, Normal, lightDir, pointLights[i].position, shadowMap[i]);
        result += CalcPointLight(pointLights[i], Normal, FragPos, viewDir, Albedo, Specular, shadow);

    }
    
    FragColor = vec4(result, 1.0);

    // reflect bright color
	float brightness = dot(result, vec3(0.2126, 0.7152, 0.0722));
	if(brightness > 1.0)
		BrightColor = vec4(result, 1.0);
	else
		BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}  

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 albedo, float specularStrength, float shadow)
{
	vec3 lightDir = normalize(light.position - fragPos);
	// Diffuse
	float diff = max(dot(normal, lightDir), 0.0);
	//vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TexCoords));
	vec3 diffuse = light.diffuse * diff * albedo;
	// Specular
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
	//vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));
	vec3 specular = light.specular * spec * specularStrength;

	// Combine results
	//vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords));
	vec3 ambient = light.ambient * albedo;

	// attenuation
	float distance = length(light.position - fragPos);
	float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

	ambient *= attenuation;
	diffuse *= attenuation; // 今のレンガcubeにはdiffuseとspecularは小さいほうが自然
	specular *= attenuation;
	// 返し値にshadowを考慮
	return (ambient + (1.0f - shadow) * (diffuse + specular));
}

float ShadowCalculation(vec3 fragPos, vec3 normal, vec3 lightDir, vec3 lightPos, samplerCube shadowMap)
{
    // 光源からフラグメントへの方向ベクトル。samplerCube はUV座標ではなく
    // この「方向」でどの面のどのテクセルかを解決するため、正規化せずそのまま使う
    vec3 fragToLight = fragPos - lightPos;
    // 光源からフラグメントまでの実距離（比較の基準値。shadowMap側の値と同じスケールにする）
    float currentDepth = length(fragToLight);
    // 法線とライト方向の角度に応じて可変にするスロープバイアス（shadow acne 対策）
    float bias = max(0.15 * (1.0 - dot(normal, lightDir)), 0.05);

    // PCF: 26方向サンプリング
    float shadow = 0.0;
    // sampleOffsetDirections を掛ける係数。directional版のtexelSizeに相当するが、
    // UV空間ではなく方向ベクトル空間でのオフセット量なので固定値になっている
    float offset = 0.05;
    // {-1,0,1}^3 から中心(0,0,0)を除いた26方向すべて
    // （立方体の頂点8+辺の中点12+面の中心6。方向ベクトルを少しずつ散らして周辺テクセルをサンプリングし、
    //   影の縁をぼかす。サンプル数を増やすほど shadow/26.0 が取り得る段階数が増え、グラデーションが細かくなる）
    vec3 sampleOffsetDirections[26] = vec3[]
    (
        vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1),
        vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
        vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
        vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
        vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1),
        vec3( 1,  0,  0), vec3(-1,  0,  0), vec3( 0,  1,  0), vec3( 0, -1,  0),
        vec3( 0,  0,  1), vec3( 0,  0, -1)
    );
    for(int i = 0; i < 26; ++i)
    {
        // 方向ベクトルを少し揺らしてサンプリングした、光源から見た「最も近い遮蔽物」までの距離
        float closestDepth = texture(shadowMap, fragToLight + sampleOffsetDirections[i] * offset).r;
        // [0,1] 正規化されていた値を farPlane 倍して実距離スケールに戻す
        closestDepth *= farPlane;
        if(currentDepth - bias > closestDepth)
            shadow += 1.0;
    }
    return shadow / 26.0;
}
