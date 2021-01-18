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

struct Vec4
{
	u8 x;
	u8 y;
	u8 z;
	u8 w;
	
	Vec4(u8 x, u8 y, u8 z, u8 w) : x(x), y(y), z(z), w(w) 
	{}

	void normalize();
	f32 get_length();
};

