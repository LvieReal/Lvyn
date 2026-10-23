#version 440

layout (location = 0) in vec2 vPos;

layout (location = 0) out vec4 fragColor;

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

// Pristine grid approach by Ben Golus.
// Based on: https://bgolus.medium.com/the-best-darn-grid-shader-yet-727f9278b9d8
float pristineGrid(vec2 uv, vec2 lineWidth) {
    vec2 ddxUV = dFdx(uv);
    vec2 ddyUV = dFdy(uv);
    vec2 uvDeriv = vec2(length(vec2(ddxUV.x, ddyUV.x)), length(vec2(ddxUV.y, ddyUV.y)));

    bvec2 invertLine = greaterThan(lineWidth, vec2(0.5));

    vec2 targetWidth = vec2(
        invertLine.x ? 1.0 - lineWidth.x : lineWidth.x,
        invertLine.y ? 1.0 - lineWidth.y : lineWidth.y
    );

    vec2 drawWidth = clamp(targetWidth, uvDeriv, vec2(0.5));
    vec2 lineAA = max(uvDeriv, vec2(1e-6)) * 1.5;

    vec2 gridUV = abs(fract(uv) * 2.0 - 1.0);
    gridUV = vec2(
        invertLine.x ? gridUV.x : 1.0 - gridUV.x,
        invertLine.y ? gridUV.y : 1.0 - gridUV.y
    );

    vec2 grid2 = 1.0 - smoothstep(drawWidth - lineAA, drawWidth + lineAA, gridUV);

    vec2 denom = max(drawWidth, vec2(1e-6));
    grid2 *= clamp(targetWidth / denom, 0.0, 1.0);

    grid2 = mix(grid2, targetWidth, clamp(uvDeriv * 2.0 - 1.0, 0.0, 1.0));

    grid2 = vec2(
        invertLine.x ? 1.0 - grid2.x : grid2.x,
        invertLine.y ? 1.0 - grid2.y : grid2.y
    );

    return mix(grid2.x, 1.0, grid2.y);
}

void main() {
    vec2 world = (vPos - viewportSize * 0.5) / max(zoom, 0.0001) + viewPos;

    float gridSize = max(gridParams.x, 1.0);
    float lineWidthWorld = clamp(gridParams.y, 0.0, gridSize * 0.5);
    vec2 lineWidth = vec2(lineWidthWorld / gridSize);

    float grid = pristineGrid(world / gridSize, lineWidth);

    bool inArena = (world.x >= 0.0 && world.y >= 0.0 && world.x <= arenaSize.x && world.y <= arenaSize.y);
    vec3 color = inArena ? arenaColor.rgb : backgroundColor.rgb;

    float gridA = clamp(grid * minorGridColor.a, 0.0, 1.0);
    color = mix(color, minorGridColor.rgb, gridA);

    fragColor = vec4(color, 1.0);
}
