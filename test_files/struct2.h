struct Vec3
{
    u8 x;
    u8 y;
    u8 z;

    Vec3(u8 x, u8 y, u8 z) : x(x), y(y), z(z)
    {}

    void normalize();
    f32 get_length();
};

struct Vec3
{
    f32 x;
    f32 y;
    f32 z;

    Vec3(f32 x, f32 y, f32 z) : x(x), y(y), z(z)
    {}

    void normalize();
    f32 get_length();
};

