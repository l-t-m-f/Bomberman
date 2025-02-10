#include "SDL3/SDL.h"
#include "randombytes.h"

#include "pluto.h"

#include "input_man.h"
#include "log.h"
#include "m-dict.h"
#include "render_target.h"

#define CELL_SIZE 32
#define MAP_WIDTH 15
#define MAP_HEIGHT 15

Sint32 DEBUG_LOG = DEBUG_LOG_NONE;

typedef struct singleton_game
{
  ecs_entity_t player;
  mat2d_entity_t cells;
  ecs_entity_t current_scene;
  dict_string_to_query_ptr_t queries;
  SDL_Point control_delta;
} game_s;

typedef struct component_cell_data
{
  bool b_is_blocked;
} cell_data_c;

typedef struct component_lifetime
{
  Uint32 duration;
  void (*on_delete_callback) (ecs_world_t *world, ecs_entity_t ent);
} lifetime_c;

ECS_COMPONENT_DECLARE (game_s);

ECS_COMPONENT_DECLARE (cell_data_c);
ECS_COMPONENT_DECLARE (lifetime_c);

static void
cell_data (void *ptr, Sint32 count, const ecs_type_info_t *type_info)
{
  cell_data_c *cell_data = ptr;
  for (Sint32 i = 0; i < count; i++)
    {
      cell_data[i].b_is_blocked = false;
    }
}

static void
lifetime (void *ptr, Sint32 count, const ecs_type_info_t *type_info)
{
  lifetime_c *lifetime = ptr;
  for (Sint32 i = 0; i < count; i++)
    {
      lifetime->duration = 500u;
      lifetime->on_delete_callback = NULL;
    }
}

static bool
can_character_move (ecs_world_t *world, ecs_entity_t ent, SDL_Point dir)
{
  const game_s *game = ecs_singleton_get (world, game_s);
  const index_c *index = ecs_get (world, ent, index_c);
  ecs_entity_t cell = *arr_entity_get (
      *mat2d_entity_get (game->cells, index->y + dir.y), index->x + dir.x);
  const cell_data_c *cell_data = ecs_get (world, cell, cell_data_c);
  return !cell_data->b_is_blocked;
}

static void
try_move_character (ecs_world_t *world, ecs_entity_t ent, SDL_Point dir)
{
  if (can_character_move (world, ent, dir) == true)
    {
      movement_c *movement = ecs_get_mut (world, ent, movement_c);

      movement->delta.x = dir.x;
      movement->delta.y = dir.y;

      ecs_modified (world, ent, movement_c);
    }
}

void
detonate_bomb (ecs_world_t *world, ecs_entity_t ent)
{
  ecs_entity_t pfb = ecs_lookup (world, "explosion_pfb");
  const game_s *game = ecs_singleton_get (world, game_s);

  const index_c *index_b = ecs_get (world, ent, index_c);

  for (Sint32 j = -2; j < 3; j++)
    {
      for (Sint32 i = -2; i < 3; i++)
        {
          SDL_Point potential_spawn
              = (SDL_Point){ .x = index_b->x + i, .y = index_b->y + j };
          if (potential_spawn.x < 0 || potential_spawn.x >= MAP_WIDTH
              || potential_spawn.y < 0 || potential_spawn.y >= MAP_HEIGHT)
            {
              continue;
            }
          if (i != 0 && j != 0)
            {
              continue;
            }
          ecs_entity_t cell = *arr_entity_get (
              *mat2d_entity_get (game->cells, potential_spawn.y),
              potential_spawn.x);

          const cell_data_c *cell_data = ecs_get (world, cell, cell_data_c);
          if (cell_data->b_is_blocked == false)
            {
              ecs_entity_t new = ecs_new_w_pair (world, EcsIsA, pfb);
              index_c *index = ecs_ensure (world, new, index_c);
              index->x = potential_spawn.x;
              index->y = potential_spawn.y;
              ecs_modified (world, new, index_c);
            }
        }
    }
}

void
handle_key_press (struct input_man *input_man, SDL_Scancode key, void *param)
{
  const bool b_has_shift_mod
      = *arr_bool_get (input_man->keyboard.key_flips, SDL_SCANCODE_LSHIFT)
        || *arr_bool_get (input_man->keyboard.key_flips, SDL_SCANCODE_RSHIFT);

  ecs_world_t *world = param;
  game_s *game = ecs_get_mut (world, ecs_id (game_s), game_s);
  if (key == SDL_SCANCODE_F)
    {
      core_s *core = ecs_get_mut (world, ecs_id (core_s), core_s);
      core->b_is_fullscreen_presentation = !core->b_is_fullscreen_presentation;
      SDL_SetWindowFullscreen (core->win, core->b_is_fullscreen_presentation);
    }
  if (key == SDL_SCANCODE_G)
    {
      if (b_has_shift_mod == true)
        {
          ecs_iter_t it = ecs_query_iter (
              world, *dict_string_to_query_ptr_get (
                         game->queries, STRING_CTE ("get_all_rocks")));
          while (ecs_query_next (&it))
            {
              for (Sint32 i = 0; i < it.count; i++)
                {
                  box_c *box = ecs_get_mut (world, it.entities[i], box_c);
                  box->b_is_shown = !box->b_is_shown;
                  ecs_modified (world, it.entities[i], box_c);
                  cache_c *cache
                      = ecs_get_mut (world, it.entities[i], cache_c);
                  cache->b_should_regenerate = true;
                  ecs_modified (world, it.entities[i], cache_c);
                }
            }
        }
    }
  if (key == SDL_SCANCODE_W)
    {
      game->control_delta.y = -1;
    }
  if (key == SDL_SCANCODE_A)
    {
      game->control_delta.x = -1;
    }
  if (key == SDL_SCANCODE_S)
    {
      game->control_delta.y = 1;
    }
  if (key == SDL_SCANCODE_D)
    {
      game->control_delta.x = 1;
    }
  if (key == SDL_SCANCODE_SPACE)
    {
      {
        ecs_entity_t pfb = ecs_lookup (world, "bomb_pfb");
        ecs_entity_t ent = ecs_new_w_pair (world, EcsIsA, pfb);
        const index_c *index_p = ecs_get (world, game->player, index_c);
        index_c *index = ecs_get_mut (world, ent, index_c);
        index->x = index_p->x;
        index->y = index_p->y;
        ecs_modified (world, ent, index_c);
      }
    }
}

void
handle_key_release (struct input_man *input_man, SDL_Scancode key, void *param)
{
}

void
handle_key_hold (struct input_man *input_man, SDL_Scancode key, void *param)
{
  ecs_world_t *world = param;
  game_s *game = ecs_get_mut (world, ecs_id(game_s), game_s);
  if (key == SDL_SCANCODE_W)
    {
      game->control_delta.y = -1;
    }
  if (key == SDL_SCANCODE_A)
    {
      game->control_delta.x = -1;
    }
  if (key == SDL_SCANCODE_S)
    {
      game->control_delta.y = 1;
    }
  if (key == SDL_SCANCODE_D)
    {
      game->control_delta.x = 1;
    }
  if (key == SDL_SCANCODE_SPACE)
    {
      {
        ecs_entity_t pfb = ecs_lookup (world, "bomb_pfb");
        ecs_entity_t ent = ecs_new_w_pair (world, EcsIsA, pfb);
        const index_c *index_p = ecs_get (world, game->player, index_c);
        index_c *index = ecs_get_mut (world, ent, index_c);
        index->x = index_p->x;
        index->y = index_p->y;
        ecs_modified (world, ent, index_c);
      }
    }
}
void
handle_mouse_press (struct input_man *input_man, SDL_FPoint pos, Uint8 button,
                    void *param)
{
}
void
handle_mouse_release (struct input_man *input_man, SDL_FPoint pos,
                      Uint8 button, void *param)
{
}
void
handle_mouse_hold (struct input_man *input_man, SDL_FPoint pos, Uint8 button,
                   void *param)
{
}

void
handle_mouse_motion (struct input_man *input_man, SDL_FPoint pos,
                     SDL_FPoint rel, void *param)
{
}

SDL_FPoint
get_relative_from_index (ecs_entity_t ent, ecs_world_t *ecs)
{
  const index_c *index = ecs_get (ecs, ent, index_c);
  SDL_FPoint result = { .x = index->x * CELL_SIZE, .y = index->y * CELL_SIZE };
  return result;
}

static void
system_lifetime_progress (ecs_iter_t *it)
{
  lifetime_c *lifetime = ecs_field (it, lifetime_c, 0);

  for (Sint32 i = 0; i < it->count; i++)
    {
      if (lifetime[i].duration > 0)
        {
          lifetime[i].duration--;
        }
      else
        {
          if (lifetime[i].on_delete_callback != NULL)
            {
              lifetime[i].on_delete_callback (it->world, it->entities[i]);
            }
          ecs_delete (it->world, it->entities[i]);
        }
    }
}

static void
create_bombers (ecs_world_t *world)
{
  game_s *game = ecs_get_mut (world, ecs_id (game_s), game_s);
  {
    ecs_entity_t pfb = ecs_lookup (world, "grid_character_pfb");
    ecs_entity_t ent = ecs_new_w_pair (world, EcsIsA, pfb);
    ecs_set_name (world, ent, "bomber0");
    index_c *index = ecs_get_mut (world, ent, index_c);
    index->x = 5;
    index->y = 1;
    sprite_c *sprite = ecs_get_mut (world, ent, sprite_c);
    string_set_str (sprite->name, "T_Sprite_Bomber0.png");
    game->player = ent;
  }
}

static void
create_map (ecs_world_t *world)
{
  game_s *game = ecs_get_mut (world, ecs_id (game_s), game_s);
  for (Sint32 j = 0; j < MAP_HEIGHT; j++)
    {
      arr_entity_t row;
      arr_entity_init (row);

      for (Sint32 i = 0; i < MAP_WIDTH; i++)
        {
          ecs_entity_t cell = 0u;
          {
            ecs_entity_t pfb = ecs_lookup (world, "grid_cell_pfb");
            cell = ecs_new_w_pair (world, EcsIsA, pfb);
            string_t temp;
            string_init_printf (temp, "cell_%d_%d", i, j);
            ecs_set_name (world, cell, string_get_cstr (temp));
            index_c *index = ecs_get_mut (world, cell, index_c);
            index->x = i;
            index->y = j;
            arr_entity_push_back (row, cell);
          }
          {
            ecs_entity_t pfb = ecs_lookup (world, "floor_pfb");
            ecs_entity_t ent = ecs_new_w_pair (world, EcsIsA, pfb);
            string_t temp;
            string_init_printf (temp, "floor_%d_%d", i, j);
            ecs_set_name (world, ent, string_get_cstr (temp));
            index_c *index = ecs_get_mut (world, ent, index_c);
            index->x = i;
            index->y = j;
          }

          array_c *array = ecs_get_mut (world, cell, array_c);
          cell_data_c *cell_data = ecs_get_mut (world, cell, cell_data_c);

          if (i != 0 && j != 0 && i != MAP_WIDTH - 1 && j != MAP_HEIGHT - 1)
            {
              Uint8 buf;
              randombytes (&buf, sizeof (Uint8));
              if (buf < 40u)
                {
                  {
                    ecs_entity_t pfb = ecs_lookup (world, "rock_pfb");
                    ecs_entity_t ent = ecs_new_w_pair (world, EcsIsA, pfb);
                    string_t temp;
                    string_init_printf (temp, "rock_%d_%d", i, j);
                    ecs_set_name (world, ent, string_get_cstr (temp));
                    index_c *index = ecs_get_mut (world, ent, index_c);
                    index->x = i;
                    index->y = j;
                    cell_data->b_is_blocked = true;
                    arr_entity_push_back (array->content, ent);
                  }
                }
              continue;
            }
          {
            ecs_entity_t pfb = ecs_lookup (world, "wall_pfb");
            ecs_entity_t ent = ecs_new_w_pair (world, EcsIsA, pfb);
            string_t temp;
            string_init_printf (temp, "wall_%d_%d", i, j);
            ecs_set_name (world, ent, string_get_cstr (temp));
            index_c *index = ecs_get_mut (world, ent, index_c);
            index->x = i;
            index->y = j;
            cell_data->b_is_blocked = true;
            arr_entity_push_back (array->content, ent);
          }
        }
      mat2d_entity_push_back (game->cells, row);
    }
  {
    ecs_entity_t ent = ecs_entity (world, { .name = "static" });
    bounds_c *bounds = ecs_ensure (world, ent, bounds_c);
    bounds->size = (SDL_FPoint){ 480.f, 480.f };
    box_c *box = ecs_ensure (world, ent, box_c);
    box->b_is_shown = false;
    box->b_uses_color = true;
    color_c *color = ecs_ensure (world, ent, color_c);
    color->default_r = 0u;
    color->default_g = 0u;
    color->default_b = 0u;
    layer_c *layer = ecs_ensure (world, ent, layer_c);
    layer->value = 0;
    origin_c *origin = ecs_ensure (world, ent, origin_c);
    render_target_c *render_target = ecs_ensure (world, ent, render_target_c);
    string_set_str (render_target->name, "RT_static");
    visibility_c *visibility = ecs_ensure (world, ent, visibility_c);
    visibility->b_state = true;
  }
}

static void
init_game_prefabs (ecs_world_t *world)
{
  {
    ecs_entity_t ent = ecs_entity (
        world, { .name = "grid_cell_pfb", .add = ecs_ids (EcsPrefab) });
    array_c *array = ecs_ensure (world, ent, array_c);
    cell_data_c *cell_data = ecs_ensure (world, ent, cell_data_c);
    index_c *index = ecs_ensure (world, ent, index_c);
  }
  {
    ecs_entity_t ent = ecs_entity (
        world, { .name = "grid_element_pfb", .add = ecs_ids (EcsPrefab) });
    index_c *index = ecs_ensure (world, ent, index_c);
    origin_c *origin = ecs_ensure (world, ent, origin_c);
    origin->relative_callback = get_relative_from_index;
  }
  {
    ecs_entity_t pfb = ecs_lookup (world, "grid_element_pfb");
    ecs_entity_t ent
        = ecs_entity (world, { .name = "grid_object_pfb",
                               .add = ecs_ids (EcsPrefab, ecs_isa (pfb)) });
    bounds_c *bounds = ecs_ensure (world, ent, bounds_c);
    bounds->size = (SDL_FPoint){ CELL_SIZE, CELL_SIZE };
    box_c *box = ecs_ensure (world, ent, box_c);
    box->b_is_shown = false;
    box->b_uses_color = true;
    color_c *color = ecs_ensure (world, ent, color_c);
    color->default_r = 0u;
    color->default_g = 155u;
    color->default_b = 0u;
    layer_c *layer = ecs_ensure (world, ent, layer_c);
    layer->value = 1;
    sprite_c *sprite = ecs_ensure (world, ent, sprite_c);
    visibility_c *visibility = ecs_ensure (world, ent, visibility_c);
    visibility->b_state = true;
  }
  {
    ecs_entity_t pfb = ecs_lookup (world, "grid_object_pfb");
    ecs_entity_t ent
        = ecs_entity (world, { .name = "grid_character_pfb",
                               .add = ecs_ids (EcsPrefab, ecs_isa (pfb)) });
    //    anim_player_c *anim_player = ecs_ensure (ecs, ent, anim_player_c);
    layer_c *layer = ecs_get_mut (world, ent, layer_c);
    layer->value = 2;
    movement_c *movement = ecs_ensure (world, ent, movement_c);
    movement->default_cooldown = 10u;
  }
  {
    ecs_entity_t pfb = ecs_lookup (world, "grid_object_pfb");
    ecs_entity_t ent
        = ecs_entity (world, { .name = "floor_pfb",
                               .add = ecs_ids (EcsPrefab, ecs_isa (pfb)) });
    cache_c *cache = ecs_ensure (world, ent, cache_c);
    string_set_str (cache->cache_name, "RT_static");
    color_c *color = ecs_ensure (world, ent, color_c);
    color->default_r = 125u;
    color->default_g = 0u;
    color->default_b = 125u;
    sprite_c *sprite = ecs_get_mut (world, ent, sprite_c);
    sprite->b_uses_color = true;
    string_set_str (sprite->name, "T_Sprite_Floor0.png");
  }
  {
    ecs_entity_t pfb = ecs_lookup (world, "grid_object_pfb");
    ecs_entity_t ent
        = ecs_entity (world, { .name = "rock_pfb",
                               .add = ecs_ids (EcsPrefab, ecs_isa (pfb)) });
    cache_c *cache = ecs_ensure (world, ent, cache_c);
    string_set_str (cache->cache_name, "RT_static");
    sprite_c *sprite = ecs_get_mut (world, ent, sprite_c);
    string_set_str (sprite->name, "T_Sprite_Rock0.png");
  }
  {
    ecs_entity_t pfb = ecs_lookup (world, "grid_object_pfb");
    ecs_entity_t ent
        = ecs_entity (world, { .name = "wall_pfb",
                               .add = ecs_ids (EcsPrefab, ecs_isa (pfb)) });
    cache_c *cache = ecs_ensure (world, ent, cache_c);
    string_set_str (cache->cache_name, "RT_static");
    color_c *color = ecs_ensure (world, ent, color_c);
    color->default_r = 66u;
    color->default_g = 125u;
    color->default_b = 45u;
    sprite_c *sprite = ecs_get_mut (world, ent, sprite_c);
    sprite->b_uses_color = true;
    string_set_str (sprite->name, "T_Sprite_Wall0.png");
  }
  {
    ecs_entity_t pfb = ecs_lookup (world, "grid_object_pfb");
    ecs_entity_t ent
        = ecs_entity (world, { .name = "bomb_pfb",
                               .add = ecs_ids (EcsPrefab, ecs_isa (pfb)) });
    lifetime_c *lifetime = ecs_ensure (world, ent, lifetime_c);
    lifetime->on_delete_callback = detonate_bomb;
    sprite_c *sprite = ecs_get_mut (world, ent, sprite_c);
    string_set_str (sprite->name, "T_Sprite_Bomb0.png");
  }
  {
    ecs_entity_t pfb = ecs_lookup (world, "grid_object_pfb");
    ecs_entity_t ent
        = ecs_entity (world, { .name = "explosion_pfb",
                               .add = ecs_ids (EcsPrefab, ecs_isa (pfb)) });
    anim_player_c *anim_player = ecs_ensure (world, ent, anim_player_c);
    dict_string_anim_pose_init (anim_player->poses);
    struct anim_pose *pose = SDL_malloc (sizeof (struct anim_pose));
    dict_sint32_anim_flipbook_init (pose->directions);
    struct anim_flipbook *flipbook
        = SDL_malloc (sizeof (struct anim_flipbook));
    string_init_set_str (flipbook->name, "T_Flipbook_Explo0.png");
    flipbook->frame_size = (SDL_FPoint){ 32.f, 32.f };
    flipbook->frame_count = (SDL_Point){ 4, 1 };
    flipbook->play_speed = 18u;
    dict_sint32_anim_flipbook_set_at (pose->directions, 0, flipbook);
    dict_string_anim_pose_set_at (anim_player->poses, STRING_CTE ("default"),
                                  pose);
    string_init_set_str (anim_player->control_pose, "default");
    anim_player->control_direction = 0;
    lifetime_c *lifetime = ecs_ensure (world, ent, lifetime_c);
    lifetime->duration = 150u;
    sprite_c *sprite = ecs_get_mut (world, ent, sprite_c);
  }
}

static void
init_game_queries (ecs_world_t *world)
{
  game_s *game = ecs_singleton_ensure (world, game_s);
  dict_string_to_query_ptr_init (game->queries);
  {
    ecs_query_t *q = ecs_query (
        world, { .terms = { { ecs_isa (ecs_lookup (world, "rock_pfb")) } } });
    dict_string_to_query_ptr_set_at (game->queries,
                                     STRING_CTE ("get_all_rocks"), q);
  }
}

static void
init_game_systems (ecs_world_t *world)
{
  ECS_SYSTEM (world, system_lifetime_progress, EcsOnUpdate, lifetime_c);
}

static void
init_game_hooks (ecs_world_t *world)
{
  ecs_type_hooks_t cell_data_hooks = { .ctor = cell_data };
  ecs_set_hooks_id (world, ecs_id (cell_data_c), &cell_data_hooks);

  ecs_type_hooks_t lifetime_hooks = { .ctor = lifetime };
  ecs_set_hooks_id (world, ecs_id (lifetime_c), &lifetime_hooks);
}

int
main (int argc, char *argv[])
{
  ecs_world_t *world = ecs_init ();

  SDL_Point default_size = { .x = 480, .y = 480 };

  struct pluto_core_params params
      = { .init_flags = SDL_INIT_VIDEO,
          .window_name = "Doomsday",
          .window_size = (SDL_Point){ default_size.x, default_size.y },
          .window_flags = SDL_WINDOW_RESIZABLE,
          .default_user_scaling = 1.f,
          .renderer_blend_mode = SDL_BLENDMODE_BLEND,
          .logical_presentation_mode = SDL_LOGICAL_PRESENTATION_INTEGER_SCALE,
          .b_has_logical_size = true,
          .gpu_driver_hint = "vulkan",
          .b_is_DPI_aware = false,
          .b_should_debug_GPU = true,
          .input_data = { .b_is_resizing_widget = false,
                          .b_is_dragging_widget = false,
                          .b_is_moving_camera = false } };
  core_s *core = init_pluto (world, &params);

  ECS_COMPONENT_DEFINE (world, game_s);
  game_s *game = ecs_singleton_ensure (world, game_s);
  mat2d_entity_init (game->cells);

  ECS_COMPONENT_DEFINE (world, cell_data_c);
  ECS_COMPONENT_DEFINE (world, lifetime_c);

  satlas_dir_to_sheets (core->atlas, "dat/gfx", false, STRING_CTE ("sprites"));

  render_target_add_to_pool (core->rts, STRING_CTE ("RT_static"),
                             (SDL_Point){ default_size.x, default_size.y });
  render_target_clear (core->rts, STRING_CTE ("RT_static"), 0, 0, 0, 255);

  {
    ecs_entity_t ent = ecs_entity (world, { .name = "stage" });
    bounds_c *bounds = ecs_ensure (world, ent, bounds_c);
    bounds->size = (SDL_FPoint){ .x = (float)default_size.x - 1.f,
                                 .y = (float)default_size.y - 1.f };
    box_c *box = ecs_ensure (world, ent, box_c);
    box->b_is_filled = true;
    layer_c *layer = ecs_ensure (world, ent, layer_c);
    origin_c *origin = ecs_ensure (world, ent, origin_c);
    origin->relative = (SDL_FPoint){ 1.f, 1.f };
    visibility_c *visibility = ecs_ensure (world, ent, visibility_c);
    visibility->b_state = true;
  }
  init_game_hooks (world);
  init_game_prefabs (world);
  init_game_queries (world);
  init_game_systems (world);
  create_map (world);
  create_bombers (world);

  SDL_Event e;
  while (1)
    {
      game->control_delta.x = 0;
      game->control_delta.y = 0;
      while (SDL_PollEvent (&e) > 0)
        {
          if (e.type == SDL_EVENT_QUIT)
            {
              SDL_Quit ();
              exit (0);
            }
          if (e.type == SDL_EVENT_KEY_DOWN)
            {
              input_man_register_scancode (core->input_man, e.key.scancode,
                                           world);
            }
          if (e.type == SDL_EVENT_KEY_UP)
            {
              input_man_unregister_scancode (core->input_man, e.key.scancode,
                                             world);
            }
          if (e.type == SDL_EVENT_MOUSE_MOTION)
            {
              input_man_try_handle_mouse_motion (core->input_man, e.motion,
                                                 world);
            }
          if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            {
              input_man_try_handle_mouse_down (core->input_man, e.button,
                                               world);
            }

          if (e.type == SDL_EVENT_MOUSE_BUTTON_UP)
            {
              input_man_try_handle_mouse_up (core->input_man, e.button, world);
            }
        }
      input_man_bounce_keys (core->input_man, world);
      try_move_character (world, game->player, game->control_delta);
      SDL_SetRenderDrawColor (core->rend, 0, 0, 0, 255);
      SDL_RenderClear (core->rend);
      ecs_progress (world, 0.f);
      SDL_RenderPresent (core->rend);
    }

  return 0;
}
