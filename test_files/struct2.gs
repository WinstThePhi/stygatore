
@template_start Vec3 <- T
struct Vec3
{
    T x;
    T y;
    T z;

    Vec3(T x, T y, T z) : x(x), y(y), z(z)
    {}

    void normalize();
    f32 get_length();
};
@template_end

@template Vec3 -> u8
@template Vec3 -> f32
