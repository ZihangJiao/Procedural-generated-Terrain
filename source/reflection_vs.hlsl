cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
};
//add a new constant buffer to hold the reflecton matrix

cbuffer ReflectionBuffer
{
    matrix reflectionMatrix;
    float4 colour;
};

//////////////
// TYPEDEFS //
//////////////
struct InputType
{
    float4 position : POSITION;
    float2 tex : TEXCOORD0;
};
// The PixleInputType now has a 4 float texture coordinate variable called reflectionPosition 
//that will be used to hold the projected reflection texture input position.

struct OutputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float4 reflectionPosition : TEXCOORD2;
    float height : HEIGHT;
    float4 color: COLOR;
};


////////////////////////////////////////////////////////////////////////////////
// Vertex Shader
////////////////////////////////////////////////////////////////////////////////
OutputType main(InputType input)
{
    OutputType output;
    matrix reflectProjectWorld;

    output.height = input.position.y;

    // Change the position vector to be 4 units for proper matrix calculations.
    //input.position.w = 1.0f;

    // Calculate the position of the vertex against the world, view, and projection matrices.
    output.position = mul(input.position, worldMatrix);
    output.position = mul(output.position, viewMatrix);
    output.position = mul(output.position, projectionMatrix);

    // Store the texture coordinates for the pixel shader.
    //output.tex = input.tex;
    /*The first change to the vertex shader is that we create a matrix for transforming the input position
    values into the projected reflection position.This matrix is a combination of the reflection matrix,
    the projection matrix, and the world matrix.*/

    // Create the projection world matrix.
    reflectProjectWorld = mul(reflectionMatrix, projectionMatrix);

    reflectProjectWorld = mul(worldMatrix, reflectProjectWorld);
    /*Now transform the input position into the projected reflection position.These transformed position
    coordinates will be used in the pixel shader to derive where to map our projected reflection texture to.*/

    // Calculate the input position against the reflectProjectWorld matrix.
    //output.reflectionPosition = mul(input.position, reflectProjectWorld);
    output.reflectionPosition = mul(input.position, reflectProjectWorld);

 
    output.tex = input.tex;
    
    output.color = colour;
    

    return output;
}