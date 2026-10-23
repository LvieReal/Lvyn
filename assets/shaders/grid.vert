#version 440

layout (location = 0) in vec4 qt_Vertex;

layout (location = 0) out vec2 vPos;

layout (std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    vec2 viewPos;
    float zoom;
    float pad;
    vec2 viewportSize;
    vec2 arenaSize;
    vec2 gridParams; // x=gridSize (world units), y=lineWidth (world units)
    vec4 arenaColor;
    vec4 backgroundColor;
    vec4 minorGridColor;
    vec4 majorGridColor;
};

void main() {
    vPos = qt_Vertex.xy;
    gl_Position = qt_Matrix * qt_Vertex;
}
