import sys
import pygame
import cpartsim

WIDTH = 800
HEIGHT = 800

NUM_PARTICLES = 2500
COLORS        = 6

DOT_RADIUS = 2   
FPS_CAP    = 60

def init_simulation_with_seed(n_particles, seed=None):
    if seed is None:
        cpartsim.init_simulation(n_particles)
    else:
        cpartsim.init_simulation(n_particles, seed)
    return cpartsim.get_seed()

def main():
    pygame.init()
    screen = pygame.display.set_mode((WIDTH, HEIGHT))
    pygame.display.set_caption("Particle Sim w/ Mouse-Attract (C accelerated)")
    clock = pygame.time.Clock()

    font = pygame.font.Font(None, 24)

    dot_surfaces = []
    for c in range(COLORS):
        surf = pygame.Surface((DOT_RADIUS * 2, DOT_RADIUS * 2), pygame.SRCALPHA)
        color = pygame.Color(0)
        color.hsva = (360.0 / COLORS * c, 100, 100, 100)
        pygame.draw.circle(surf, color, (DOT_RADIUS, DOT_RADIUS), DOT_RADIUS)
        dot_surfaces.append(surf.convert_alpha())

    if len(sys.argv) >= 2:
        try:
            provided_seed = int(sys.argv[1])
        except ValueError:
            print("Error: Seed must be an integer. Falling back to time-based seed.")
            provided_seed = None
    else:
        provided_seed = None

    current_seed = init_simulation_with_seed(NUM_PARTICLES, provided_seed)
    print(f"Seed used: {current_seed}")

    running = True
    paused  = False

    seed_input_active = False
    seed_input_text   = ""

    x_list, y_list, color_list = cpartsim.get_positions()

    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False

            elif event.type == pygame.KEYDOWN:

                if seed_input_active:
                    if event.key == pygame.K_RETURN:
                        try:
                            new_seed = int(seed_input_text)
                            current_seed = init_simulation_with_seed(NUM_PARTICLES, new_seed)
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
                    running = False

                elif event.key == pygame.K_s:
                    seed_input_active = True
                    seed_input_text = ""
                    paused = True

                elif event.key == pygame.K_r:
                    current_seed = init_simulation_with_seed(NUM_PARTICLES, current_seed)
                    print(f"Reinitialized with same seed: {current_seed}")
                    x_list, y_list, color_list = cpartsim.get_positions()

                elif event.key == pygame.K_n:
                    current_seed = init_simulation_with_seed(NUM_PARTICLES, None)
                    print(f"Reinitialized with new seed: {current_seed}")
                    x_list, y_list, color_list = cpartsim.get_positions()

                elif event.key == pygame.K_SPACE:
                    paused = not paused

        if (not paused) and (not seed_input_active):

            mx, my = pygame.mouse.get_pos()
            norm_x = mx / WIDTH
            norm_y = my / HEIGHT
            left_pressed = pygame.mouse.get_pressed()[0]  

            if 0.0 <= norm_x <= 1.0 and 0.0 <= norm_y <= 1.0:
                cpartsim.set_cursor(norm_x, norm_y, 1 if left_pressed else 0)
            else:

                cpartsim.set_cursor(-1.0, -1.0, 0)

            cpartsim.update_frame()

            x_list, y_list, color_list = cpartsim.get_positions()

        screen.fill((0, 0, 0))

        for i in range(NUM_PARTICLES):
            sx = int(x_list[i] * WIDTH)  - DOT_RADIUS
            sy = int(y_list[i] * HEIGHT) - DOT_RADIUS
            c  = color_list[i]
            screen.blit(dot_surfaces[c], (sx, sy))

        if seed_input_active:
            prompt = "Enter seed: " + seed_input_text
            txt_surf = font.render(prompt, True, (255, 255, 255))
            bg_rect = pygame.Rect(5, 5, txt_surf.get_width() + 10, txt_surf.get_height() + 6)
            pygame.draw.rect(screen, (0, 0, 0), bg_rect)
            screen.blit(txt_surf, (10, 8))
        else:
            info = f"Seed: {current_seed}    (Hold LMB to attract, R: replay, N: new seed, S: set seed, SPACE: pause)"
            info_surf = font.render(info, True, (255, 255, 255))
            bg_rect = pygame.Rect(5, 5, info_surf.get_width() + 10, info_surf.get_height() + 6)
            pygame.draw.rect(screen, (0, 0, 0), bg_rect)
            screen.blit(info_surf, (10, 8))

        pygame.display.flip()
        clock.tick(FPS_CAP)

    pygame.quit()
    sys.exit()

if __name__ == "__main__":
    main()