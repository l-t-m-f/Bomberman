#include "SDL3/SDL.h"

#include "pluto.h"

#include "input_man.h"
#include "log.h"

Sint32 DEBUG_LOG = DEBUG_LOG_NONE;

typedef struct singleton_game
{
  ecs_entity_t player;
  mat2d_entity_t cells;
  ecs_entity_t current_scene;
} game_s;

ECS_COMPONENT_DECLARE (game_s);

void
handle_key_press (struct input_man *input_man, SDL_Scancode key, void *param)
{
  ecs_world_t *ecs = param;
  const core_s *core = ecs_singleton_get (ecs, core_s);
  if(key == SDL_SCANCODE_F)
    {
      SDL_SetWindowFullscreen (core->win, true);
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

int
main (int argc, char *argv[])
{
  ecs_world_t *ecs = ecs_init ();

  SDL_Point default_size = { .x = 352, .y = 352  };

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
          .b_is_DPI_aware = true,
          .b_should_debug_GPU = true,
          .input_data = { .b_is_resizing_widget = false,
                          .b_is_dragging_widget = false,
                          .b_is_moving_camera = false } };
  core_s *core = init_pluto (ecs, &params);

  ECS_COMPONENT_DEFINE (ecs, game_s);
  game_s *game = ecs_singleton_ensure (ecs, game_s);

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
