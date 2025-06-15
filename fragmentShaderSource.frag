#version 330 core

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoord;

uniform sampler2D miniTex;
uniform sampler2D roomTex;
uniform bool isRoom;
uniform bool isbox;

uniform vec3 ambientColor;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 lightPos2;
uniform vec3 lightColor2;
uniform vec3 cameraPos;
uniform vec3 objColor;
uniform bool light1Enabled; // 第一個光源開關
uniform bool light2Enabled; // 第二個光源開關


out vec4 FragColor;

void main() {
    vec3 norm = normalize(Normal);
    vec3 cameraVec = normalize(cameraPos - FragPos);

    // 第一個光源
    vec3 diffuse1 = vec3(0.0);
    vec3 specular1 = vec3(0.0);
    if (light1Enabled) { // 只有當光源啟用時才計算
        vec3 lightDir = normalize(lightPos - FragPos);
        vec3 reflectVec = reflect(-lightDir, norm);

        float specularAmount = pow(max(dot(reflectVec, cameraVec), 0), 2);
        specular1 = specularAmount * lightColor;

        diffuse1 = max(dot(lightDir, norm), 0) * lightColor;
    }
    
    // 第二個光源
    vec3 diffuse2 = vec3(0.0);
    vec3 specular2 = vec3(0.0);
    if (light2Enabled) { // 只有當光源啟用時才計算
        vec3 lightDir2 = normalize(lightPos2 - FragPos);
        vec3 reflectVec2 = reflect(-lightDir2, norm);
        
        float specularAmount2 = pow(max(dot(reflectVec2, cameraVec), 0), 2);
        specular2 = specularAmount2 * lightColor2;

        diffuse2 = max(dot(lightDir2, norm), 0) * lightColor2;
    }

    vec3 lighting = ambientColor + (diffuse1 + specular1) + (diffuse2 + specular2);
    
    vec4 finalColor;

    if (isRoom) {
        vec2 repeated_texcoord = TexCoord * 3.0;
        vec4 texColor = texture(roomTex, repeated_texcoord);
        
        // 檢查是否為天花板（法線 Y 分量接近 1.0）
        bool isCeiling = abs(norm.y - 1.0) < 0.01;
        // 檢查是否為四面牆壁（法線 X 或 Z 分量接近 ±1.0）
        bool isWall = (abs(abs(norm.x) - 1.0) < 0.01) || (abs(abs(norm.z) - 1.0) < 0.01);

        if (isCeiling) {
            // 天花板：黃色與貼圖混合
            vec3 yellow = vec3(1.0, 1.0, 0.0);
            float ceilingMixFactor = 0.2; // 80% 貼圖 + 20% yellow
            vec3 mixedColor = mix(texColor.rgb, yellow, ceilingMixFactor);
            finalColor = vec4(mixedColor * lighting, texColor.a);
        } else if (isWall) {
            // 四面牆壁：灰色與貼圖混合
            vec3 gray = vec3(0.5, 0.5, 0.5);
            float wallMixFactor = 0.5; // 50% 貼圖 + 50% 灰色
            vec3 mixedColor = mix(texColor.rgb, gray, wallMixFactor);
            finalColor = vec4(mixedColor * lighting, texColor.a);
        } else {
            // 地板：保持原始貼圖效果
            finalColor = vec4(texColor.rgb * lighting, texColor.a);
        }
    } 
    else { //ball
        vec4 texColor = vec4(objColor, 1.0);
        finalColor = vec4(texColor.rgb * lighting, texColor.a);
    }
   
    FragColor = finalColor;
}