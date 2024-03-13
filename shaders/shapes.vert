#ifdef GL_ES
precision mediump int;
precision mediump float;
#endif

attribute vec3 position;
attribute vec2 texCoord;

varying vec2 outTexCoord;

void main()
{
    gl_Position = vec4(position,1.0);
    outTexCoord = vec2(texCoord.x, 1.0 - texCoord.y);
}
