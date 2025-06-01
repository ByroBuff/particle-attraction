import sys
import pygame
import cpartsim
import config   

def init_simulation_with_seed(n_particles, seed=None):
    if seed is None:
        cpartsim.init_simulation(n_particles)
    else:
        cpartsim.init_simulation(n_particles, seed)
    return cpartsim.get_seed()

def main():
    pygame.init()

    cpartsim.set_constants(
        config.DT,
        config.FRICTION_HALF,
        config.R_MAX,
        config.COLORS,
        config.FORCE_FACTOR,
        config.R_MOUSE,
        config.K_MOUSE
    )

    if config.FULLSCREEN:

        screen = pygame.display.set_mode((0, 0), pygame.FULLSCREEN)
        WIDTH, HEIGHT = screen.get_size()
    else:

        WIDTH  = config.SCREEN_WIDTH
        HEIGHT = config.SCREEN_HEIGHT
        screen = pygame.display.set_mode((WIDTH, HEIGHT))

    pygame.display.set_caption("Particle Sim w/ Mouse‐Attract")

    sim_size = min(WIDTH, HEIGHT)
    offset_x = (WIDTH  - sim_size) // 2
    offset_y = (HEIGHT - sim_size) // 2

    dot_surfaces = []
    for c in range(config.COLORS):
        surf = pygame.Surface((config.DOT_RADIUS * 2, config.DOT_RADIUS * 2), pygame.SRCALPHA)
        color = pygame.Color(0)
        color.hsva = (360.0 / config.COLORS * c, 100, 100, 100)
        pygame.draw.circle(
            surf,
            color,
            (config.DOT_RADIUS, config.DOT_RADIUS),
            config.DOT_RADIUS
        )
        dot_surfaces.append(surf.convert_alpha())

    if len(sys.argv) >= 2:
        try:
            provided_seed = int(sys.argv[1])
        except ValueError:
            print("Error: Seed must be an integer. Falling back to time‐based seed.")
            provided_seed = None
    else:
        provided_seed = None

    current_seed = init_simulation_with_seed(config.NUM_PARTICLES, provided_seed)
    print(f"Seed used: {current_seed}")

    x_list, y_list, color_list = cpartsim.get_positions()

    paused = False
    seed_input_active = False
    seed_input_text = ""
    clock = pygame.time.Clock()
    font = pygame.font.Font(None, 24)

    while True:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                return

            elif event.type == pygame.KEYDOWN:

                if seed_input_active:
                    if event.key == pygame.K_RETURN:
                        try:
                            new_seed = int(seed_input_text)
                            current_seed = init_simulation_with_seed(config.NUM_PARTICLES, new_seed)
                            print(f"Reinitialized with custom seed: {current_seed}")
                        except ValueError:
                            print("Invalid seed entered; must be an integer.")
                        seed_input_active = False
                        seed_input_text = ""
                        paused = False
                        x_list, y_list, color_list = cpartsim.get_positions()

                    elif event.key == pygame.K_BACKSPACE:
                        seed_input_text = seed_input_text[:-1]

                    elif event.key == pygame.K_ESCAPE:
                        seed_input_active = False
                        seed_input_text = ""
                        paused = False

                    else:
                        if event.unicode.isdigit():
                            seed_input_text += event.unicode
                    continue  

                if event.key == pygame.K_ESCAPE:
                    pygame.quit()
                    return

                elif event.key == pygame.K_s:
                    seed_input_active = True
                    seed_input_text = ""
                    paused = True

                elif event.key == pygame.K_r:
                    current_seed = init_simulation_with_seed(config.NUM_PARTICLES, current_seed)
                    print(f"Reinitialized with same seed: {current_seed}")
                    x_list, y_list, color_list = cpartsim.get_positions()

                elif event.key == pygame.K_n:
                    current_seed = init_simulation_with_seed(config.NUM_PARTICLES, None)
                    print(f"Reinitialized with new seed: {current_seed}")
                    x_list, y_list, color_list = cpartsim.get_positions()

                elif event.key == pygame.K_SPACE:
                    paused = not paused

        if not paused and not seed_input_active:
            mx, my = pygame.mouse.get_pos()
            left_pressed = pygame.mouse.get_pressed()[0]

            if (offset_x <= mx < offset_x + sim_size) and (offset_y <= my < offset_y + sim_size):
                norm_x = (mx - offset_x) / sim_size
                norm_y = (my - offset_y) / sim_size
                cpartsim.set_cursor(norm_x, norm_y, 1 if left_pressed else 0)
            else:
                cpartsim.set_cursor(-1.0, -1.0, 0)

            cpartsim.update_frame()
            x_list, y_list, color_list = cpartsim.get_positions()

        screen.fill((0, 0, 0))

        for i in range(config.NUM_PARTICLES):
            sx = offset_x + int(x_list[i] * sim_size) - config.DOT_RADIUS
            sy = offset_y + int(y_list[i] * sim_size) - config.DOT_RADIUS
            c_idx = color_list[i]
            screen.blit(dot_surfaces[c_idx], (sx, sy))

        if seed_input_active:
            prompt = "Enter seed: " + seed_input_text
            txt_surf = font.render(prompt, True, (255, 255, 255))
            bg_rect = pygame.Rect(5, 5, txt_surf.get_width() + 10, txt_surf.get_height() + 6)
            pygame.draw.rect(screen, (0, 0, 0), bg_rect)
            screen.blit(txt_surf, (10, 8))
        else:
            info = (
                f"Seed: {current_seed}    "
                "(Hold LMB to attract, R: replay, N: new seed, S: set seed, SPACE: pause)"
            )
            info_surf = font.render(info, True, (255, 255, 255))
            bg_rect   = pygame.Rect(5, 5, info_surf.get_width() + 10, info_surf.get_height() + 6)
            pygame.draw.rect(screen, (0, 0, 0), bg_rect)
            screen.blit(info_surf, (10, 8))

        pygame.display.flip()
        clock.tick(config.FPS_CAP)

if __name__ == "__main__":
    main()