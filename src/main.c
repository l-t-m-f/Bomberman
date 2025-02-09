#include "SDL3/SDL.h"

#include "pluto.h"

#include "input_man.h"
#include "log.h"
#include "m-dict.h"

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

  ecs_world_t *ecs = param;
  if (key == SDL_SCANCODE_F)
    {
      core_s *core = ecs_get_mut (ecs, ecs_id (core_s), core_s);
      core->b_is_fullscreen_presentation = !core->b_is_fullscreen_presentation;
      SDL_SetWindowFullscreen (core->win, core->b_is_fullscreen_presentation);
    }
  if (key == SDL_SCANCODE_G)
    {
      const game_s *game = ecs_singleton_get (ecs, game_s);
      if (b_has_shift_mod == true)
        {
          ecs_iter_t it = ecs_query_iter (
              ecs, *dict_string_to_query_ptr_get (
                       game->queries, STRING_CTE ("get_all_rocks")));
          while (ecs_query_next (&it))
            {
              for (Sint32 i = 0; i < it.count; i++)
                {
                  box_c *box = ecs_get_mut (ecs, it.entities[i], box_c);
                  box->b_is_shown = !box->b_is_shown;
                  ecs_modified (ecs, it.entities[i], box_c);
                }
            }
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
create_map (ecs_world_t *ecs)
{
  for (Sint32 i = 0; i < MAP_WIDTH; i++)
    {
      for (Sint32 j = 0; j < MAP_HEIGHT; j++)
        {
          {
            ecs_entity_t pfb = ecs_lookup (ecs, "rock_pfb");
            ecs_entity_t ent = ecs_new_w_pair (ecs, EcsIsA, pfb);
            index_c *index = ecs_get_mut (ecs, ent, index_c);
            index->x = i;
            index->y = j;
          }
        }
    }
}

static void
create_prefabs (ecs_world_t *ecs)
{
  {
    ecs_entity_t ent = ecs_entity (
        ecs, { .name = "grid_element_pfb", .add = ecs_ids (EcsPrefab) });
    index_c *index = ecs_ensure (ecs, ent, index_c);
    origin_c *origin = ecs_ensure (ecs, ent, origin_c);
    origin->relative_callback = get_relative_from_index;
  }
  {
    ecs_entity_t pfb = ecs_lookup (ecs, "grid_element_pfb");
    ecs_entity_t ent
        = ecs_entity (ecs, { .name = "grid_object_pfb",
                             .add = ecs_ids (EcsPrefab, ecs_isa (pfb)) });
    bounds_c *bounds = ecs_ensure (ecs, ent, bounds_c);
    bounds->size = (SDL_FPoint){ CELL_SIZE, CELL_SIZE };
    box_c *box = ecs_ensure (ecs, ent, box_c);
    box->b_is_shown = false;
    box->b_uses_color = true;
    color_c *color = ecs_ensure (ecs, ent, color_c);
    color->default_r = 0u;
    color->default_g = 155u;
    color->default_b = 0u;
    layer_c *layer = ecs_ensure (ecs, ent, layer_c);
    sprite_c *sprite = ecs_ensure (ecs, ent, sprite_c);
    visibility_c *visibility = ecs_ensure (ecs, ent, visibility_c);
    visibility->b_state = true;
  }
  {
    ecs_entity_t pfb = ecs_lookup (ecs, "grid_object_pfb");
    ecs_entity_t ent
        = ecs_entity (ecs, { .name = "rock_pfb",
                             .add = ecs_ids (EcsPrefab, ecs_isa (pfb)) });
    sprite_c *sprite = ecs_get_mut (ecs, ent, sprite_c);
    string_init_set_str (sprite->name, "T_Obj_Rock0.png");
  }
}

static void
create_queries (ecs_world_t *ecs)
{
  game_s *game = ecs_singleton_ensure (ecs, game_s);
  dict_string_to_query_ptr_init (game->queries);
  {
    ecs_query_t *q = ecs_query (
        ecs, { .terms = { { ecs_isa (ecs_lookup (ecs, "rock_pfb")) } } });
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
  create_map (ecs);
  create_queries (ecs);

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
