@template_start Vec4 <- T
typedef struct @template_name
{
	T x;
	T y;
	T z;
	T w;
} @template_name;
@template_end

@template_start Vec3 <- T
typedef struct @template_name
{
	T x;
	T y;
	T z;
} @template_name;
@template_end

@template Vec3 -> f32 -> Vec3f
@template Vec3 -> u8 -> Vec3c
@template Vec4 -> f32 -> Vec4f
@template Vec4 -> u8 -> Vec4c
