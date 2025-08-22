#version 330

uniform sampler2D   sampler0;
uniform int         u_NumChannels;

in vec2             tex;
out vec4            FragColor;


void main()
{
	// RGBA
    if (u_NumChannels == 0)                     
        FragColor = texture(sampler0, tex);
	
    // RGB
    else if (u_NumChannels == 1)
        FragColor = vec4(texture(sampler0, tex).rgb, 1.0);

    // RG
    else if (u_NumChannels == 2)
        FragColor = vec4(texture(sampler0, tex).rg, 0.0, 1.0);

    // R
    else if (u_NumChannels == 3)
        FragColor = vec4(texture(sampler0, tex).r, 0.0, 0.0, 1.0);
    
    // G
      else if (u_NumChannels == 4)
          FragColor = vec4(0.0, texture(sampler0, tex).g, 0.0, 1.0);
    
    // B
    else if (u_NumChannels == 5)
          FragColor = vec4(0.0, 0.0, texture(sampler0, tex).b, 1.0);
 
     // A
    else if (u_NumChannels == 6)
          FragColor = vec4(texture(sampler0, tex).aaa, 1.0);
}
