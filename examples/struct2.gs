~output_ext .h

@template_start Linked_List <- T
typedef struct @template_name
{
    T data;
    
    @template_name *next;
} @template_name;
@template_end

@template Linked_List -> f32 -> Linked_List_F32
