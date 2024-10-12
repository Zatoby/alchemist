
#define SPRITE_MAX 1024

typedef struct Sprite {
    Gfx_Image *image;
    Vector2 size;
} Sprite;
typedef enum SpriteID {
    SPRITE_nil,
    SPRITE_player,
    SPRITE_tree_0,
    SPRITE_rock_0
} SpriteID;
Sprite sprites[SPRITE_MAX];
Sprite *get_sprite(SpriteID id) {
    if (id >= 0 && id < SPRITE_MAX) {
        return &sprites[id];
    }

    return &sprites[0];
}

typedef enum EntityArchetype {
    arch_nil = 0,
    arch_rock = 1,
    arch_tree = 2,
    arch_player = 3,
} EntityArchetype;

typedef struct Entity {
    bool is_valid;
    EntityArchetype arch;
    Vector2 pos;

    bool render_sprite;
    SpriteID sprite_id;
} Entity;

#define MAX_ENTITY_COUNT 1024

typedef struct World {
    Entity entities[MAX_ENTITY_COUNT];
} World;
World *world = 0;

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

void setup_player(Entity *en) {
    en->arch = arch_player;
    en->sprite_id = SPRITE_player;
}

void setup_rock(Entity *en) {
    en->arch = arch_rock;
    en->sprite_id = SPRITE_rock_0;
}

void setup_tree(Entity *en) {
    en->arch = arch_tree;
    en->sprite_id = SPRITE_tree_0;
}

int entry(int argc, char **argv) {
    window.title = STR("Alchemist");
    window.clear_color = hex_to_rgba(0x211730ff);

    world = alloc(get_heap_allocator(), sizeof(World));
    memset(world, 0, sizeof(World));

    sprites[SPRITE_player] = (Sprite){.image = load_image_from_disk(fixed_string("player.png"), get_heap_allocator()), .size = v2(5.0, 8.0)};
    sprites[SPRITE_tree_0] = (Sprite){.image = load_image_from_disk(fixed_string("tree_0.png"), get_heap_allocator()), .size = v2(6.0, 10.0)};
    sprites[SPRITE_rock_0] = (Sprite){.image = load_image_from_disk(fixed_string("rock_0.png"), get_heap_allocator()), .size = v2(4.0, 2.0)};

    Entity *player_en = entity_create();
    setup_player(player_en);
    for (int i = 0; i < 10; i++) {
        Entity *en = entity_create();
        setup_rock(en);
        en->pos = v2(get_random_float32_in_range(-200, 200), get_random_float32_in_range(-200, 200));
    }
    for (int i = 0; i < 10; i++) {
        Entity *en = entity_create();
        setup_tree(en);
        en->pos = v2(get_random_float32_in_range(-200, 200), get_random_float32_in_range(-200, 200));
    }

    // === Game Loop
    float64 last_time = os_get_elapsed_seconds();
    while (!window.should_close) {
        reset_temporary_storage();

        float zoom = 7.0;
        draw_frame.camera_xform = m4_make_scale(v3(1.0 / zoom, 1.0 / zoom, 1.0));

        float64 now = os_get_elapsed_seconds();
        float64 delta_t = now - last_time;
        last_time = now;

        // == Render Entities
        for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
            Entity *en = &world->entities[i];
            if (en->is_valid) {
                switch (en->arch) {
                    // case arch_player:
                    // break;
                    default:
                        Sprite *sprite = get_sprite(en->sprite_id);

                        Vector2 size = sprite->size;
                        Matrix4 xform = m4_scalar(1.0);
                        xform = m4_translate(xform, v3(en->pos.x, en->pos.y, 0));
                        xform = m4_translate(xform, v3(size.x * -0.5, 0.0, 0));
                        draw_image_xform(sprite->image, xform, sprite->size, COLOR_WHITE);
                        break;
                }
            }
        }

        if (is_key_just_pressed(KEY_ESCAPE)) {
            window.should_close = true;
        }

        // == Controls
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