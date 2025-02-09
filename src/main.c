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

DICT_DEF2 (dict_string_to_query_ptr, string_t, M_STRING_OPLIST, ecs_query_t *,
           M_PTR_OPLIST)

typedef struct singleton_game
{
  ecs_entity_t player;
  mat2d_entity_t cells;
  ecs_entity_t current_scene;
  dict_string_to_query_ptr_t queries;
} game_s;

ECS_COMPONENT_DECLARE (game_s);

void
handle_key_press (struct input_man *input_man, SDL_Scancode key, void *param)
{
  const bool b_has_shift_mod
      = *arr_bool_get (input_man->keyboard.key_flips, SDL_SCANCODE_LSHIFT)
        || *arr_bool_get (input_man->keyboard.key_flips, SDL_SCANCODE_RSHIFT);

  ecs_world_t *world = param;
  if (key == SDL_SCANCODE_F)
    {
      core_s *core = ecs_get_mut (world, ecs_id (core_s), core_s);
      core->b_is_fullscreen_presentation = !core->b_is_fullscreen_presentation;
      SDL_SetWindowFullscreen (core->win, core->b_is_fullscreen_presentation);
    }
  if (key == SDL_SCANCODE_G)
    {
      const game_s *game = ecs_singleton_get (world, game_s);
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
      const game_s *game = ecs_singleton_get (world, game_s);
      movement_c *movement = ecs_get_mut (world, game->player, movement_c);
      movement->delta.y = -1;
      ecs_modified(world, game->player, movement_c);
    }
  if (key == SDL_SCANCODE_A)
    {
      const game_s *game = ecs_singleton_get (world, game_s);
      movement_c *movement = ecs_get_mut (world, game->player, movement_c);
      movement->delta.x = -1;
      ecs_modified(world, game->player, movement_c);
    }
  if (key == SDL_SCANCODE_S)
    {
      const game_s *game = ecs_singleton_get (world, game_s);
      movement_c *movement = ecs_get_mut (world, game->player, movement_c);
      movement->delta.y = 1;
      ecs_modified(world, game->player, movement_c);
    }
  if (key == SDL_SCANCODE_D)
    {
      const game_s *game = ecs_singleton_get (world, game_s);
      movement_c *movement = ecs_get_mut (world, game->player, movement_c);
      movement->delta.x = 1;
      ecs_modified(world, game->player, movement_c);
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
  if (key == SDL_SCANCODE_W)
    {
      const game_s *game = ecs_singleton_get (world, game_s);
      movement_c *movement = ecs_get_mut (world, game->player, movement_c);
      movement->delta.y = -1;
      ecs_modified(world, game->player, movement_c);
    }
  if (key == SDL_SCANCODE_A)
    {
      const game_s *game = ecs_singleton_get (world, game_s);
      movement_c *movement = ecs_get_mut (world, game->player, movement_c);
      movement->delta.x = -1;
      ecs_modified(world, game->player, movement_c);
    }
  if (key == SDL_SCANCODE_S)
    {
      const game_s *game = ecs_singleton_get (world, game_s);
      movement_c *movement = ecs_get_mut (world, game->player, movement_c);
      movement->delta.y = 1;
      ecs_modified(world, game->player, movement_c);
    }
  if (key == SDL_SCANCODE_D)
    {
      const game_s *game = ecs_singleton_get (world, game_s);
      movement_c *movement = ecs_get_mut (world, game->player, movement_c);
      movement->delta.x = 1;
      ecs_modified(world, game->player, movement_c);
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
    string_init_set_str (sprite->name, "T_Sprite_Bomber0.png");
    game->player = ent;
  }
}

static void
create_map (ecs_world_t *world)
{
  for (Sint32 i = 0; i < MAP_WIDTH; i++)
    {
      for (Sint32 j = 0; j < MAP_HEIGHT; j++)
        {
          {
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
            }
          }
        }
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
    string_init_set_str (render_target->name, "RT_static");
    visibility_c *visibility = ecs_ensure (world, ent, visibility_c);
    visibility->b_state = true;
  }
}

static void
create_prefabs (ecs_world_t *world)
{
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
    string_init_set_str (cache->cache_name, "RT_static");
    color_c *color = ecs_ensure (world, ent, color_c);
    color->default_r = 125u;
    color->default_g = 0u;
    color->default_b = 125u;
    sprite_c *sprite = ecs_get_mut (world, ent, sprite_c);
    sprite->b_uses_color = true;
    string_init_set_str (sprite->name, "T_Sprite_Floor0.png");
  }
  {
    ecs_entity_t pfb = ecs_lookup (world, "grid_object_pfb");
    ecs_entity_t ent
        = ecs_entity (world, { .name = "rock_pfb",
                               .add = ecs_ids (EcsPrefab, ecs_isa (pfb)) });
    cache_c *cache = ecs_ensure (world, ent, cache_c);
    string_init_set_str (cache->cache_name, "RT_static");
    sprite_c *sprite = ecs_get_mut (world, ent, sprite_c);
    string_init_set_str (sprite->name, "T_Sprite_Rock0.png");
  }
  {
    ecs_entity_t pfb = ecs_lookup (world, "grid_object_pfb");
    ecs_entity_t ent
        = ecs_entity (world, { .name = "wall_pfb",
                               .add = ecs_ids (EcsPrefab, ecs_isa (pfb)) });
    cache_c *cache = ecs_ensure (world, ent, cache_c);
    string_init_set_str (cache->cache_name, "RT_static");
    color_c *color = ecs_ensure (world, ent, color_c);
    color->default_r = 66u;
    color->default_g = 125u;
    color->default_b = 45u;
    sprite_c *sprite = ecs_get_mut (world, ent, sprite_c);
    sprite->b_uses_color = true;
    string_init_set_str (sprite->name, "T_Sprite_Wall0.png");
  }
}

static void
create_queries (ecs_world_t *world)
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

int
main (int argc, char *argv[])
{
  ecs_world_t *ecs = ecs_init ();

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
  core_s *core = init_pluto (ecs, &params);

  ECS_COMPONENT_DEFINE (ecs, game_s);

  satlas_dir_to_sheets (core->atlas, "dat/gfx", false, STRING_CTE ("sprites"));

  render_target_add_to_pool (core->rts, STRING_CTE ("RT_static"),
                             (SDL_Point){ default_size.x, default_size.y });
  render_target_clear (core->rts, STRING_CTE ("RT_static"), 0, 0, 0, 255);

  {
    ecs_entity_t ent = ecs_entity (ecs, { .name = "stage" });
    bounds_c *bounds = ecs_ensure (ecs, ent, bounds_c);
    bounds->size = (SDL_FPoint){ .x = (float)default_size.x - 1.f,
                                 .y = (float)default_size.y - 1.f };
    box_c *box = ecs_ensure (ecs, ent, box_c);
    box->b_is_filled = true;
    layer_c *layer = ecs_ensure (ecs, ent, layer_c);
    origin_c *origin = ecs_ensure (ecs, ent, origin_c);
    origin->relative = (SDL_FPoint){ 1.f, 1.f };
    visibility_c *visibility = ecs_ensure (ecs, ent, visibility_c);
    visibility->b_state = true;
  }
  create_prefabs (ecs);
  create_queries (ecs);
  create_map (ecs);
  create_bombers (ecs);

  SDL_Event e;
  while (1)
    {
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
                                           ecs);
            }
          if (e.type == SDL_EVENT_KEY_UP)
            {
              input_man_unregister_scancode (core->input_man, e.key.scancode,
                                             ecs);
            }
          if (e.type == SDL_EVENT_MOUSE_MOTION)
            {
              input_man_try_handle_mouse_motion (core->input_man, e.motion,
                                                 ecs);
            }
          if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            {
              input_man_try_handle_mouse_down (core->input_man, e.button, ecs);
            }

          if (e.type == SDL_EVENT_MOUSE_BUTTON_UP)
            {
              input_man_try_handle_mouse_up (core->input_man, e.button, ecs);
            }
        }
      input_man_bounce_keys (core->input_man, ecs);
      SDL_SetRenderDrawColor (core->rend, 0, 0, 0, 255);
      SDL_RenderClear (core->rend);
      ecs_progress (ecs, 0.f);
      SDL_RenderPresent (core->rend);
    }

  return 0;
}
