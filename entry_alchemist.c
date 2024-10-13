
#include "range.c"

const int tile_width = 8;
const float entity_selection_radius = 16.0f;

const int rock_health = 5;
const int tree_health = 3;

int world_pos_to_tile_pos(float world_pos) {
    return roundf(world_pos / (float)tile_width);
}
float tile_pos_to_world_pos(int tile_pos) {
    return ((float)tile_pos * (float)tile_width);
}
Vector2 round_v2_to_tile(Vector2 world_pos) {
    world_pos.x = tile_pos_to_world_pos(world_pos_to_tile_pos(world_pos.x));
    world_pos.y = tile_pos_to_world_pos(world_pos_to_tile_pos(world_pos.y));
    return world_pos;
}

// :util

float sin_breathe(float time, float rate) {
    return (sin(time * rate) + 1) / 2;
}

// :totarget
bool almost_equals(float a, float b, float epsilon) {
    return fabs(a - b) <= epsilon;
}
bool animate_f32_to_target(float *value, float target, float delta_t, float rate) {
    *value += (target - *value) * (1.0 - pow(2.0f, -rate * delta_t));
    if (almost_equals(*value, target, 0.001f)) {
        *value = target;
        return true;  // reached
    }
    return false;
}
void animate_v2_to_target(Vector2 *value, Vector2 target, float delta_t, float rate) {
    animate_f32_to_target(&(value->x), target.x, delta_t, rate);
    animate_f32_to_target(&(value->y), target.y, delta_t, rate);
}

Vector2 screen_to_world() {
    float mouse_x = input_frame.mouse_x;
    float mouse_y = input_frame.mouse_y;
    Matrix4 proj = draw_frame.projection;
    Matrix4 view = draw_frame.camera_xform;
    float window_w = window.width;
    float window_h = window.height;

    // Normalize the mouse coordinates
    float ndc_x = (mouse_x / (window_w * 0.5f)) - 1.0f;
    float ndc_y = (mouse_y / (window_h * 0.5f)) - 1.0f;

    // Transform to world coordinates
    Vector4 world_pos = v4(ndc_x, ndc_y, 0, 1);
    world_pos = m4_transform(m4_inverse(proj), world_pos);
    world_pos = m4_transform(view, world_pos);

    // log("%f, %f", world_pos.x, world_pos.y);
    // Return as 2D vector
    return (Vector2){world_pos.x, world_pos.y};
}
// scuffed to_worldspace i guess... works tho
// float whalf = window.pixel_width / 2.0;
// float hhalf = window.pixel_height / 2.0;
// Vector3 pos = v3((input_frame.mouse_x - whalf) / zoom, (input_frame.mouse_y - hhalf) / zoom, 0);
// pos = v3(pos.x + camera_pos.x, pos.y + camera_pos.y, 0);
// log("%f, %f", pos.x, pos.y);

#define SPRITE_MAX 1024
typedef struct Sprite {
    Gfx_Image *image;
} Sprite;
typedef enum SpriteID {
    SPRITE_nil,
    SPRITE_player,
    SPRITE_tree_pine,
    SPRITE_rock_0,
    SPRITE_item_rock,
    SPRITE_item_pine_wood
} SpriteID;
Sprite sprites[SPRITE_MAX];
Sprite *get_sprite(SpriteID id) {
    if (id >= 0 && id < SPRITE_MAX) {
        return &sprites[id];
    }

    return &sprites[0];
}

Vector2 get_sprite_size(Sprite *sprite) {
    return v2(sprite->image->width, sprite->image->height);
}

// Item items[ITEM_MAX];
// Item *get_item(ItemID id) {
//     if (id >= 0 && id < ITEM_MAX) {
//         return &items[id];
//     }
//     return &items[0];
// }

// :entity
#define MAX_ENTITY_COUNT 1024
typedef enum EntityArchetype {
    arch_nil = 0,
    arch_rock = 1,
    arch_tree_pine = 2,
    arch_player = 3,

    arch_item_rock = 4,
    arch_item_pine_wood = 5,
} EntityArchetype;
typedef struct Entity {
    bool is_valid;
    EntityArchetype arch;
    Vector2 pos;
    int health;
    bool destroyable_world_item;
    bool is_item;

    bool render_sprite;
    SpriteID sprite_id;
} Entity;

typedef struct World {
    Entity entities[MAX_ENTITY_COUNT];
} World;
World *world = 0;

typedef struct WorldFrame {
    Entity *selected_entity;
} WorldFrame;
WorldFrame world_frame;

Entity *entity_create() {
    Entity *entity_found = 0;
    for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
        Entity *existing_entity = &world->entities[i];
        if (!existing_entity->is_valid) {
            entity_found = existing_entity;
            break;
        }
    }

    assert(entity_found, "No more free entities!");
    entity_found->is_valid = true;
    return entity_found;
}

void entity_destroy(Entity *entity) {
    memset(entity, 0, sizeof(Entity));
}

// :setups

void setup_player(Entity *en) {
    en->arch = arch_player;
    en->sprite_id = SPRITE_player;
    en->destroyable_world_item = false;
}

void setup_rock(Entity *en) {
    en->arch = arch_rock;
    en->sprite_id = SPRITE_rock_0;
    en->health = rock_health;
    en->destroyable_world_item = true;
}

void setup_tree(Entity *en) {
    en->arch = arch_tree_pine;
    en->sprite_id = SPRITE_tree_pine;
    en->health = tree_health;
    en->destroyable_world_item = true;
}

void setup_item_pine_wood(Entity *en) {
    en->arch = arch_item_pine_wood;
    en->sprite_id = SPRITE_item_pine_wood;
    en->destroyable_world_item = false;
    en->is_item = true;
}

int entry(int argc, char **argv) {
    window.title = STR("Alchemist");
    window.clear_color = hex_to_rgba(0x211730ff);

    world = alloc(get_heap_allocator(), sizeof(World));
    memset(world, 0, sizeof(World));

    // :text
    Gfx_Font *font = load_font_from_disk(STR("C:/windows/fonts/arial.ttf"), get_heap_allocator());
    assert(font, "Failed loading arial.ttf");
    const u32 font_height = 48;

    sprites[SPRITE_player] = (Sprite){.image = load_image_from_disk(fixed_string("res/sprites/player.png"), get_heap_allocator())};
    sprites[SPRITE_tree_pine] = (Sprite){.image = load_image_from_disk(fixed_string("res/sprites/tree_pine.png"), get_heap_allocator())};
    sprites[SPRITE_rock_0] = (Sprite){.image = load_image_from_disk(fixed_string("res/sprites/rock_0.png"), get_heap_allocator())};
    sprites[SPRITE_item_pine_wood] = (Sprite){.image = load_image_from_disk(fixed_string("res/sprites/item_pinewood.png"), get_heap_allocator())};

    Entity *player_en = entity_create();
    setup_player(player_en);
    for (int i = 0; i < 10; i++) {
        Entity *en = entity_create();
        setup_rock(en);
        en->pos = v2(get_random_float32_in_range(-200, 200), get_random_float32_in_range(-200, 200));
        en->pos = round_v2_to_tile(en->pos);
        // en->pos.y -= tile_width * 0.5;
    }
    for (int i = 0; i < 10; i++) {
        Entity *en = entity_create();
        setup_tree(en);
        en->pos = v2(get_random_float32_in_range(-200, 200), get_random_float32_in_range(-200, 200));
        en->pos = round_v2_to_tile(en->pos);
        // en->pos.y -= tile_width * 0.5;
    }

    float zoom = 7.0;
    Vector2 camera_pos = v2(0, 0);

    // === Game Loop
    float64 last_time = os_get_elapsed_seconds();
    while (!window.should_close) {
        reset_temporary_storage();
        world_frame = (WorldFrame){0};

        float64 now = os_get_elapsed_seconds();
        float64 delta_t = now - last_time;
        last_time = now;

        // :camera
        {
            Vector2 target_pos = player_en->pos;
            animate_v2_to_target(&camera_pos, target_pos, delta_t, 15.0f);
            // camera_pos = player_en->pos;

            draw_frame.camera_xform = m4_make_scale(v3(1.0, 1.0, 1.0));
            draw_frame.camera_xform = m4_mul(draw_frame.camera_xform, m4_make_translation(v3(camera_pos.x, camera_pos.y, 0)));
            draw_frame.camera_xform = m4_mul(draw_frame.camera_xform, m4_make_scale(v3(1.0 / zoom, 1.0 / zoom, 1.0)));
        }

        Vector2 mouse_pos_world = screen_to_world();
        int mouse_tile_x = world_pos_to_tile_pos(mouse_pos_world.x);
        int mouse_tile_y = world_pos_to_tile_pos(mouse_pos_world.y);

        // :select
        {
            // log("%f, %f", input_frame.mouse_x, input_frame.mouse_y);
            // draw_text(font, tprint("%f %f", mouse_pos_world.x, mouse_pos_world.y), font_height, mouse_pos_world, v2(0.1, 0.1), COLOR_WHITE);

            float smallest_dist = INFINITY;

            for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
                Entity *en = &world->entities[i];
                if (en->is_valid && en->destroyable_world_item) {
                    int entity_tile_x = world_pos_to_tile_pos(en->pos.x);
                    int entity_tile_y = world_pos_to_tile_pos(en->pos.y);

                    float dist = fabs(v2_length(v2_sub(en->pos, mouse_pos_world)));

                    if (dist < entity_selection_radius) {
                        if (!world_frame.selected_entity || dist < smallest_dist) {
                            world_frame.selected_entity = en;
                            smallest_dist = dist;
                        }
                    }
                }
            }
        }

        // :tile rendering
        {
            int player_tile_x = world_pos_to_tile_pos(player_en->pos.x);
            int player_tile_y = world_pos_to_tile_pos(player_en->pos.y);
            int tile_radius_x = 40;
            int tile_radius_y = 30;
            for (int x = player_tile_x - tile_radius_x; x < player_tile_x + tile_radius_x; x++) {
                for (int y = player_tile_y - tile_radius_y; y < player_tile_y + tile_radius_y; y++) {
                    if ((x + (y % 2 == 0)) % 2 == 0) {
                        Vector4 col = v4(0.2, 0.2, 0.2, 0.2);
                        float x_pos = x * tile_width;
                        float y_pos = y * tile_width;
                        draw_rect(v2(x_pos + tile_width * -0.5, y_pos + tile_width * -0.5), v2(tile_width, tile_width), col);
                    }
                }
            }
        }

        // :render
        for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
            Entity *en = &world->entities[i];
            if (en->is_valid) {
                switch (en->arch) {
                    // case arch_player:
                    // break;
                    default:
                        Sprite *sprite = get_sprite(en->sprite_id);
                        Matrix4 xform = m4_scalar(1.0);

                        if (en->is_item) {
                            xform = m4_translate(xform, v3(0, 2.0 * sin_breathe(os_get_elapsed_seconds(), 4), 0));
                        }

                        Vector2 size = get_sprite_size(sprite);
                        xform = m4_translate(xform, v3(en->pos.x, en->pos.y, 0));
                        xform = m4_translate(xform, v3(0.0, tile_width * -0.5, 0));
                        xform = m4_translate(xform, v3(sprite->image->width * -0.5, 0.0, 0));

                        Vector4 col = COLOR_WHITE;
                        if (world_frame.selected_entity == en) {
                            col = COLOR_RED;
                        }

                        draw_image_xform(sprite->image, xform, get_sprite_size(sprite), col);
                        break;
                }
            }
        }

        if (is_key_just_pressed(KEY_ESCAPE)) {
            window.should_close = true;
        }

        {
            Entity *selected_en = world_frame.selected_entity;

            if (is_key_just_pressed(MOUSE_BUTTON_LEFT) && selected_en) {
                consume_key_just_pressed(MOUSE_BUTTON_LEFT);  // -> consuming key, so that if we call it later on it won't hit. i.e UI
                selected_en->health -= 1;
                if (selected_en->health <= 0) {
                    switch (selected_en->arch) {
                        case arch_tree_pine: {
                            {
                                Entity *en = entity_create();
                                setup_item_pine_wood(en);
                                en->pos = selected_en->pos;
                            }
                            // spawn
                        } break;
                        case arch_rock: {
                            // spawn
                        } break;
                        default: {
                        } break;
                    }

                    entity_destroy(selected_en);
                }
            }
        }

        // :controls
        Vector2 input_axis = v2(0, 0);
        if (is_key_down('A')) {
            input_axis.x -= 1.0;
        }
        if (is_key_down('D')) {
            input_axis.x += 1.0;
        }
        if (is_key_down('S')) {
            input_axis.y -= 1.0;
        }
        if (is_key_down('W')) {
            input_axis.y += 1.0;
        }
        input_axis = v2_normalize(input_axis);

        float speed = 50.0;
        player_en->pos = v2_add(player_en->pos, v2_mulf(input_axis, speed * delta_t));

        {
            // Vector2 size = v2(5.0, 8.0);
            Matrix4 xform = m4_scalar(1.0);
            xform = m4_translate(xform, v3(player_en->pos.x, player_en->pos.y, 0));
            // xform = m4_translate(xform, v3(size.x * -0.5, 0.0, 0));
        }

        os_update();
        gfx_update();
    }

    return 0;
}