
typedef enum EntityArchetype
{
	nil = 0,
	rock = 1,
	tree = 2,
	player = 3,
} EntityArchetype;

typedef struct Entity
{
	bool is_valid;
	EntityArchetype arch;
	Vector2 pos;
} Entity;

#define MAX_ENTITY_COUNT 1024

typedef struct World
{
	Entity entities[MAX_ENTITY_COUNT];
} World;
World *world = 0;

Entity *entity_create()
{
	Entity *entity_found = 0;
	for (int i = 0; i < MAX_ENTITY_COUNT; i++)
	{
		Entity *existing_entity = &world->entities[i];
		if (!existing_entity->is_valid)
		{
			entity_found = existing_entity;
			break;
		}
	}

	assert(entity_found, "No more free entities!");
	return entity_found;
}

void entity_destroy(Entity *entity)
{
	memset(entity, 0, sizeof(Entity))
}

int entry(int argc, char **argv)
{

	// This is how we (optionally) configure the window.
	// To see all the settable window properties, ctrl+f "struct Os_Window" in os_interface.c
	window.title = STR("Alchemist");
	window.clear_color = hex_to_rgba(0x211730ff);

	world = alloc(get_heap_allocator(), sizeof(World));

	Gfx_Image *player = load_image_from_disk(fixed_string("player.png"), get_heap_allocator());
	Gfx_Image *tree = load_image_from_disk(fixed_string("tree_0.png"), get_heap_allocator());
	assert(player, "No Player Image");

	Entity *player_en = entity_create();
	Entity *tree_en = entity_create();

	float64 last_time = os_get_elapsed_seconds();
	while (!window.should_close)
	{
		reset_temporary_storage();

		float zoom = 5.0;
		draw_frame.camera_xform = m4_make_scale(v3(1.0 / zoom, 1.0 / zoom, 1.0));

		float64 now = os_get_elapsed_seconds();
		float64 delta_t = now - last_time;
		last_time = now;

		if (is_key_just_pressed(KEY_ESCAPE))
		{
			window.should_close = true;
		}

		Vector2 input_axis = v2(0, 0);
		if (is_key_down('A'))
		{
			input_axis.x -= 1.0;
		}
		if (is_key_down('D'))
		{
			input_axis.x += 1.0;
		}
		if (is_key_down('S'))
		{
			input_axis.y -= 1.0;
		}
		if (is_key_down('W'))
		{
			input_axis.y += 1.0;
		}
		input_axis = v2_normalize(input_axis);

		float speed = 50.0;
		player_en->pos = v2_add(player_en->pos, v2_mulf(input_axis, speed * delta_t));

		{
			Vector2 size = v2(5.0, 8.0);
			Matrix4 xform = m4_scalar(1.0);
			xform = m4_translate(xform, v3(player_en->pos.x, player_en->pos.y, 0));
			xform = m4_translate(xform, v3(size.x * -0.5, 0.0, 0));
			draw_image_xform(player, xform, size, COLOR_WHITE);
		}

		{
			Vector2 size = v2(4.0, 6.0);
			Matrix4 xform = m4_scalar(1.0);
			xform = m4_translate(xform, v3(size.x * -0.5, 0.0, 0));
			draw_image_xform(tree, xform, size, COLOR_WHITE);
		}

		os_update();
		gfx_update();
	}

	return 0;
}