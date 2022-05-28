////////////////////////////////////////////////////////////////////////////////
// Filename: reflection.ps
////////////////////////////////////////////////////////////////////////////////


/////////////
// GLOBALS //
/////////////
Texture2D shaderTexture:register(t0);
SamplerState SampleType;
//We add a new texture variable for the scene reflection render to texture.
Texture2D shaderTexture1:register(t1);
Texture2D reflectionTexture:register(t2);


//////////////
// TYPEDEFS //
//////////////
struct InputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float4 reflectionPosition : TEXCOORD2;
    float height : HEIGHT;
    float4 color: COLOR;
};


////////////////////////////////////////////////////////////////////////////////
// Pixel Shader
////////////////////////////////////////////////////////////////////////////////
float4 main(InputType input) : SV_TARGET
{
    float4 textureColor;
    float2 reflectTexCoord;
    float4 reflectionColor;
    float4 color;

    if (input.height <= -1.0) {
        textureColor = shaderTexture1.Sample(SampleType, input.tex);

    }else{
        textureColor = shaderTexture.Sample(SampleType, input.tex);
    }
 //   textureColor = shaderTexture.Sample(SampleType, input.tex);

    // Sample the texture pixel at this location.
    

    //The input reflection position homogenous coordinates need to be converted to 
    //proper texture coordinates.To do so first divide by the W coordinate.This leaves us 
    //with tu and tv coordinates in the - 1 to + 1 range, to fix it to map to a 0 to + 1 range just divide by 2 and add 0.5.

    // Calculate the projected reflection texture coordinates.
    reflectTexCoord.x = input.reflectionPosition.x / input.reflectionPosition.w / 2.0f + 0.5f;
    reflectTexCoord.y = -input.reflectionPosition.y / input.reflectionPosition.w / 2.0f + 0.5f;


    //Now when we sample from the reflection texture we use the projected reflection coordinates 
    //that have been converted to get the right reflection pixel for this projected reflection position.
    // Sample the texture pixel from the reflection texture using the projected texture coordinates.

    reflectionColor = reflectionTexture.Sample(SampleType, reflectTexCoord);

    if (input.height <= -1.0) {
        reflectionColor = lerp(reflectionColor, input.color, 0.5f);
    }

    //Finally we blend the texture from the floor with the reflection texture to create the effect of 
    //the reflected cube on the floor.Here we use a linear interpolation between the two textures with 
    //a factor of 0.15.You can change this to a different blend equation or change the factor amount for a different or stronger effect.
    // Do a linear interpolation between the two textures for a blend effect.


//    textureColor = lerp(textureColor, input.color, 0.5f);
   


    color = lerp(textureColor, reflectionColor, 0.5f);

    return color;
}
