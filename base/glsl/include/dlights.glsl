layout( std140 ) uniform u_DynamicLight {
	int u_NumDynamicLights;
};

uniform isamplerBuffer u_DynamicLightTiles;
uniform samplerBuffer u_DynamicLightData;

void applyDynamicLights( int count, int tile_index, mat3 invTBN, vec3 position, vec3 normal, vec3 wo, inout vec3 diffuselight, inout vec3 specularlight ) {

	vec3 tangent, bitangent;

	for( int i = 0; i < count; i++ ) {
		int idx = tile_index * 50 + i; // NOTE(msc): 50 = MAX_DLIGHTS_PER_TILE
		int dlight_index = texelFetch( u_DynamicLightTiles, idx ).x;

		vec4 data = texelFetch( u_DynamicLightData, dlight_index );
		vec3 origin = floor( data.xyz );
		vec3 dlight_color = fract( data.xyz ) / 0.9;
		float radius = data.w;

		float intensity = DLIGHT_CUTOFF * radius * radius;
		float dist = distance( position, origin );
		vec3 lightdir = normalize( position - origin - normal ); // - normal to prevent 0,0,0

		float dlight_intensity = max( 0.0, intensity / ( dist * dist ) - DLIGHT_CUTOFF );

		vec3 L = -lightdir;
		vec3 wi = invTBN * L;

		float kS = SATURATE(u_Ks);
		float kD = 1.0 - kS;

		diffuselight += dlight_color * dlight_intensity * Lambert(wi);
		specularlight += dlight_color * max(0.0, 1.0-dist/radius) * EricHeitz2018GGX(wo, wi, u_MaterialColor.rgb, u_Metallic, u_Roughness, u_Anisotropic, u_IOR);
		
		//lambertlight += dlight_color * dlight_intensity * max( 0.0, LambertLight( normal, -lightdir ) );
		//specularlight += dlight_color * max( 0.0, 1.0 - dist / radius ) * SpecularLight( normal, lightdir, viewDir, u_Shininess ) * u_Specular;
	}
}
