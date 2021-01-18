struct Vec4
{
	f32 x;
	f32 y;
	f32 z;
	f32 w;
	
	Vec4(f32 x, f32 y, f32 z, f32 w) : x(x), y(y), z(z), w(w) 
	{}

	void normalize();
	f32 get_length();
};
