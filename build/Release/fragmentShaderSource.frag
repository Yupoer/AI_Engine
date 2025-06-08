#version 400 core

in vec2 TexCoord;
in vec3 Normal;     // 如果 isRoom 分支不使用光照，Normal 和 FragPos 對它就不是必需的
in vec3 FragPos;

out vec4 FragColor;

uniform sampler2D roomTex;
// uniform sampler2D boxTex;
uniform bool isRoom;
uniform bool isbox;
uniform bool isAABB;
uniform vec3 objColor;       // 用於 'else' 分支的物件
uniform vec3 ambientColor;   // 用於 'else' 分支的光照
uniform vec3 lightPos;       // 用於 'else' 分支的光照
uniform vec3 lightColor;     // 用於 'else' 分支的光照
uniform vec3 lightPos2;      // 用於 'else' 分支的光照
uniform vec3 lightColor2;    // 用於 'else' 分支的光照
uniform vec3 cameraPos;      // 用於 'else' 分支的光照
uniform bool light1Enabled;  // 用於 'else' 分支的光照
uniform bool light2Enabled;  // 用於 'else' 分支的光照

void main()
{
    if (isAABB) {
        FragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f); // AABB 框線設為紅色
    }
    else if (isRoom) {
        vec4 texColor = texture(roomTex, TexCoord);    // 從 roomTex 採樣紋理顏色
        vec3 white_rgb = vec3(1.0f, 1.0f, 1.0f);     // 純白色 (RGB部分)
        float MixFactor = 0.2;                       // 20% 的白色混合進來 (即 80% 紋理 + 20% 白色)
                                                     // mix(A, B, factor) = A*(1-factor) + B*factor
                                                     // 所以 mix(texColor.rgb, white_rgb, 0.2) = texColor.rgb * 0.8 + white_rgb * 0.2
        vec3 mixedColor = mix(texColor.rgb, white_rgb, MixFactor);

        // 如果房間不受光照影響，直接輸出混合顏色
        FragColor = vec4(mixedColor, texColor.a);

        // 如果你想讓房間也受一點點整體亮度的影響，可以這樣 (但不是完整的Phong光照):
        // float roomBrightness = 1.0; // 可以是一個 uniform 或者固定值
        // FragColor = vec4(mixedColor * roomBrightness, texColor.a);
        // FragColor = clamp(FragColor, 0.0, 1.0); // 確保不超過範圍

    }
    else { // 其他物件 (球、不規則物體) - 這裡有完整的光照計算
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(lightPos - FragPos);
        vec3 lightDir2 = normalize(lightPos2 - FragPos);
        vec3 viewDir = normalize(cameraPos - FragPos);

        // 環境光
        vec3 ambient = ambientColor * objColor;

        // 漫反射
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * lightColor * objColor;

        // 第二個光源的漫反射
        float diff2 = max(dot(norm, lightDir2), 0.0);
        vec3 diffuse2 = diff2 * lightColor2 * objColor;

        // 鏡面反射
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
        vec3 specular = spec * lightColor;

        // 第二個光源的鏡面反射
        vec3 reflectDir2 = reflect(-lightDir2, norm);
        float spec2 = pow(max(dot(viewDir, reflectDir2), 0.0), 32.0);
        vec3 specular2 = spec2 * lightColor2;

        // 根據光源開關狀態計算最終顏色
        vec3 result = ambient;
        if (light1Enabled) {
            result += diffuse + specular;
        }
        if (light2Enabled) {
            result += diffuse2 + specular2;
        }

        FragColor = vec4(result, 1.0);
    }
}