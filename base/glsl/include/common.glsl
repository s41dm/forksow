#define M_PI 3.14159265358979323846

float sRGBToLinear( float srgb ) {
	if( srgb <= 0.04045 )
		return srgb * ( 1.0 / 12.92 );
	return pow( ( srgb + 0.055 ) * ( 1.0 / 1.055 ), 2.4 );
}

vec3 sRGBToLinear( vec3 srgb ) {
	return vec3( sRGBToLinear( srgb.r ), sRGBToLinear( srgb.g ), sRGBToLinear( srgb.b ) );
}

vec4 sRGBToLinear( vec4 srgb ) {
	return vec4( sRGBToLinear( srgb.r ), sRGBToLinear( srgb.g ), sRGBToLinear( srgb.b ), srgb.a );
}

float LinearTosRGB( float linear ) {
	if( linear <= 0.0031308 )
		return linear * 12.92;
	return 1.055 * pow( linear, 1.0 / 2.4 ) - 0.055;
}

vec3 LinearTosRGB( vec3 linear ) {
	return vec3( LinearTosRGB( linear.r ), LinearTosRGB( linear.g ), LinearTosRGB( linear.b ) );
}

vec4 LinearTosRGB( vec4 linear ) {
	return vec4( LinearTosRGB( linear.r ), LinearTosRGB( linear.g ), LinearTosRGB( linear.b ), linear.a );
}

float LinearizeDepth( float ndc ) {
	return u_NearClip / ( 1.0 - ndc );
}

vec3 NormalToRGB( vec3 normal ) {
	return normal * 0.5 + 0.5;
}

float Unlerp( float lo, float x, float hi ) {
	return ( x - lo ) / ( hi - lo );
}

// must match the CPU OrthonormalBasis
void OrthonormalBasis( vec3 v, out vec3 tangent, out vec3 bitangent ) {
	float s = step( 0.0, v.z ) * 2.0 - 1.0;
	float a = -1.0 / ( s + v.z );
	float b = v.x * v.y * a;

	tangent = vec3( 1.0 + s * v.x * v.x * a, s * b, -s * v.x );
	bitangent = vec3( b, s + v.y * v.y * a, -v.y );
}


#define SATURATE(x) clamp(x,0.0,1.0)

float Square(float x) { return x * x; }

float EricHeitz2018GGXG1Lambda(vec3 V, float alpha_x, float alpha_y)
{
	float Vx2 = Square(V.x);
	float Vy2 = Square(V.y);
	float Vz2 = Square(V.z);
	float ax2 = Square(alpha_x);
	float ay2 = Square(alpha_y);
	return (-1.0 + sqrt(1.0 + (Vx2 * ax2 + Vy2 * ay2) / Vz2)) / 2.0;
}

float EricHeitz2018GGXG1(vec3 V, float alpha_x, float alpha_y)
{
	return 1.0 / (1.0 + EricHeitz2018GGXG1Lambda(V, alpha_x, alpha_y));
}

// wm: microfacet normal in frame
float EricHeitz2018GGXD(in vec3 N, in float alpha_x, in float alpha_y)
{
	float Nx2 = Square(N.x);
	float Ny2 = Square(N.y);
	float Nz2 = Square(N.z);
	float ax2 = Square(alpha_x);
	float ay2 = Square(alpha_y);
	return 1.0 / (M_PI * alpha_x * alpha_y * Square(Nx2 / ax2 + Ny2 / ay2 + Nz2));
}


float EricHeitz2018GGXG2(in vec3 V, in vec3 L, in float alpha_x, in float alpha_y)
{
	return EricHeitz2018GGXG1(V, alpha_x, alpha_y) * EricHeitz2018GGXG1(L, alpha_x, alpha_y);
}

vec3 SchlickFresnel(in float NoX, in vec3 F0)
{
	return F0 + (1.0 - F0) * pow(1.0 - NoX, 5.0);
}


float Theta(in vec3 w) { return acos(w.z / length(w)); }
float CosTheta(in vec3 w) { return cos(Theta(w)); }

vec3 EricHeitz2018GGX(in vec3 wo, in vec3 wi, in vec3 albedo, in float metallic, in float roughness, in float anisotropic, in float ior) {
	float alpha = roughness * roughness;
    float aspect = sqrt(1.0 - 0.9 * anisotropic);
    float alpha_x = alpha * aspect;
    float alpha_y = alpha / aspect;

	float NoV = CosTheta(wo);
	float NoL = CosTheta(wi);

	//if (NoV <= 0.0 || NoL <= 0.0) return vec3(0.0);

	vec3 wm = normalize(wo + wi);
	float cosT = SATURATE(CosTheta(wo));//wo.z;

	vec3 F0 = vec3(abs((1.0 - ior)/(1.0 + ior)));
	F0 *= F0;
	F0 = mix(F0, albedo, metallic);

	float mask = step(0.0, NoL) * step(0.0, CosTheta(wm));

	vec3 F = SchlickFresnel(cosT, F0);
	float D = EricHeitz2018GGXD(wm, alpha_x, alpha_y);
	float G2 = EricHeitz2018GGXG2(wo, wi, alpha_x, alpha_y);

	//return vec3(F*D*G2);

	float denom = SATURATE( 4.0 * (NoV * /*SATURATE(CosTheta(wm))*/ SATURATE(CosTheta(wm)) + 0.05) );

	return (F * D * G2 * mask) / denom;// / (4.0 * max(NoL,0.0) * max(NoL,0.0) + 0.04);
}

// expect tangent space light vector
float HalfLambert(vec3 v) {
	float ill = max(CosTheta(v),0.0);
	return pow(ill,2.0)*0.5+0.5;
}

float Lambert(vec3 v) {
	return max(CosTheta(v),0.0);
}
