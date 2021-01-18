@template_start Vec4 <- T
struct Vec4
{
	T x;
	T y;
	T z;
	T w;
	
	Vec4(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) 
	{}

	void normalize();
	f32 get_length();
};
@template_end

@template Vec4 -> f32
@template Vec4 -> u8
