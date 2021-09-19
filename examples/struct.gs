~output_ext .h

@template_start Vec4 <- T
typedef struct @template_name @template_name;
struct @template_name
{
	T x;
	T y;
	T z;
	T w;
};
@template_end

@template_start Vec3 <- T
typedef struct @template_name @template_name;
struct @template_name
{
	T x;
	T y;
	T z;
};
@template_end

@template_start Vec2 <- T
typedef struct @template_name @template_name;
struct @template_name
{
	T x;
	T y;
};
@template_end

@template Vec4 -> f32 -> Vec4f
@template Vec4 -> u8 -> Vec4c

@template Vec3 -> f32 -> Vec3f
@template Vec3 -> u8 -> Vec3c

@template Vec2 -> f32 -> Vec2f
@template Vec2 -> u16 -> Vec2Coords
