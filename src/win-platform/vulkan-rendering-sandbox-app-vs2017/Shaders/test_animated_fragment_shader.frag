#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in mat4 resultMat;

layout(location = 0) out vec4 outColor;

uniform vec2 mouse;
uniform vec2 resolution;

float random( vec2 p )
{
	return fract( sin( fract( sin( p.x ) ) + p.y) *142.17563);
}

float worley( vec2 p, float timeSpeed )
{
	float d = 10.0;
	for( int xo = -1; xo <= 1; xo++ )
	{
		for( int yo = -1; yo <= 1; yo++ )
		{
			vec2 test_cell = floor(p) + vec2( xo, yo );
			
			float f1 = random( test_cell );
			float f2 = random( test_cell + vec2(1.0,81.0) );
			float diff = resultMat[1][2];
			float xp = mix( f1, f2, sin(diff*timeSpeed) );
			float yp = mix( f1, f2, cos(diff*timeSpeed) );
			
			vec2 c = test_cell + vec2(xp,yp);
			
			vec2 cTop = p - c;
			d = min( d, dot(cTop,cTop) );
		}
	}
	return d;
}

float worley2( vec2 p )
{
	float d = 10.0;
	for( int xo = -1; xo <= 1; xo++ )
	{
		for( int yo = -1; yo <= 1; yo++ )
		{
			vec2 test_cell = floor(p) + vec2( xo, yo );
			
			vec2 c = test_cell;
			
			vec2 cTop = p - c;
			d = min( d, dot(cTop,cTop) );
		}
	}
	return d;
}

float pass( vec2 uv, float timeSpeed )
{
	float t = worley( uv / 50.0, timeSpeed );
	t = pow(t, 7.0 );
	
	return t;
}

void main() {
	vec2 uv = gl_FragCoord.xy ;

	float t = worley2( gl_FragCoord.xy / 1.6 );
	vec3 finalColor = vec3( 0,0,0);

	t = pass( uv/2.0, 0.5 );
	finalColor += vec3(sqrt(t*12.0), sqrt(t*25.0), sqrt(t * 10.0) );

	outColor = vec4(finalColor, 0.2);
}