#version 330 core

// Outputs colors in RGBA
out vec4 FragColor;
out vec4 FragColor2;

// Imports the current position from the Vertex Shader
in vec3 crntPos;
// Imports the normal from the Vertex Shader
in vec3 Normal;
// Imports the color from the Vertex Shader
in vec3 color;
// Imports the texture coordinates from the Vertex Shader
in vec2 texCoord;

// Gets the Texture Units from the main function
uniform sampler2D diffuse0;
uniform sampler2D specular0;
// Gets the color of the light from the main function
uniform vec4 lightColor;
uniform vec4 lightColor2;
// Gets the position of the light from the main function
uniform vec3 lightPos;
uniform vec3 lightPos2;
uniform vec3 lampPos;
// Gets the position of the camera from the main function
uniform vec3 camPos;
// Gets the direction of the spotlight from the main function
uniform vec3 spotDirection;
uniform vec3 spotDirection2;
// Gets the position of the lamp from the main function
uniform float time;
uniform int isOn; // 0 = off, 1 = on


bool inYellowRoom(vec3 pos)
{
    // First rectangle: from (-5, *, 1) to (9, *, 6)
    bool inRect1 = (pos.x >= -5.0 && pos.x <= 9.0) && (pos.z >= 1.0 && pos.z <= 6.0);
    // Second rectangle: from (-5, *, -9) to (0, *, 6)
    bool inRect2 = (pos.x >= -5.0 && pos.x <= 0.0) && (pos.z >= -9.0 && pos.z <= 6.0);
    return inRect1 || inRect2;
}

// Simple hash function for pseudo-randomness
float rand(float n) {
    return fract(sin(n) * 43758.5453123);
}

bool inLamp(vec3 pos)
{
    bool inRect1 = (pos.x >= -2.15 && pos.x <= -1.32) && (pos.y >= 3.5 && pos.y <= 3.8) && (pos.z >= 2.5 && pos.z <= 3.09);
    return inRect1;
}

vec4 pointLight()
{	
	// used in two variables so I calculate it here to not have to do it twice
	vec3 lightVec = lightPos2 - crntPos;

	// intensity of light with respect to distance
	float dist = length(lightVec);
	float a = 3.0;
	float b = 0.7;
	float inten = 1.0f / (a * dist * dist + b * dist + 1.0f);

	// ambient lighting
	float ambient = 0.20f;

	// diffuse lighting
	vec3 normal = normalize(Normal);
	vec3 lightDirection = normalize(lightVec);
	float diffuse = max(dot(normal, lightDirection), 0.0f);

	// specular lighting
	float specularLight = 0.50f;
	vec3 viewDirection = normalize(camPos - crntPos);
	vec3 reflectionDirection = reflect(-lightDirection, normal);
	float specAmount = pow(max(dot(viewDirection, reflectionDirection), 0.0f), 4);
	float specular = specAmount * specularLight;

	return (texture(diffuse0, texCoord) * (diffuse * inten + ambient) + texture(specular0, texCoord).r * specular * inten) * lightColor2;
}

vec4 direcLight()
{
	// ambient lighting
	float ambient = 0.20f;

	// diffuse lighting
	vec3 normal = normalize(Normal);
	vec3 lightDirection = normalize(vec3(1.0f, 1.0f, 0.0f));
	float diffuse = max(dot(normal, lightDirection), 0.0f);

	// specular lighting
	float specularLight = 0.50f;
	vec3 viewDirection = normalize(camPos - crntPos);
	vec3 reflectionDirection = reflect(-lightDirection, normal);
	float specAmount = pow(max(dot(viewDirection, reflectionDirection), 0.0f), 16);
	float specular = specAmount * specularLight;

	return (texture(diffuse0, texCoord) * (diffuse + ambient) + texture(specular0, texCoord).r * specular) * lightColor;
}

vec4 spotLight()
{
	// controls how big the area that is lit up is
	float outerCone = 0.94f;
	float innerCone = 0.95f;

	// ambient lighting
	float ambient = 0.05f;

	// diffuse lighting
	vec3 normal = normalize(Normal);
	vec3 lightDirection = normalize(lightPos - crntPos);
	float diffuse = max(dot(normal, lightDirection), 0.0f);

	// specular lighting
	float specularLight = 0.50f;
	vec3 viewDirection = normalize(camPos - crntPos);
	vec3 reflectionDirection = reflect(-lightDirection, normal);
	float specAmount = pow(max(dot(viewDirection, reflectionDirection), 0.0f), 16);
	float specular = specAmount * specularLight;

	// calculates the intensity of the crntPos based on its angle to the center of the light cone
	float angle = dot(normalize(spotDirection), -lightDirection);
	float inten = clamp((angle - outerCone) / (innerCone - outerCone), 0.0f, 1.0f);

	return (texture(diffuse0, texCoord) * (diffuse * inten + ambient) + texture(specular0, texCoord).r * specular * inten) * lightColor;
}

vec4 yellowSpotLight(float amb)
{
    // Lamp position and boost parameters
    float lampRadius = 1.5; // How far the lamp's boost reaches
    float lampBoost = 2.5;  // How much brighter at the center

    // Spotlight cone parameters
    float outerCone = 0.9f;
    float innerCone = 0.8f;

    vec3 lightVec = lightPos2 - crntPos;
    float dist = length(lightVec);
    float a = 3.0;
    float b = 0.7;
    float inten = 1.0f / (a * dist * dist + b * dist + 1.0f);

    // Lamp boost based on distance to lampPos
    float lampDist = length(crntPos - lampPos);
    float lampFactor = 1.0 + lampBoost * clamp(1.0 - lampDist / lampRadius, 0.0, 1.0);

    // Spotlight cone calculation
    vec3 lightDirection = normalize(lightVec);
    float angle = dot(normalize(spotDirection2), -lightDirection);
    float coneIntensity = clamp((angle - outerCone) / (innerCone - outerCone), 0.0f, 1.0f);

    // ambient lighting
    float ambient = amb;

    // diffuse lighting
    vec3 normal = normalize(Normal);
    float diffuse = max(dot(normal, lightDirection), 0.0f);

    // specular lighting
    float specularLight = 0.50f;
    vec3 viewDirection = normalize(camPos - crntPos);
    vec3 reflectionDirection = reflect(-lightDirection, normal);
    float specAmount = pow(max(dot(viewDirection, reflectionDirection), 0.0f), 16);
    float specular = specAmount * specularLight;

    return (texture(diffuse0, texCoord) * (diffuse * inten * coneIntensity * lampFactor + ambient) +
            texture(specular0, texCoord).r * specular * inten * coneIntensity * lampFactor) * lightColor2;
}

void main()
{
    FragColor = spotLight()*isOn;

    // Flicker: randomly enable/disable yellow light based on time
    float flickerSeed = floor(time * 5.0); // Flicker speed (increase for faster flicker)
    if (rand(flickerSeed) > 0.3) {
        if (inYellowRoom(crntPos)) {
            FragColor += yellowSpotLight(0.20);
        }
        if (inLamp(crntPos)) {
            FragColor += yellowSpotLight(0.60);
        }
    }
}