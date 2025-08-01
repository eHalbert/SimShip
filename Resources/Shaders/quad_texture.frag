#version 330

uniform sampler2D sampler0;
uniform int u_NumChannels;

in vec2 tex;
out vec4 my_FragColor0;

void main()
{
	// RGBA
    if (u_NumChannels == 0)                     
        my_FragColor0 = texture(sampler0, tex);
	
    // RGB
    else if (u_NumChannels == 1)
        my_FragColor0 = vec4(texture(sampler0, tex).rgb, 1.0);

    // RG
    else if (u_NumChannels == 2)
        my_FragColor0 = vec4(texture(sampler0, tex).rg, 0.0, 1.0);

    // R
    else if (u_NumChannels == 3)
        my_FragColor0 = vec4(texture(sampler0, tex).r, 0.0, 0.0, 1.0);
    
    // G
      else if (u_NumChannels == 4)
          my_FragColor0 = vec4(0.0, texture(sampler0, tex).g, 0.0, 1.0);
    
    // B
    else if (u_NumChannels == 5)
          my_FragColor0 = vec4(0.0, 0.0, texture(sampler0, tex).b, 1.0);
 
     // A
    else if (u_NumChannels == 6)
          my_FragColor0 = vec4(texture(sampler0, tex).aaa, 1.0);

}
